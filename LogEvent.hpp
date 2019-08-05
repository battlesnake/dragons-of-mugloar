#pragma once
#include <ostream>
#include <unordered_map>
#include <string>

#include "Game.hpp"

namespace mugloar
{

/* Emit event features to log file */
void log_event(std::ostream& f, const Game& game, const std::unordered_map<std::string, float>& entry);

}
