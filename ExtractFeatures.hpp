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

/* Helper function for extracting features from SOLVE action */
void extract_action_features(std::unordered_map<std::string, float>& features, const Message& message);

/* Helper function for extracting features from BUY action */
void extract_action_features(std::unordered_map<std::string, float>& features, const Item& item);

/* Partial change between game states */
struct GameStateDiff
{
	Number score;
	Number lives;
	Number gold;
	Number level;
	Number rep_people;
	Number rep_state;
	Number rep_underworld;
	Number turn;
};

/* Partial state of game */
struct GameState
{
	Number score;
	Number lives;
	Number gold;
	Number level;
	Number rep_people;
	Number rep_state;
	Number rep_underworld;
	Number turn;

	std::unordered_map<std::string, int> items;

	GameState() = default;
	GameState(const GameState&) = default;
	GameState& operator = (const GameState&) = default;

	GameState(const mugloar::Game& game);

	GameStateDiff operator - (const GameState& r) const;

};

/* Extract feature-set from game state */
void extract_game_state(std::unordered_map<std::string, float>& features, const GameState& state);

/* Extract feature-set from game state diff */
void extract_game_diff_state(std::unordered_map<std::string, float>& features, const GameStateDiff& state_diff);

}
