#include <vector>
#include <thread>
#include <functional>

#include "b64dec.hpp"
#include "rot13dec.hpp"
#include "Game.hpp"

using std::function;
using std::string;
using std::pair;

namespace mugloar {

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
