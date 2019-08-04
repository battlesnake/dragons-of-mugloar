#include <vector>
#include <functional>
#include <stdexcept>

#include "LowerCase.hpp"
#include "b64dec.hpp"
#include "rot13dec.hpp"
#include "Game.hpp"

using std::function;
using std::string;
using std::pair;

namespace mugloar {

static std::vector<string> prob_map {
	/* Score: -20.4868 */
	{ "piece of cake" },
	/* Score: -21.1751 */
	{ "sure thing" },
	/* Score: -26.3083 */
	{ "walk in the park" },
	/* Score: -30.4547 */
	{ "quite likely" },
	/* Score: -35.0355 */
	{ "hmmm...." },
	/* Score: -37.837 */
	{ "gamble" },
	/* Score: -45.8725 */
	{ "risky" },
	/* Score: -46.1147 */
	{ "rather detrimental" },
	/* Score: -59.7092 */
	{ "playing with fire" },
	/* Score: -84.7348 */
	{ "suicide mission" },
	/* Score: -117.938 */
	{ "impossible" },
};

/* String to enum */
Probability lookup_probability(std::string name)
{
	name = lowercase(name);
	Probability p = Probability(0);
	for (const auto& s : prob_map) {
		if (s == name) {
			return p;
		}
	}
	throw std::runtime_error("Invalid probability: " + name);
}

/* Enum to string */
const std::string& reverse_lookup_probability(Probability p)
{
	if (p >= 0 && p < prob_map.size()) {
		return prob_map[p];
	} else {
		throw std::runtime_error("Invalid probability value");
	}
}

Game::Game(const Api& api) :
	api(api)
{
	api.game_start(_id, _lives, _gold, _level, _score, _high_score, _turn);
	turn_started();
}

void Game::turn_started()
{
	if (dead()) {
		return;
	}
	/* Reputation update advances a turn (or otherwise has some other annoying side-effect), do it before updating others */
	update_reputation();
	update_items();
	update_messages();
}

void Game::update_reputation()
{
	api.investigate_reputation(_id, _people_rep, _state_rep, _underworld_rep);
}

void Game::update_messages()
{
	_messages.clear();
	api.get_messages(_id, [this] (AdId ad_id, String message, Number reward, Number expires_in, String probability, Format format) {
		function<string(const string&)> decoder = nullptr;
		switch (format) {
		case PLAIN: decoder = [] (const string& s) { return s; }; break;
		case BASE64: decoder = b64dec; break;
		case ROT13: decoder = rot13dec; break;
		}
		ad_id = decoder(ad_id);
		message = decoder(message);
		probability = decoder(probability);
		_messages.push_back({
			std::move(ad_id),
			std::move(message),
			reward,
			expires_in,
			probability,
			format
			});
	});
}

void Game::update_items()
{
	_shop_items.clear();
	api.shop_list_items(_id, [this] (ItemId item_id, String name, Number cost) {
		_shop_items.push_back({
			std::move(item_id),
			std::move(name),
			cost
			});
	});
}

pair<bool, String> Game::solve_message(const Message& message)
{
	bool success;
	String explanation;
	api.solve_message(_id, message.id, success, _lives, _gold, _score, _high_score, _turn, explanation);
	turn_started();
	return std::make_pair(success, std::move(explanation));
}

bool Game::purchase_item(const Item& item)
{
	bool success;
	api.shop_buy_item(_id, item.id, success, _gold, _lives, _level, _turn);
	if (success) {
		_own_items.emplace_back(item);
	}
	turn_started();
	return success;
}

}
