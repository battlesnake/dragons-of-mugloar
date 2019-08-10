/*
 * Basic AI agent, using pre-programmed rules
 *
 * Will also append to event log that's used by the costed feature
 * machine-learning AI agent.
 */
#include <atomic>
#include <mutex>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>

#include <getopt.h>

#include "Locale.hpp"
#include "Game.hpp"
#include "ExtractFeatures.hpp"
#include "LogEvent.hpp"
#include "AnsiCodes.hpp"
#include "Parallel.hpp"
#include "BasicAssist.hpp"

using std::string;
using std::string_view;
using std::ifstream;
using std::ofstream;
using std::ostream;
using std::stringstream;
using std::vector;
using std::unordered_map;
using std::pair;
using std::tuple;
using std::function;
using std::to_string;
using std::cerr;
using std::endl;
using std::flush;
using std::atomic;
using std::mutex;
using std::scoped_lock;
using namespace mugloar;

/* For synchronising IO to files and STDERR */
static mutex io_mutex;

/* Output files */
static ofstream scores;
static ofstream events;

static mutex score_mutex;
static pair<long, string> best_score { 0, "(none)" };
static vector<string> current_scores;

/* API binding */
static const mugloar::Api api;

static ofstream scoreboard_file;
static ostream *scoreboard_display;

static void play_move(mugloar::Game& game, ostream& ss)
{
	unordered_map<string, float> features;

	auto pre = GameState(game);

	if (auto items = sort_items(game); !items.empty()) {
		/* If "buy item" action available, do it */
		const auto& item = *items[0];
		ss << "Buying item " << Emph(Cyan(item.name)) << " for " << Yellow(int(item.cost)) << " gold" << endl;
		extract_action_features(features, item);
		game.purchase_item(item);
	} else if (auto msgs = sort_messages(game); !msgs.empty()) {
		/* Else, if "solve message" action available, do it */
		const auto& msg = *msgs[0];
		ss << "Solving message " << Emph(Cyan(msg.message)) << " for " << Yellow(int(msg.reward)) << " gold " << " with difficulty " << Magenta(msg.probability) << endl;
		extract_action_features(features, msg);
		game.solve_message(msg);
	} else {
		/* Else, burn a turn */
		game.update_reputation();
	}

	auto post = GameState(game);

	auto diff = post - pre;

	extract_game_state(features, pre);
	extract_game_diff_state(features, diff);

	/* Log features and changes */
	log_event(events, game, features);
}

static void print_scores()
{
	scoped_lock lock(io_mutex, score_mutex);
	ostream& ss = *scoreboard_display;
	ss << endl;
	for (size_t i = 0; i < current_scores.size(); i++) {
		ss << Strong("Worker #") << i << Strong(": ") << current_scores[i] << endl;
	}
	ss << endl;
	ss << Strong("Best: ") << best_score.second << endl;
	ss << endl;
}

static ostream& print_game(const mugloar::Game& game, ostream& ss)
{
	ss << Strong("Game=") << Emph(Cyan(game.id()))
		<< Strong(", Turn=") << Emph(int(game.turn()))
		<< Strong(", Score=") << Green(Strong(Emph(int(game.score()))))
		<< Strong(", Level=") << Red(Strong(Emph(int(game.level()))))
		<< Strong(", Lives=") << Magenta(Strong(Emph(int(game.lives()))))
		<< Strong(", Gold=") << Yellow(Strong(Emph(int(game.gold()))));
	return ss;
}

static void play_game(mugloar::Game& game)
{
	/*
	 * We don't care about reputation, we've already statically modelled it
	 * somewhat in the assistant
	 */
	game.autoupdate_reputation = false;

	/* Keep playing until we die */
	while (!stopping && !game.dead()) {

		/* Log game status */
		stringstream ss;
		print_game(game, ss) << endl;

		/* Play a move */
		play_move(game, ss);

		/* Update scoreboard */
		string stats;
		{
			stringstream ss;
			print_game(game, ss) << flush;
			stats = ss.str();
		}
		{
			scoped_lock lock(score_mutex);
			auto score = game.score();
			current_scores[worker_id] = stats;
			if (score > best_score.first) {
				best_score = { score, std::move(stats) };
			}
		}

		/* Check for status request */
		if (!status_request.test_and_set()) {
			print_scores();
		}

		/* Print move summary */
		ss << flush;
		{
			scoped_lock lock(io_mutex);
			cerr << ss.rdbuf() << endl;
		}

	}
}

static void worker_task()
{
	/* Keep playing games until stop is requested by user */
	do {

		/* Play game */
		mugloar::Game game(api);
		try {
			play_game(game);
		} catch (std::runtime_error e) {
			cerr << "Worker #" << worker_id << ": error: " << e.what() << endl;
		}

		/* Log game result */
		{
			stringstream ss;
			ss << "id=" << game.id() << "\tscore=" << game.score() << "\tturns=" << game.turn() << "\tlevel=" << game.level() << "\tlives=" << game.lives() << "\t" << endl << flush;
			auto str = ss.str();

			scoped_lock lock(io_mutex);
			cerr << str;
			scores << str << flush;
		}

	} while (!stopping);
}

static void help()
{
	cerr << "Arguments:" << endl;
	cerr << "  -o output-filename" << endl;
	cerr << "  -s score-filename" << endl;
	cerr << "  -p worker-count" << endl;
	cerr << "  -S scoreboard-filename" << endl;
	cerr << endl;
	cerr << "Send SIGHUP or SIGQUIT (^\\) to print the scoreboard" << endl;
	cerr << endl;
}

int main(int argc, char *argv[])
{
	init_locale();

	const char *outfilename = nullptr;
	const char *scorefilename = nullptr;
	const char *scoreboardfilename = nullptr;
	int worker_count = 20;

	char c;
	while ((c = getopt(argc, argv, "ho:s:p:S:")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'o': outfilename = optarg; break;
		case 's': scorefilename = optarg; break;
		case 'S': scoreboardfilename = optarg; break;
		case 'p': worker_count = std::stoi(optarg); break;
		case '?': help(); return 1;
		}
	}

	if (!outfilename || !scorefilename || worker_count <= 0 || optind != argc) {
		help();
		return 1;
	}

	/* Open output files */

	events = ofstream(outfilename, std::ios::binary | std::ios_base::app);
	scores = ofstream(scorefilename, std::ios::binary | std::ios_base::app);

	/* Scoreboard file: default to STDERR if no file specified */
	if (scoreboardfilename) {
		scoreboard_file = ofstream(scoreboardfilename, std::ios::binary | std::ios_base::app);
		scoreboard_display = &scoreboard_file;
	} else {
		scoreboard_display = &cerr;
	}

	/* Initialise score table */
	current_scores.resize(worker_count);
	std::fill(current_scores.begin(), current_scores.end(), "(starting)");

	/* Create workers */
	run_parallel(worker_count, worker_task);

	/* Print scores */
	print_scores();

}
