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

#include "Menu.hpp"
#include "AnsiCodes.hpp"
#include "Game.hpp"
#include "CollectState.hpp"
#include "CollectActionFeatures.hpp"

using std::string;
using std::string_view;
using std::ifstream;
using std::vector;
using std::unordered_map;
using std::pair;
using std::tuple;
using std::function;
using std::to_string;
using std::cerr;
using std::endl;
using mugcollect::State;
using mugcollect::ActionFeatures;

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

/* Convert string to lowercase, assuming UTF-8 encoding */
static string lowercase(const string& in)
{
	icu::UnicodeString us(in.c_str(), "UTF-8");
	us = us.toLower();
	string out;
	us.toUTF8String(out);
	return out;
}

static float play_move(mugloar::Game& game, const unordered_map<string, float>& costs)
{
	auto state = State(game);

	/* Build list of possible actions and action features */
	vector<tuple<string, function<void()>, ActionFeatures, float>> actions;
	actions.reserve(100);

	for (const auto& msg : game.messages()) {
		vector<string> other;
		if (msg.encoded) {
			other.emplace_back("encoded");
		}
		other.emplace_back(msg.probability);
		actions.push_back({
			"SOLVE " + msg.message + " FOR " + to_string(msg.reward) + " GOLD",
			[&] () { game.solve_message(msg); },
			ActionFeatures("SOLVE", msg.message, other),
			0
			});
	}

	for (const auto& item : game.shop_items()) {
		vector<string> other;
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
		other.emplace_back(item.id);
		actions.push_back({
			"BUY " + item.name + " FOR " + to_string(item.cost) + " GOLD",
			[&] () { game.purchase_item(item); },
			ActionFeatures("BUY", item.name, other),
			0
			});
	}

	/* Calculate estimated costs */

	float base_cost = 0;

	base_cost += game.lives() * costs.at("lives");
	base_cost += game.score() * costs.at("score");
	base_cost += game.people_rep() * costs.at("people_rep");
	base_cost += game.state_rep() * costs.at("state_rep");
	base_cost += game.underworld_rep() * costs.at("underworld_rep");

	float max_cost = -std::numeric_limits<float>::infinity();
	function<void()> *max_action = nullptr;
	string *max_name = nullptr;
	for (auto& [name, action, features, cost] : actions) {
		cost = base_cost;
		for (const auto& word : features.words) {
			auto it = costs.find(lowercase(word));
			if (it != costs.end()) {
				cost += it->second;
			} else {
				cerr << " * Unknown feature: " << word << endl;
			}
		}
		if (cost > max_cost) {
			max_cost = cost;
			max_action = &action;
			max_name = &name;
		}
	}

	if (max_action == nullptr) {
		throw std::runtime_error("No actions!");
	}

	cerr << "Action chosen: " << *max_name << endl;

	max_action->operator()();

	return max_cost;

}

static void play_game(mugloar::Game& game, const unordered_map<string, float>& costs)
{
	while (!game.dead()) {
		play_move(game, costs);
		cerr << "Turn=" << game.turn() << ", Score=" << game.score() << ", Lives=" << game.lives() << ", Gold=" << game.gold() << endl;
	}
}

static void help()
{
	cerr << "Arguments:" << endl;
	cerr << "  -i input-filename" << endl;
}

int main(int argc, char *argv[])
{
	const char *infilename = nullptr;
	char c;
	bool once = false;
	while ((c = getopt(argc, argv, "hi:1")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'i': infilename = optarg; break;
		case '1': once = true; break;
		case '?': help(); return 1;
		}
	}

	if (!infilename || optind != argc) {
		help();
		return 1;
	}

	const auto raw_data = read_file(infilename);

	const auto costs = read_costs(raw_data);

	mugloar::Api api;

	bool quit = false;

	do {

		cerr << "Game starting..." << endl;

		mugloar::Game game(api);

		cerr << "Game started!" << endl << endl;

		play_game(game, costs);

		cerr << "ID=" << game.id() << ", score=" << game.score() << ", turns=" << game.turn() << endl;

		if (once) {
			quit = true;
		} else {
			menu({
				{ 'c', "Continue", [] () {} },
				{ 'q', "Quit", [&] () { quit = true; } }
			})();
		}

	} while (!quit);

}
