#include "CollectState.hpp"

namespace mugloar
{

void extract_game_state(std::unordered_map<std::string, float>& features, const GameState& state)
{
	features["game:score"] = state.score;
	features["game:lives"] = state.lives;
	features["game:gold"] = state.gold;
	features["game:rep_people"] = state.rep_people;
	features["game:rep_state"] = state.rep_state;
	features["game:rep_underworld"] = state.rep_underworld;
	for (const auto& [name, count] : state.items) {
		features["item:" + name] = count;
	}
}

void extract_game_diff_state(std::unordered_map<std::string, float>& features, const GameStateDiff& state_diff)
{
	features["diff:score"] = state_diff.score;
	features["diff:lives"] = state_diff.lives;
	features["diff:gold"] = state_diff.gold;
	features["diff:rep_people"] = state_diff.rep_people;
	features["diff:rep_state"] = state_diff.rep_state;
	features["diff:rep_underworld"] = state_diff.rep_underworld;
}

}
