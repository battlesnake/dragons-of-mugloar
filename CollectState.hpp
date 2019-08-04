#pragma once
#include <tuple>
#include <unordered_map>
#include <string>
#include "Game.hpp"

namespace mugloar {

using mugloar::Number;

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

	GameState(const mugloar::Game& game) :
		score(game.score()),
		lives(game.lives()),
		gold(game.gold()),
		rep_people(game.people_rep()),
		rep_state(game.state_rep()),
		rep_underworld(game.underworld_rep())
	{
	}

	GameStateDiff operator - (const GameState& r) const
	{
		GameStateDiff res;
		res.score = score - r.score;
		res.lives = lives - r.lives;
		res.gold = gold - r.gold;
		res.rep_people = rep_people - r.rep_people;
		res.rep_state = rep_state - r.rep_state;
		res.rep_underworld = rep_underworld - r.rep_underworld;
		return res;
	}

};

void extract_game_state(std::unordered_map<std::string, float>& features, const GameState& state);
void extract_game_diff_state(std::unordered_map<std::string, float>& features, const GameStateDiff& state_diff);

}
