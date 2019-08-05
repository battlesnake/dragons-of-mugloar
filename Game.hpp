#pragma once
#include "Types.hpp"
#include "Api.hpp"
#include <vector>

namespace mugloar {

/* Forward declarations */

class Game;

struct Message;

struct Item;

enum Probability
{
	PIECE_OF_CAKE = 0,
	SURE_THING,
	WALK_IN_THE_PARK,
	QUITE_LIKELY,
	HMMM,
	GAMBLE,
	RISKY,
	RATHER_DETRIMENTAL,
	PLAYING_WITH_FIRE,
	SUICIDE_MISSION,
	IMPOSSIBLE
};

Probability lookup_probability(std::string name);
const std::string& reverse_lookup_probability(Probability p);
float probability_risk(Probability p);

/* Main class for a game instance */
class Game
{
	const Api& api;

	GameId _id;
	Number _lives = 0;
	Number _gold = 0;
	Number _level = 0;
	Number _score = 0;
	Number _high_score = 0;
	Number _turn = 0;

	Number _people_rep = 0;
	Number _state_rep = 0;
	Number _underworld_rep = 0;

	std::vector<Message> _messages;

	std::vector<Item> _shop_items;

	std::vector<Item> _own_items;

	void turn_started();
	void update_messages();
	void update_items();
	void internal_update_reputation();
public:
	Game(const Api& api);

	/* Getters */

	const GameId& id() const { return _id; }
	Number lives() const { return _lives; }
	Number gold() const { return _gold; }
	Number level() const { return _level; }
	Number score() const { return _score; }
	Number high_score() const { return _high_score; }
	Number turn() const { return _turn; }

	Number people_rep() const { return _people_rep; }
	Number state_rep() const { return _state_rep; }
	Number underworld_rep() const { return _underworld_rep; }

	const std::vector<Message>& messages() const { return _messages; }

	const std::vector<Item>& shop_items() const { return _shop_items; }

	const std::vector<Item>& own_items() const { return _own_items; }

	bool dead() const { return _lives == 0; }

	/* Auto-update reputation after each action? */
	bool autoupdate_reputation = true;

	/* Methods */

	std::pair<bool, String> solve_message(const Message& message);

	bool purchase_item(const Item& item);

	void update_reputation();
};

/* Message/advert */
struct Message
{
	AdId id;

	String message;

	/*
	 * Bad doc:
	 * "reward" is declared as "String" in doc, but example gives "Number".
	 *
	 * Ensure input-validation is tight.
	 */
	Number reward;

	/* Number of turns */
	Number expires_in;

	/*
	 * Undocumented:
	 * probability (String, possibly enum-string)
	 */
	String probability;

	/*
	 * Undocumented:
	 * encrypted (number, nullable)
	 */
	Format cipher;
};

/* Item in shop or in our rucksack */
struct Item
{
	ItemId id;

	String name;

	Number cost;
};

}
