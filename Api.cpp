#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstdio>
#include <stdlib.h>

#include <cpr/cpr.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "Api.hpp"

using std::string;
using std::function;
using std::cerr;
using std::endl;
using std::runtime_error;
using std::to_string;
using std::printf;

namespace mugloar {

static bool debug_api = getenv("DEBUG_API") != nullptr;

enum Method {
	GET,
	POST
};

static void execute_request(Method method, const string& base, const string& path, rapidjson::Document& response)
{
	if (debug_api) {
		cerr << (method == GET ? "GET" : "POST") << " \t" << "... " << path << endl;
	}

	cpr::Url url = base + path;

	auto r = method == GET ? cpr::Get(url) : cpr::Post(url);

	if (debug_api) {
		cerr << "Status: " << r.status_code << endl;
	}

	if (r.status_code == 400) {
		rapidjson::Document err;
		err.Parse(r.text.c_str());
		throw runtime_error(string("Bad request: ") + err["error"].GetString());
	} else if (r.status_code == 410) {
		throw runtime_error("u ded");
	} else if (r.status_code == 502) {
		throw runtime_error("Bad Gateway");
	} else if (r.status_code != 200) {
		throw runtime_error("HTTP code " + to_string(r.status_code));
	}

	if (debug_api) {
		cerr << "Body: " << endl << r.text << endl;
	}

	response.Parse(r.text.c_str());

	if (debug_api) {
		cerr << endl;
	}
}

Api::Api(string base) :
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

void Api::get_messages(const GameId& game_id, function<void(AdId, String, Number, Number, String, Format)> consume_message) const
{
	rapidjson::Document response;
	execute_request(GET, base, "/" + game_id + "/messages", response);
	/* BUG: API defines root as object with "messages" array-member but example has the array as root */
	for (const auto& msg : response.GetArray()) {
		Format format = PLAIN;
		if (msg.HasMember("encrypted")) {
			const auto& n = msg["encrypted"];
			const char *message = msg["message"].GetString();
			if (n.IsNull()) {
				format = PLAIN;
			} else if (n.IsInt64() && n.GetInt64() == 1) {
				format = BASE64;
			} else {
				rapidjson::StringBuffer sb;
				rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
				n.Accept(writer);
				cerr << "UNSUPPORTED ENCRYPTION" << endl;
				cerr << " * spec: " << sb.GetString() << endl;
				cerr << " * value: " << endl;
				cerr << "     ";
				for (const char *p = message; *p; ++p) {
					fprintf(stderr, "%02hhx ", *p);
				}
				cerr << endl;

				throw runtime_error("Unsupported encryption");
			}
		}
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
			format
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

void Api::shop_list_items(const GameId& game_id, function<void(ItemId, String, Number)> consume_item) const
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
