#pragma once
/*
 * Log state of game and feature-sets to the event file, which can be analysed
 * in order to understand what works and what doesn't wokr (tactically).
 *
 * This log file is used by the machine-learning player's learner program.
 */
#include <ostream>
#include <unordered_map>
#include <string>

#include "Game.hpp"

namespace mugloar
{

/* Emit event features to log file */
void log_event(std::ostream& f, const Game& game, const std::unordered_map<std::string, float>& entry);

}
