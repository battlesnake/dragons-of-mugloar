#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <functional>

#include <getopt.h>

#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include "Locale.hpp"
#include "Game.hpp"
#include "ExtractFeatures.hpp"
#include "LogEvent.hpp"

using std::string;
using std::string_view;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::unordered_map;
using std::pair;
using std::tuple;
using std::function;
using std::to_string;
using std::cerr;
using std::endl;
using std::atomic;
using std::thread;
using std::mutex;
using std::scoped_lock;
using namespace mugloar;

using Costs = unordered_map<string, float>;

static mutex io_mutex;

static ofstream events;
static ofstream scores;

static atomic<bool> stopping { false };

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

static float play_move(mugloar::Game& game, const unordered_map<string, float>& costs)
{
	/* Build list of possible actions and action features */
	vector<tuple<string, function<void()>, function<void()>, float>> actions;
	actions.reserve(100);

	unordered_map<string, float> features;

	for (const auto& msg : game.messages()) {
		actions.push_back({
			"SOLVE " + msg.message + " FOR " + to_string(msg.reward) + " GOLD",
			[&] () { game.solve_message(msg); },
			[&] () { extract_action_features(features, msg); },
			0
			});
	}

	for (const auto& item : game.shop_items()) {
		/*
		 * Without this, the AI will just attempt to buy healing potions
		 * every turn.
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
			"BUY " + item.name + " FOR " + to_string(item.cost) + " GOLD",
			[&] () { game.purchase_item(item); },
			[&] () { extract_action_features(features, item); },
			0
			});
	}

	/* Calculate estimated costs */

	auto pre = GameState(game);

	typename decltype(actions)::pointer max = nullptr;
	for (auto& action : actions) {
		auto& [name, execute, get_features, score] = action;
		score = 0;
		/* Build feature set */
		features.clear();
		extract_game_state(features, pre);
		get_features();
		/* Calculate total cost of features */
		for (const auto& [feature, value] : features) {
			auto it = costs.find(feature);
			if (it != costs.end()) {
				score += value * it->second;
			} else {
				cerr << " * Unknown feature: " << feature << endl;
				score += -100;
			}
		}
		if (max == nullptr || score > std::get<3>(*max)) {
			max = &action;
		}
	}

	if (max == nullptr) {
		throw std::runtime_error("No actions!");
	}
	const auto& [name, execute, get_features, max_score] = *max;

	cerr << "Action chosen: (" << max_score << ") " << name << endl;

	execute();

	auto post = GameState(game);

	auto diff = post - pre;

	features.clear();
	extract_game_state(features, pre);
	get_features();
	extract_game_diff_state(features, diff);

	log_event(events, features);

	return max_score;

}

static void play_game(mugloar::Game& game, const Costs& costs)
{
	while (!stopping && !game.dead()) {
		cerr << "Game=" << game.id() << ", Turn=" << game.turn() << ", Score=" << game.score() << ", Lives=" << game.lives() << ", Gold=" << game.gold() << endl;
		play_move(game, costs);
	}
}

static void worker_task(int index, const mugloar::Api& api, const Costs& costs)
{
	do {

		mugloar::Game game(api);

		try {
			play_game(game, costs);
		} catch (std::runtime_error e) {
			cerr << "Worker #" << index << ": error: " << e.what() << endl;
			continue;
		}

		cerr << "ID=" << game.id() << ", score=" << game.score() << ", turns=" << game.turn() << endl;

		{
			scoped_lock lock(io_mutex);
			scores << "id=" << game.id() << "\tscore=" << game.score() << "\tturn=" << game.turn() << "\t" << endl;
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

	const auto raw_data = read_file(infilename);

	const auto costs = read_costs(raw_data);

	mugloar::Api api;
	events = ofstream(outfilename, std::ios::binary | std::ios_base::app);
	scores = ofstream(scorefilename, std::ios::binary | std::ios_base::app);

	if (once) {
		worker_task(0, api, costs);
		return 0;
	}

	vector<thread> workers;
	workers.reserve(worker_count);

	cerr << "Starting " << worker_count << " workers..." << endl;

	for (int i = 0; i < worker_count; i++) {
		workers.emplace_back([&, i] () { worker_task(i, api, costs); });
	}

	cerr << "Started." << endl;

	do {
		cerr << "Press <q> <ENTER> to stop." << endl;
	} while (getchar() != 'q');

	stopping = true;

	cerr << "Stopping..." << endl;

	for (auto& worker : workers) {
		worker.join();
	}

	cerr << "Stopped." << endl;

}
