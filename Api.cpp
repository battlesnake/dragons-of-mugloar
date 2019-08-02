#include <sstream>
#include <stdexcept>
#include <iostream>

#include <cpr/cpr.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "Api.hpp"

namespace mugloar {

enum Method {
	GET,
	POST
};

static void execute_request(Method method, const std::string& base, const std::string& path, rapidjson::Document& response)
{
	std::cerr << (method == GET ? "GET" : "POST") << " \t" << "... " << path << std::endl;

	cpr::Url url = base + path;

	auto r = method == GET ? cpr::Get(url) : cpr::Post(url);

	std::cerr << "Status: " << r.status_code << std::endl;

	if (r.status_code != 200) {
		throw std::runtime_error("HTTP code " + std::to_string(r.status_code));
	}

	std::cerr << "Body: " << std::endl << r.text << std::endl;

	response.Parse(r.text.c_str());

	std::cerr << std::endl;
}

Api::Api(std::string base) :
	base(base)
{
}

void Api::game_start(GameId& game_id, Number& lives, Number& gold, Number& level, Number& score, Number& high_score, Number& turn) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/game/start", response);
	game_id = response["gameId"].GetString();
	lives = response["lives"].GetInt64();
	gold = response["gold"].GetInt64();
	level = response["level"].GetInt64();
	score = response["score"].GetInt64();
	high_score = response["highScore"].GetInt64();
	turn = response["turn"].GetInt64();
}

void Api::investigate_reputation(const GameId& game_id, Number& people, Number& state, Number& underworld) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/investigate/reputation", response);
	people = response["people"].GetDouble();
	state = response["state"].GetDouble();
	underworld = response["underworld"].GetDouble();
}

void Api::get_messages(const GameId& game_id, std::function<void(AdId, String, Number, Number, String, bool)> consume_message) const
{
	rapidjson::Document response;
	execute_request(GET, base, "/" + game_id + "/messages", response);
	/* BUG: API defines root as object with "messages" array-member but example has the array as root */
	for (const auto& msg : response.GetArray()) {
		/*
		 * "Encrypted" field is null/1.
		 * Non-null indicates that the text fields are base64 encoded.
		 *
		 * This includes the also-undocumented "probability" fields.
		 */
		consume_message(
			msg["adId"].GetString(),
			msg["message"].GetString(),
			/* BUG: API defines this as string, but example is number */
			msg["reward"].GetInt64(),
			msg["expiresIn"].GetInt64(),
			msg["probability"].GetString(),
			msg.HasMember("encrypted") && !msg["encrypted"].IsNull()
			);
	}
}

void Api::solve_message(const GameId& game_id, const AdId& ad_id, bool& success, Number& lives, Number& gold, Number& score, Number& high_score, Number& turn, String& message) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/solve/" + ad_id, response);
	success = response["success"].GetBool();
	lives = response["lives"].GetInt64();
	gold = response["gold"].GetInt64();
	score = response["score"].GetInt64();
	high_score = response["highScore"].GetInt64();
	turn = response["turn"].GetInt64();
	message = response["message"].GetString();
}

void Api::shop_list_items(const GameId& game_id, std::function<void(ItemId, String, Number)> consume_item) const
{
	rapidjson::Document response;
	execute_request(GET, base, "/" + game_id + "/shop", response);
	/* BUG: API defines root as object with "items" array-member but example has the array as root */
	for (const auto& item : response.GetArray()) {
		consume_item(
			item["id"].GetString(),
			item["name"].GetString(),
			item["cost"].GetInt64()
			);
	}
}

void Api::shop_buy_item(const GameId& game_id, const ItemId& item_id, bool& success, Number& gold, Number& lives, Number& level, Number& turn) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/shop/buy/" + item_id, response);
	/* BUG: API defines this as a string, but example is boolean */
	success = response["shoppingSuccess"].GetBool();
	gold = response["gold"].GetInt64();
	lives = response["lives"].GetInt64();
	level = response["level"].GetInt64();
	turn = response["turn"].GetInt64();
}

}
