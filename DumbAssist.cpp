#include <algorithm>
#include <tuple>

#include "DumbAssist.hpp"

using std::vector;
using std::sort;
using std::make_tuple;

namespace mugloar
{

/*
 * The ranker functions return tuples which we use in comparators for sorting.
 *
 * The tuples should really be pre-calculated once rather then being calculated
 * every time a comparator is invoked.
 *
 * Given inefficiencies elsewhere (e.g. in probability_risk function), these
 * inefficient loops wrapping more inefficient loops could really start to
 * create a noticeable slowdown when the nesting gets high enough!
 */

static auto message_ranker(const Message& msg)
{
	/*
	 * Ordering:
	 *  * high safety > low safety
	 *  * high reward > low reward
	 *  * expires soon > expires later
	 */
	return make_tuple(
		probability_risk(lookup_probability(msg.probability)),
		msg.reward,
		-msg.expires_in);
}

static constexpr auto HPOT_ID = "hpot";
static constexpr auto HPOT_COST = 50;

static auto item_ranker(const Game& game, const Item& item)
{
	/* Is healing potion (+1 life) */
	auto hpot = item.id == HPOT_ID;

	/* Fudge factor */
	bool prio = false;
	if (hpot) {
		if (game.turn() < 50) {
			if (game.lives() == 1) {
				prio = true;
			}
		} else if (game.turn() < 100) {
			if (game.lives() < 3) {
				prio = true;
			}
		} else if (game.lives() < 5) {
			prio = true;
		}
	}

	/* How much can we afford to spend? */
	Number can_spend = game.gold();
	if (!hpot) {
		if (game.turn() > 140) {
			can_spend -= 3 * HPOT_COST;
		} else if (game.turn() > 80) {
			can_spend -= 2 * HPOT_COST;
		} else if (game.turn() > 10) {
			can_spend -= HPOT_COST;
		}
	}

	/* Can we afford it (leaving enough for a healing potion */
	bool can_afford = item.cost <= can_spend;

	/* Which item (excluding hpot) do we have the least of? */
	auto& own_items = game.own_items();
	bool not_yet = false;
	if (!own_items.empty()) {
		auto fewest = own_items.begin();
		for (auto it = own_items.begin(), end = own_items.end(); it != end; ++it) {
			const auto& [own, count] = *it;
			if (own.id != HPOT_ID && count <= fewest->second) {
				/* If "item" is joint-least, tiebreak to "item" */
				if (count < fewest->second || own.id == item.id) {
					fewest = it;
				}
			}
		}
		not_yet = fewest->first.id != item.id;
	}

	/* How many do we already have? */
	auto it = own_items.find(item);
	int have = it == own_items.end() ? 0 : it->second;

	/* Do we advise to buy this at all? */
	bool recommend = prio && can_afford && !not_yet;

	/* Don't balance count of healing potions */
	if (hpot) {
		have = 1000;
	}

	/*
	 * Ordering:
	 *  * explicit priority > all others
	 *  * can afford > can't afford
	 *  * recommend > don't recommend
	 *  * own less > own more (hpot forced to lose this comparison)
	 *  * cheaper > costlier
	 */
	return make_tuple(prio, can_afford, recommend, -have, -item.cost);
}

vector<const Message *> sort_messages(const Game& game)
{
	vector<const Message *> res;
	res.reserve(game.messages().size());

	for (const auto& msg : game.messages()) {
		res.push_back(&msg);
	}

	sort(res.begin(), res.end(), [] (auto *a, auto *b) {
		return message_ranker(*a) > message_ranker(*b);
	});

	return res;
}

vector<const Item *> sort_items(const Game& game)
{
	vector<const Item *> res;
	res.reserve(game.shop_items().size());

	for (const auto& item : game.shop_items()) {
		res.push_back(&item);
	}

	sort(res.begin(), res.end(), [&] (auto *a, auto *b) {
		return item_ranker(game, *a) > item_ranker(game, *b);
	});

	return res;
}

}
