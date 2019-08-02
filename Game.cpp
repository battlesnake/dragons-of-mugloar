#include "parse_number.hpp"
#include "Game.hpp"

namespace mugloar {

Game::Game(const Api& api) :
	api(api)
{
	api.game_start(_id, _lives, _gold, _level, _score, _high_score, _turn);
	turn_started();
}

void Game::turn_started()
{
	update_reputation();
	update_messages();
	update_items();
}

void Game::update_reputation()
{
	api.investigate_reputation(_id, _people_rep, _state_rep, _underworld_rep);
}

void Game::update_messages()
{
	_messages.clear();
	api.get_messages(_id, [this] (AdId ad_id, String message, Number reward, Number expires_in) {
		_messages.push_back({
			std::move(ad_id),
			std::move(message),
			reward,
			expires_in
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

std::pair<bool, String> Game::solve_message(const Message& message)
{
	bool success;
	String explanation;
	api.solve_message(_id, message.id, success, _lives, _gold, _score, _high_score, _turn, explanation);
	return std::make_pair(success, std::move(explanation));
}

bool Game::purchase_item(const Item& item)
{
	bool success;
	api.shop_buy_item(_id, item.id, success, _gold, _lives, _level, _turn);
	if (success) {
		_own_items.emplace_back(item);
	}
	return success;
}

}
