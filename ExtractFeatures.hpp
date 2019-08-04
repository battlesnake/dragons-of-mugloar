#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

#include "Game.hpp"

namespace mugloar {

/*
 * Extracts features of an action, that we want to use for training
 *
 * Currently:
 *  * individual words
 *  * pairs of consecutive words
 */
void extract_action_features(std::unordered_map<std::string, float>& features, const std::string& type, const std::string& description);

void extract_action_features(std::unordered_map<std::string, float>& features, const Message& message);

void extract_action_features(std::unordered_map<std::string, float>& features, const Item& item);

struct GameStateDiff
{
	Number score;
	Number lives;
	Number gold;
	Number rep_people;
	Number rep_state;
	Number rep_underworld;
};

struct GameState
{
	Number score;
	Number lives;
	Number gold;
	Number rep_people;
	Number rep_state;
	Number rep_underworld;

	std::unordered_map<std::string, int> items;

	GameState() = default;
	GameState(const GameState&) = default;
	GameState& operator = (const GameState&) = default;

	GameState(const mugloar::Game& game);

	GameStateDiff operator - (const GameState& r) const;

};

void extract_game_state(std::unordered_map<std::string, float>& features, const GameState& state);
void extract_game_diff_state(std::unordered_map<std::string, float>& features, const GameStateDiff& state_diff);

}
