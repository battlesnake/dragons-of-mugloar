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

#include <fcntl.h>
#include <unistd.h>

#include <getopt.h>

#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include "Game.hpp"
#include "CollectState.hpp"
#include "CollectActionFeatures.hpp"

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
using std::cout;
using std::cerr;
using std::endl;
using std::get;
using mugloar::Number;
using mugloar::Api;
using mugloar::Game;
using mugcollect::State;
using mugcollect::ActionFeatures;

static ofstream outfile;

static atomic<bool> stopping{false};

using LogEntry = pair<ActionFeatures, State>;

/* Convert string to lowercase, assuming UTF-8 encoding */
static string lowercase(const string& in)
{
	icu::UnicodeString us(in.c_str(), "UTF-8");
	us = us.toLower();
	string out;
	us.toUTF8String(out);
	return out;
}

static void dump_log_header()
{
	static const vector<string> cols = {
		"delta_dead",
		"delta_lives",
		"delta_score",
		"delta_people_rep",
		"delta_state_rep",
		"delta_underworld_rep",

		"dead",
		"lives",
		"score",
		"people_rep",
		"state_rep",
		"underworld_rep",

		"action_features",
	};
	for (const auto& col : cols) {
		outfile << col << "\t";
	}
	outfile << endl;
}

static void dump_log(const State& pre, const State& post, const ActionFeatures& af)
{
	static size_t count = 0;
	static mutex buffer_dump_mx;
	scoped_lock lock(buffer_dump_mx);
	auto dump_v = [] (const auto& v) {
		outfile << get<0>(v) << "\t"
				<< get<1>(v) << "\t"
				<< get<2>(v) << "\t"
				<< get<3>(v) << "\t"
				<< get<4>(v) << "\t"
				<< get<5>(v) << "\t";
	};
	/* Knowledge gained from action */
	dump_v((post - pre).vector);
	/* Knowledge available before action (including knowledge about action) */
	dump_v(pre.vector);
	for (const auto& w : af.words) {
		outfile << lowercase(w) << "\t";
	}
	/* End of entry */
	outfile << endl;
	++count;
	if (count % 100 == 0) {
		cerr << "Logged " << count << " entries" << endl;
	}
}

/* One worker (automated player) */
static void worker_task(size_t worker_id, const Api& api)
{
	random_device rd;
	mt19937 prng(rd());

	vector<pair<function<void()>, function<ActionFeatures()>>> actions;
	actions.reserve(100);

	while (!stopping) {

		/* Create a game */
		Game game(api);

		/* Initialise pre-action state to current state */
		State pre(game);

		/* Post-action state set here doesn't matter */
		State post(pre);

		while (!stopping && !game.dead()) {

			/* Build list of possible actions and action features */
			actions.clear();

			for (const auto& msg : game.messages()) {
				actions.push_back({
					[&] () { game.solve_message(msg); },
					[&] () {
						vector<string> other;
						if (msg.encoded) {
							other.emplace_back("encoded");
						}
						other.emplace_back(msg.probability);
						return ActionFeatures("SOLVE", msg.message, other);
					}
					});
			}

			for (const auto& item : game.shop_items()) {
				actions.push_back({
					[&] () { game.purchase_item(item); },
					[&] () {
						vector<string> other;
						other.emplace_back(item.id);
						return ActionFeatures("BUY", item.name, other);
					}
					});
			}

			int action_idx = uniform_int_distribution<int>(0, actions.size() - 1)(prng);
			const auto& [action, get_features] = actions[action_idx];

			/* Get action features */
			auto features = get_features();

			/* Execute action */
			try {
				action();
			} catch (std::runtime_error e) {
				cerr << "Exception in worker " << worker_id << ": " << e.what() << endl;
				break;
			}

			/* Calculate post-action state */
			post = State(game);

			/* Emit action features and state change */
			dump_log(pre, post, features);

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

	outfile = ofstream(outfilename, std::ios::binary | std::ios_base::app);

	Api api;

	vector<thread> workers;
	workers.reserve(worker_count);

	cerr << "Starting " << worker_count << " workers..." << endl;

	dump_log_header();

	for (int i = 0; i < worker_count; ++i) {
		workers.emplace_back(worker_task, i, api);
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
