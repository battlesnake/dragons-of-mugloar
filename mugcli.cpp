/*
 * Text-user interface to manually play the game, with recommendations for item
 * purchases and quest sorting provided by the basic AI agent.
 */
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <tuple>

#include <getopt.h>

#include "Locale.hpp"
#include "Menu.hpp"
#include "AnsiCodes.hpp"
#include "Game.hpp"
#include "BasicAssist.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::pair;
using std::tuple;
using std::map;
using std::set;
using mugloar::Number;
using mugloar::Probability;

static void help()
{
	cerr << "No parameters expected" << endl;
}

static string risk_str(Probability p)
{
	string s = std::to_string(int(probability_risk(p) * 100 + 0.5)) + "%";
	switch (p) {
	case mugloar::PIECE_OF_CAKE:
	case mugloar::SURE_THING:
	case mugloar::WALK_IN_THE_PARK:
		return Green(s);
	case mugloar::QUITE_LIKELY:
	case mugloar::HMMM:
	case mugloar::GAMBLE:
		return Yellow(s);
	case mugloar::RISKY:
	case mugloar::RATHER_DETRIMENTAL:
		return Magenta(s);
	case mugloar::PLAYING_WITH_FIRE:
	case mugloar::SUICIDE_MISSION:
		return Red(s);
	case mugloar::IMPOSSIBLE:
		return Strong(Red(s));
	}
	/* Should not get here */
	return Blue(s);
}

int main(int argc, char *argv[])
{
	init_locale();

	char c;
	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case '?': help(); return 1;
		}
	}

	if (optind != argc) {
		help();
		return 1;
	}

	mugloar::Api api;

	bool quit = false;

	do {

		cerr << Strong("Game starting...") << endl;

		mugloar::Game game(api);

		cerr << Strong("Game started!") << endl << endl;

		/* Disable automatic reputation updates */
		game.autoupdate_reputation = false;

		bool end_game = false;

		while (!game.dead() && !end_game) {

			/* Print current status */

			cout << endl;
			cout << Strong(Blue(std::string(160, '='))) << endl;
			cout << endl;
			cout << Strong("Game: ") << Emph(game.id()) << "    \t";
			cout << Strong("Turn: ") << Emph(int(game.turn())) << "    \t";
			cout << Magenta("Lives: ") << Emph(int(game.lives())) << "    \t";
			cout << Yellow("Gold: ") << Emph(int(game.gold())) << "    \t";
			cout << Cyan("Level: ") << Emph(int(game.level())) << "    \t";
			cout << Green("Score: ") << Emph(int(game.score())) << " (top: " << game.high_score() << ")" << endl;
			cout << endl;
			cout << "Last known reputation: people=" << game.people_rep() << " state=" << game.state_rep() << " underworld=" << game.underworld_rep() << endl;
			cout << endl;

			/*
			 * Build owned-item histogram, sorted by cost (asc) then
			 * by name (asc)
			 */
			map<pair<Number, string>, int> histogram;
			for (const auto& [item, count] : game.own_items()) {
				histogram.try_emplace(make_pair(Number(item.cost), item.name), count);
			}

			/* Print owned-item histogram */
			cout << Strong("Purchased items:") << endl;
			for (const auto& [kv, count] : histogram) {
				const auto& [cost, name] = kv;
				cout << "  " << count << "x " << Emph(name) << " (" << Yellow(cost) << " gold each)" << endl;
			}
			cout << endl;

			/* Hotkey counter for menu items */
			char id = 'a';

			/* Build options menu */
			Menu options;

			options.emplace_back('#',
				"Query reputation",
				[&] () { game.update_reputation(); });

			/* Menu items for quests */
			for (const auto *pmsg : sort_messages(game)) {
				const auto& msg = *pmsg;
				stringstream ss;
				string expires_str = std::to_string(int(msg.expires_in));
				if (msg.expires_in == 1) {
					expires_str = Red(expires_str);
				} else if (msg.expires_in == 2) {
					expires_str = Yellow(expires_str);
				} else {
					expires_str = Cyan(expires_str);
				}
				ss << "Solve #" << msg.id
						<< " at " << risk_str(mugloar::lookup_probability(msg.probability)) << " confidence "
						<< "for " << Yellow(int(msg.reward)) << " gold "
						<< "(expires in " << expires_str << " turns)"
						<< ": " << Emph(msg.message);
				options.emplace_back(id,
					ss.str(),
					[&] () { game.solve_message(msg); });
				++id;
			}

			/* Menu items for purchasing from shop */
			auto sorted_items = sort_items(game);
			for (const auto& item : game.shop_items()) {
				stringstream ss;
				ss << "Buy item #" << item.id << " ";
				if (!sorted_items.empty() && sorted_items[0] == &item && item.cost <= game.gold()) {
					ss << Strong(Cyan(item.name));
				} else {
					ss << Cyan(item.name);
				}
				ss << " for " << Yellow(int(item.cost)) << " gold";
				options.emplace_back(id,
					ss.str(),
					[&] () { game.purchase_item(item); });
				++id;
			}

			options.emplace_back('!',
					"End game",
					[&] () { end_game = true; });

			/* Request choice from user and execute */

			menu(options)();

		}

		cout << endl;
		cout << "Game #" << game.id() << " finished at turn #" << game.turn() << " with score " << game.score() << " at level " << game.level() << endl;
		cout << endl;

		/* Play again? */
		menu({
			{ 'c', "Continue", [] () {} },
			{ 'q', "Quit", [&] () { quit = true; } }
		})();

	} while (!quit);
}
