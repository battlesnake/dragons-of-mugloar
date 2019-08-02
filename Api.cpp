#include <sstream>
#include <stdexcept>
#include <iostream>

#include <cpr/cpr.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "parse_number.hpp"
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
	lives = parse_number(response["lives"].GetString());
	gold = parse_number(response["gold"].GetString());
	level = parse_number(response["level"].GetString());
	score = parse_number(response["score"].GetString());
	high_score = parse_number(response["highScore"].GetString());
	turn = parse_number(response["turn"].GetString());
}

void Api::investigate_reputation(const GameId& game_id, Number& people, Number& state, Number& underworld) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/investigate/reputation", response);
	people = parse_number(response["people"].GetString());
	state = parse_number(response["state"].GetString());
	underworld = parse_number(response["underworld"].GetString());
}

void Api::get_messages(const GameId& game_id, std::function<void(AdId, String, Number, Number)> consume_message) const
{
	rapidjson::Document response;
	execute_request(GET, base, "/" + game_id + "/messages", response);
	for (const auto& msg : response["messages"].GetArray()) {
		consume_message(
			msg["adId"].GetString(),
			msg["message"].GetString(),
			parse_number(msg["reward"].GetString()),
			parse_number(msg["expiresIn"].GetString())
			);
	}
}

void Api::solve_message(const GameId& game_id, const AdId& ad_id, bool& success, Number& lives, Number& gold, Number& score, Number& high_score, Number& turn, String& message) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/solve/" + ad_id, response);
	success = response["success"].GetBool();
	lives = parse_number(response["lives"].GetString());
	gold = parse_number(response["gold"].GetString());
	score = parse_number(response["score"].GetString());
	high_score = parse_number(response["highScore"].GetString());
	turn = parse_number(response["turn"].GetString());
	message = response["message"].GetString();
}

void Api::shop_list_items(const GameId& game_id, std::function<void(ItemId, String, Number)> consume_item) const
{
	rapidjson::Document response;
	execute_request(GET, base, "/" + game_id + "/shop", response);
	for (const auto& item : response["items"].GetArray()) {
		consume_item(
			item["id"].GetString(),
			item["name"].GetString(),
			parse_number(item["cost"].GetString())
			);
	}
}

void Api::shop_buy_item(const GameId& game_id, const ItemId& item_id, bool& success, Number& gold, Number& lives, Number& level, Number& turn) const
{
	rapidjson::Document response;
	execute_request(POST, base, "/" + game_id + "/shop/buy/" + item_id, response);
	/* BUG: API defines this as a string, but example is boolean */
	success = response["shoppingSuccess"].GetBool();
	gold = parse_number(response["gold"].GetString());
	lives = parse_number(response["lives"].GetString());
	level = parse_number(response["level"].GetString());
	turn = parse_number(response["turn"].GetString());
}

}
