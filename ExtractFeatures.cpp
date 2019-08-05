#include "ExtractFeatures.hpp"
#include "LowerCase.hpp"

using std::string;
using std::string_view;
using std::vector;
using std::unordered_map;
using std::to_string;

/* Helper functions for decomposing text strings to words */
namespace detail
{

/* Emit ordered list of words found in string */
static void words_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]);
		bool strend = it + 1 == size;
		if (wordend) {
			if (it > begin) {
				res.emplace_back(sv.substr(begin, it - begin));
			}
			begin = it + 1;
		} else if (strend) {
			res.emplace_back(sv.substr(begin, size - begin));
		}
	}
}

/* Emit ordered list of adjacent pairs of words found in string */
static void word_pairs_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0, prebegin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]);
		bool strend = it + 1 == size;
		if (wordend) {
			if (it > begin) {
				if (prebegin < begin && begin < it) {
					res.emplace_back(sv.substr(prebegin, it - prebegin));
				}
				prebegin = begin;
			}
			begin = it + 1;
		} else if (strend) {
			if (prebegin < begin && begin < size) {
				res.emplace_back(sv.substr(prebegin, size - prebegin));
			}
		}
	}
}

}


namespace mugloar
{

void extract_action_features(unordered_map<string, float>& features, const string& type, const string& description)
{
	vector<string_view> name_words;
	name_words.reserve(1000);

	/* Get words from description */
	detail::words_of(name_words, description);

	/* Get word pairs from description */
	detail::word_pairs_of(name_words, description);

	/* Action type */
	features["action:" + type] = 1;

	/* Build feature set */
	for (const auto& w : name_words) {
		features[lowercase(string(w))] = 1;
	}
}

void extract_action_features(unordered_map<string, float>& features, const Message& message)
{
	extract_action_features(features, "solve", message.message);

	/* Cipher type */
	if (message.cipher == PLAIN) {
		features["cipher:none"] = 1;
	} else {
		features["cipher:" + to_string(int(message.cipher))] = 1;
	}

	/* Probability */
	features["probability:" + lowercase(message.probability)] = 1;
}

void extract_action_features(unordered_map<string, float>& features, const Item& item)
{
	extract_action_features(features, "buy", item.name);
}

void extract_game_state(std::unordered_map<std::string, float>& features, const GameState& state)
{
	features["game:score"] = state.score;
	features["game:lives"] = state.lives;
	features["game:gold"] = state.gold;
	features["game:level"] = state.level;
	features["game:rep_people"] = state.rep_people;
	features["game:rep_state"] = state.rep_state;
	features["game:rep_underworld"] = state.rep_underworld;
	features["game:turn"] = state.turn;
	for (const auto& [name, count] : state.items) {
		features["item:" + name] = count;
	}
	/* Boolean features for specific values */
	features["lives:" + to_string(int(state.lives))] = 1;
	features["level:" + to_string(int(state.level))] = 1;
	features["gold:50min=" + to_string(int(state.gold / 50) * 50)] = 1;
	features["turn:" + to_string(int(state.turn))] = 1;
}

void extract_game_diff_state(std::unordered_map<std::string, float>& features, const GameStateDiff& state_diff)
{
	features["diff:score"] = state_diff.score;
	features["diff:lives"] = state_diff.lives;
	features["diff:gold"] = state_diff.gold;
	features["diff:level"] = state_diff.level;
	features["diff:rep_people"] = state_diff.rep_people;
	features["diff:rep_state"] = state_diff.rep_state;
	features["diff:rep_underworld"] = state_diff.rep_underworld;
}

GameState::GameState(const Game& game) :
	score(game.score()),
	lives(game.lives()),
	gold(game.gold()),
	level(game.level()),
	rep_people(game.people_rep()),
	rep_state(game.state_rep()),
	rep_underworld(game.underworld_rep()),
	turn(game.turn())
{
}

GameStateDiff GameState::operator - (const GameState& r) const
{
	GameStateDiff res;
	res.score = score - r.score;
	res.lives = lives - r.lives;
	res.gold = gold - r.gold;
	res.level = level - r.level;
	res.rep_people = rep_people - r.rep_people;
	res.rep_state = rep_state - r.rep_state;
	res.rep_underworld = rep_underworld - r.rep_underworld;
	res.turn = turn - r.turn;
	return res;
}

}
