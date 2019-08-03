#pragma once
#include <tuple>
#include <unordered_map>
#include <string>
#include "Game.hpp"

namespace mugcollect {

using mugloar::Number;

/* Represents features of momentary game-state that we want to use for training */
struct State
{
	using StateVector = std::tuple<Number, Number, Number, Number, Number, Number>;

	StateVector vector;

	std::unordered_map<std::string, int> items;

	State(const mugloar::Game& game);

	State(const StateVector& v);

	State(const State& s) = default;
	State& operator = (const State& s) = default;

	/* Calculate piecewise difference between states */
	State operator - (const State& rhs) const;

};

}
