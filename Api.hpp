#pragma once
#include <functional>
#include <string>
#include "Types.hpp"

namespace mugloar {

/* String-field cipher types */
enum Format
{
	PLAIN,
	BASE64,
	ROT13
};

/* Makes calls to API endpoints and returns parsed response fields */
class Api
{
	std::string base;

public:

	Api(std::string base = "https://dragonsofmugloar.com/api/v2");

	void game_start(GameId& game_id, Number& lives, Number& gold, Number& level, Number& score, Number& high_score, Number& turn) const;

	void investigate_reputation(const GameId& game_id, Number& people, Number& state, Number& underworld) const;

	void get_messages(const GameId& game_id, std::function<void(AdId, String, Number, Number, String, Format)> consume_message) const;

	void solve_message(const GameId& game_id, const AdId& ad_id, bool& success, Number& lives, Number& gold, Number& score, Number& high_score, Number& turn, String& message) const;

	void shop_list_items(const GameId& game_id, std::function<void(ItemId, String, Number)> consume_item) const;

	void shop_buy_item(const GameId& game_id, const ItemId& item_id, bool& success, Number& gold, Number& lives, Number& level, Number& turn) const;

};

}
