#pragma once
#include <vector>

#include "Game.hpp"

namespace mugloar
{

/* Recommendations (first is "best") */

std::vector<const Message *> sort_messages(const Game& game);

std::vector<const Item *> sort_items(const Game& game);

}
