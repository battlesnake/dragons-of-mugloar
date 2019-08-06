#pragma once
/*
 * Executive assistant.
 *
 * Orders messages in descending order of preference.
 *
 * Filters items based on whether they should be bought, and orders them in
 * descending order of preference.
 *
 * Currently, does not seem to get into the optimal scenario of "solve message,
 * buy 300-cost item, solve message, buy 300-cost item".
 *
 * This could possibly be improved by adding bonus preference points to any
 * message which would push us over 100 or 300 spendable gold (i.e. account for
 * the discrete nature of the game - 100 gold is far more useful than 99 gold).
 */
#include <vector>

#include "Game.hpp"

namespace mugloar
{

/* Recommendations (first is "best") */

std::vector<const Message *> sort_messages(const Game& game);

std::vector<const Item *> sort_items(const Game& game);

}
