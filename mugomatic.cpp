#include <atomic>
#include <mutex>
#include <iostream>
#include <string>
#include <string_view>
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

using Costs = unordered_map<string, float>;

/* For synchronising IO to files and STDERR */
static mutex io_mutex;

/* Output files */
static ofstream events;
static ofstream scores;

/* Read file cells */
static vector<vector<string>> read_file(const string& in)
{
	cerr << "Reading file " << in << "..." << endl;

	ifstream f(in);
	vector<vector<string>> data;
	data.reserve(100000);
	/* Read file line-by-line */
	string line;
	while (getline(f, line)) {
		string_view sv(line);
		string_view::size_type end;
		/* Split line into fields by tab-terminator */
		auto& fields = data.emplace_back();
		fields.reserve(5);
		while ((end = sv.find('\t')) != string_view::npos) {
			fields.push_back(string(sv.substr(0, end)));
			sv.remove_prefix(end + 1);
		}
	}
	return data;
}

/* Read costs for state and for action features */
static unordered_map<string, float> read_costs(const vector<vector<string>>& data)
{
	cerr << "Building cost table..." << endl;

	unordered_map<string, float> costs;
	costs.reserve(data.size() * 3);
	for (const auto& line : data) {
		costs[line[2]] = std::stod(line[0]);
	}

	return costs;
}

static float play_move(mugloar::Game& game, const unordered_map<string, float>& costs, ostream& ss)
{
	/* Build list of possible actions and action features */
	vector<tuple<string, function<void()>, function<void()>, float>> actions;
	actions.reserve(100);

	unordered_map<string, float> features;

	/* Build action list for solving messages */
	for (const auto& msg : game.messages()) {
		actions.push_back({
			"SOLVE " + msg.message + " FOR " + to_string(int(msg.reward)) + " GOLD",
			[&] () { game.solve_message(msg); },
			[&] () { extract_action_features(features, msg); },
			0
			});
	}

	/* Build action list for buying items */
	for (const auto& item : game.shop_items()) {
		/*
		 * Without this, the AI will just attempt to buy healing potions
		 * every turn, since the average score of that (including fails)
		 * is always going to be bigger than non-trivial "solves".
		 * We could fix this by assigning a penalty to failed buy
		 * attempts, but this could still leave us stuck in a loop.
		 *
		 * With this, the AI will probably buy a healing potion whenever
		 * it can afford to.
		 *
		 * The learning process only analyses single moves and does not
		 * learn to buy other items based on future benefits that they
		 * deliver (as it doesn't look more than 1 move into the future
		 * from an action).
		 */
		if (item.cost > game.gold()) {
			continue;
		}
		actions.push_back({
			"BUY " + item.name + " FOR " + to_string(int(item.cost)) + " GOLD",
			[&] () { game.purchase_item(item); },
			[&] () { extract_action_features(features, item); },
			0
			});
	}

	/* Calculate estimated costs */

	auto pre = GameState(game);

	typename decltype(actions)::pointer max = nullptr;
	bool unknown = false;
	for (auto& action : actions) {
		auto& [name, execute, get_features, score] = action;
		score = 0;
		/* Build feature set */
		features.clear();
		extract_game_state(features, pre);
		features.erase("game:score");
		features.erase("game:lives");
		get_features();
		/* Calculate total cost of features */
		for (const auto& [feature, value] : features) {
			auto it = costs.find(feature);
			if (it != costs.end()) {
				score += value * it->second;
			} else {
				if (!unknown) {
					ss << " * Unknown feature:";
					unknown = true;
				}
				ss << "  [" << feature << "]";
				score += -100;
			}
		}
		if (max == nullptr || score > std::get<3>(*max)) {
			max = &action;
		}
	}
	if (unknown) {
		ss << endl;
	}

	if (max == nullptr) {
		throw std::runtime_error("No actions!");
	}
	const auto& [name, execute, get_features, max_score] = *max;

	/* Log move summary */
	ss << Strong(Magenta("Action chosen:")) << " cost=" << Emph(max_score) << " name=" << Emph(name) << endl;

	/* Execute move */
	execute();

	auto post = GameState(game);

	auto diff = post - pre;

	/* Rebuild feature set */
	features.clear();
	extract_game_state(features, pre);
	get_features();
	extract_game_diff_state(features, diff);

	/* Log features and changes */
	log_event(events, game, features);

	return max_score;

}

static void play_game(mugloar::Game& game, const Costs& costs)
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
		play_move(game, costs, ss);

		/* Print move summary */
		ss << flush;
		{
			scoped_lock lock(io_mutex);
			cerr << ss.rdbuf() << endl;
		}

	}
}

static void worker_task(int index, const mugloar::Api& api, const Costs& costs)
{
	do {

		/* Play game */
		mugloar::Game game(api);
		try {
			play_game(game, costs);
		} catch (std::runtime_error e) {
			cerr << "Worker #" << index << ": error: " << e.what() << endl;
			continue;
		}

		/* Log game result */
		{
			stringstream ss;
			ss << "id=" << game.id() << "\tscore=" << int(game.score()) << "\tturns=" << int(game.turn()) << "\tlevel=" << int(game.level()) << "\tlives=" << int(game.lives()) << "\t" << endl << flush;
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
	cerr << "  -i input-filename" << endl;
	cerr << "  -o output-filename" << endl;
	cerr << "  -s score-filename" << endl;
	cerr << "  -p worker-count" << endl;
}

int main(int argc, char *argv[])
{
	init_locale();

	const char *infilename = nullptr;
	const char *outfilename = nullptr;
	const char *scorefilename = nullptr;
	int worker_count = 20;
	bool once = false;

	char c;
	while ((c = getopt(argc, argv, "hi:o:s:p:1")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'i': infilename = optarg; break;
		case 'o': outfilename = optarg; break;
		case 's': scorefilename = optarg; break;
		case 'p': worker_count = std::stoi(optarg); break;
		case '1': once = true; break;
		case '?': help(); return 1;
		}
	}

	if (!infilename || !outfilename || !scorefilename || worker_count <= 0 || optind != argc) {
		help();
		return 1;
	}

	/* Load feature cost data */

	const auto raw_data = read_file(infilename);

	const auto costs = read_costs(raw_data);

	/* Open output files */

	events = ofstream(outfilename, std::ios::binary | std::ios_base::app);
	scores = ofstream(scorefilename, std::ios::binary | std::ios_base::app);

	/* API binding */
	mugloar::Api api;

	/* Run once in this thread if only one run requested */
	if (once) {
		worker_task(0, api, costs);
		return 0;
	}

	/* Create workers */
	run_parallel(worker_count, [&] (int i) { worker_task(i, api, costs); });

}
