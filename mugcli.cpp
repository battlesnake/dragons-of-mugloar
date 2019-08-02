#include <iostream>
#include <string>

#include "Menu.hpp"
#include "AnsiCodes.hpp"
#include "Game.hpp"

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	using std::cerr;
	using std::cout;
	using std::endl;
	using std::to_string;

	bool quit = false;

	do {

		cerr << "Game starting..." << endl;

		mugloar::Api api;
		mugloar::Game game(api);

		cerr << "Game started!" << endl << endl;

		while (!game.dead()) {

			cout << endl;
			cout << Strong(Blue(std::string(40, '='))) << endl;
			cout << Strong("Turn: #") << game.turn() << endl;
			cout << Magenta("Lives: ") << game.lives() << endl;
			cout << Yellow("Gold: ") << game.gold() << endl;
			cout << Cyan("Level: ") << game.level() << endl;
			cout << Green("Score: ") << game.score() << " (top: " << game.high_score() << ")" << endl;
			cout << "Reputation: people=" << game.people_rep() << " state=" << game.state_rep() << " underworld=" << game.underworld_rep() << endl;
			cout << endl;

			cout << Strong("Purchased items:") << endl;
			for (const auto& item : game.own_items()) {
				cout << "  " << item.name << " (" << item.cost << ")" << endl;
			}
			cout << endl;

			char id = 'a';
			Menu options;

			cout << Strong("Messages:") << endl;
			for (const auto& msg : game.messages()) {
				cout << "  [" << id << "]  " << msg.message << endl;
				cout << "       Reward: " << msg.reward << endl;
				cout << "       Expires: " << msg.expires_in << endl;
				cout << endl;
				options.emplace_back(id,
					"Attempt quest for " + to_string(msg.reward) + " gold",
					[&] () { game.solve_message(msg); });
				++id;
			}
			cout << endl;

			cout << Strong("Available items in shop:") << endl;
			for (const auto& item : game.shop_items()) {
				cout << "  [" << id << "]  " << item.name << endl;
				cout << "       Cost: " << item.cost << endl;
				cout << endl;
				options.emplace_back(id,
					"Buy " + item.name + " for " + to_string(item.cost) + " gold",
					[&] () { game.purchase_item(item); });
				++id;
			}
			cout << endl;

			menu(options)();

		}

		menu({
			{ 'c', "Continue", [] () {} },
			{ 'q', "Quit", [&] () { quit = true; } }
		})();

	} while (!quit);
}
