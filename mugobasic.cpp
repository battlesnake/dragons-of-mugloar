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

static void play_move(mugloar::Game& game, ostream& ss)
{
	unordered_map<string, float> features;

	auto pre = GameState(game);

	if (auto items = sort_items(game); !items.empty()) {
		const auto& item = *items[0];
		ss << "Buying item " << Emph(Cyan(item.name)) << " for " << Yellow(item.cost) << " gold" << endl;
		extract_action_features(features, item);
		game.purchase_item(item);
	} else if (auto msgs = sort_messages(game); !msgs.empty()) {
		const auto& msg = *msgs[0];
		ss << "Solving message " << Emph(Cyan(msg.message)) << " for " << Yellow(msg.reward) << " gold " << " with difficulty " << Magenta(lookup_probability(msg.probability)) << endl;
		extract_action_features(features, msg);
		game.solve_message(msg);
	} else {
		game.update_reputation();
	}

	auto post = GameState(game);

	auto diff = post - pre;

	extract_game_state(features, pre);
	extract_game_diff_state(features, diff);

	/* Log features and changes */
	log_event(events, game, features);
}

static void play_game(mugloar::Game& game)
{
	while (!stopping && !game.dead()) {
		/* Log game status */
		stringstream ss;
		ss << Strong("Game=") << Emph(Cyan(game.id()))
			<< Strong(", Turn=") << Emph(int(game.turn()))
			<< Strong(", Score=") << Green(Strong(Emph(int(game.score()))))
			<< Strong(", Level=") << Red(Strong(Emph(int(game.level()))))
			<< Strong(", Lives=") << Magenta(Strong(Emph(int(game.lives()))))
			<< Strong(", Gold=") << Yellow(Strong(Emph(int(game.gold()))))
			<< endl;

		/* Play a move */
		play_move(game, ss);

		/* Print move summary */
		ss << flush;
		{
			scoped_lock lock(io_mutex);
			cerr << ss.rdbuf() << endl;
		}

	}
}

static void worker_task(int index, const mugloar::Api& api)
{
	do {

		/* Play game */
		mugloar::Game game(api);
		try {
			play_game(game);
		} catch (std::runtime_error e) {
			cerr << "Worker #" << index << ": error: " << e.what() << endl;
			continue;
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
}

int main(int argc, char *argv[])
{
	init_locale();

	const char *outfilename = nullptr;
	const char *scorefilename = nullptr;
	int worker_count = 20;
	bool once = false;

	char c;
	while ((c = getopt(argc, argv, "ho:s:p:1")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'o': outfilename = optarg; break;
		case 's': scorefilename = optarg; break;
		case 'p': worker_count = std::stoi(optarg); break;
		case '1': once = true; break;
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

	/* API binding */
	mugloar::Api api;

	/* Run once in this thread if only one run requested */
	if (once) {
		worker_task(0, api);
		return 0;
	}

	/* Create workers */
	run_parallel(worker_count, [&] (int i) { worker_task(i, api); });

}
