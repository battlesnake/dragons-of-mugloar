#include <vector>
#include <functional>
#include <stdexcept>
#include <limits>

#include "LowerCase.hpp"
#include "Base64dec.hpp"
#include "Rot13dec.hpp"
#include "Game.hpp"

using std::function;
using std::string;
using std::vector;
using std::pair;
using std::numeric_limits;
using std::optional;

namespace mugloar {

/* Risk values determined by machine learning, AKA "statistician but with higher salary" */
static vector<pair<string, float>> prob_map {
	{ "piece of cake", -20.4868 },
	{ "sure thing", -21.1751 },
	{ "walk in the park", -26.3083 },
	{ "quite likely", -30.4547 },
	{ "hmmm....", -35.0355 },
	{ "gamble", -37.837 },
	{ "risky", -45.8725 },
	{ "rather detrimental", -46.1147 },
	{ "playing with fire", -59.7092 },
	{ "suicide mission", -84.7348 },
	{ "impossible", -117.938 },
};

/* String to enum */
Probability lookup_probability(string name)
{
	name = lowercase(name);
	int p = 0;
	for (const auto& s : prob_map) {
		if (s.first == name) {
			return Probability(p);
		}
		p++;
	}
	throw std::runtime_error("Invalid probability: " + name);
}

/* Enum to string */
const string& reverse_lookup_probability(Probability p)
{
	if (p < 0 || p >= prob_map.size()) {
		throw std::runtime_error("Invalid probability value");
	}
	return prob_map[p].first;
}

/* Normalises in every call, inefficient!  But vs time for API calls, not noticeable */
float probability_risk(Probability p)
{
	if (p < 0 || p >= prob_map.size()) {
		throw std::runtime_error("Invalid probability value");
	}
	/* Find min/max risk */
	float m = numeric_limits<float>::infinity();
	float M = -numeric_limits<float>::infinity();
	for (const auto& [name, risk] : prob_map) {
		(void) name;
		m = std::min(m, risk);
		M = std::max(M, risk);
	}
	/* Normalise risk value to 0..1 range (1 is low risk, 0 is high risk) */
	float r = prob_map[p].second;
	return (r - m) / (M - m);
}

Game::Game(const Api& api, const optional<GameId>& id) :
	api(api)
{
	if (id) {
		_id = *id;
	} else {
		api.game_start(_id, _lives, _gold, _level, _score, _high_score, _turn);
	}
	autoupdate_reputation = false;
	turn_started();
	autoupdate_reputation = true;
}

void Game::turn_started()
{
	if (dead()) {
		return;
	}
	/* Reputation update advances a turn (or otherwise has some other annoying side-effect), do it before updating others */
	if (autoupdate_reputation) {
		internal_update_reputation();
	}
	update_items();
	update_messages();
}

/* Updates reputation (which costs a turn) but does call turn_started after */
void Game::internal_update_reputation()
{
	api.investigate_reputation(_id, _people_rep, _state_rep, _underworld_rep);
	_turn = _turn + 1;
}

/* Update reputation and advance client to next turn */
void Game::update_reputation()
{
	internal_update_reputation();
	turn_started();
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
		_own_items.try_emplace(item, 0).first->second++;
	}
	turn_started();
	return success;
}

}
