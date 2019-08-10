/*
 * Executes random actions to build/extend the initial training dataset for
 * costed feature AI agent.
 */
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>
#include <functional>
#include <unordered_map>

#include <fcntl.h>
#include <unistd.h>

#include <getopt.h>

#include "Locale.hpp"
#include "Game.hpp"
#include "ExtractFeatures.hpp"
#include "LogEvent.hpp"
#include "Parallel.hpp"

using std::thread;
using std::atomic;
using std::pair;
using std::vector;
using std::string;
using std::function;
using std::random_device;
using std::mt19937;
using std::uniform_int_distribution;
using std::mutex;
using std::scoped_lock;
using std::ofstream;
using std::unordered_map;
using std::cout;
using std::cerr;
using std::endl;
using std::get;
using namespace mugloar;

/* Output file for event log */
static ofstream outfile;

/* API binding */
static const Api api;

/* One worker (automated player) */
static void worker_task()
{
	random_device rd;
	mt19937 prng(rd());

	vector<pair<function<void()>, function<void()>>> actions;
	actions.reserve(100);

	while (!stopping) {

		/* Create a game */
		Game game(api);

		/* Initialise pre-action state to current state */
		GameState pre(game);

		/* Post-action state set here doesn't matter */
		GameState post;

		while (!stopping && !game.dead()) {

			/* Build list of possible actions and action features */

			actions.clear();

			unordered_map<string, float> features;

			/* Messages */
			for (const auto& msg : game.messages()) {
				actions.push_back({
					[&] () { game.solve_message(msg); },
					[&] () { extract_action_features(features, msg); }
					});
			}

			/* Items in shop */
			for (const auto& item : game.shop_items()) {
				actions.push_back({
					[&] () { game.purchase_item(item); },
					[&] () { extract_action_features(features, item); }
					});
			}

			/* Randomly select an action */
			int action_idx = uniform_int_distribution<int>(0, actions.size() - 1)(prng);
			const auto& [action, get_features] = actions[action_idx];

			/* Get action features */
			get_features();

			/* Execute action */
			try {
				action();
			} catch (const std::runtime_error& e) {
				cerr << "Exception in worker " << worker_id << ": " << e.what() << endl;
				break;
			}

			/* Calculate post-action state */
			post = GameState(game);

			auto diff = post - pre;

			extract_game_state(features, pre);
			extract_game_diff_state(features, diff);

			/* Emit action features and state change */
			log_event(outfile, game, features);

			/* This post-action state is the next iteration's pre-action state */
			pre = post;

		}

	}
}

static void help()
{
	cerr << "Arguments:" << endl;
	cerr << "  -o output-filename" << endl;
	cerr << "  -p worker-count" << endl;
}

int main(int argc, char *argv[])
{
	init_locale();

	char c;
	int worker_count = 4;
	const char *outfilename = nullptr;
	while ((c = getopt(argc, argv, "hp:o:")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'p': worker_count = std::atoi(optarg); break;
		case 'o': outfilename = optarg; break;
		case '?': help(); return 1;
		}
	}

	if (!outfilename || worker_count <= 0 || optind != argc) {
		help();
		return 1;
	}

	/* Open output file */
	outfile = ofstream(outfilename, std::ios::binary | std::ios_base::app);

	/* Start workers */
	run_parallel(worker_count, [&] () { worker_task(); });

}
