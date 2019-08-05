#include <algorithm>
#include <tuple>

#include "BasicAssist.hpp"

using std::vector;
using std::sort;
using std::make_tuple;

namespace mugloar
{

/*
 * The ranker functions return tuples which we use in comparators for sorting.
 */

/*
 * Simple tuple sort
 */
static auto message_ranker(const Game& game, const Message& msg)
{
	/*
	 * Scale reward by risk
	 *
	 * Ordering:
	 *  * high (scaled) reward > low (scaled) reward
	 *  * high safety > low safety
	 *  * expires soon > expires later
	 *
	 * If we're down to the last life, pick the safest option, and rank
	 * second by reward.
	 *
	 * If we're taking an action with 1 life left, it means we can't afford
	 * a health potion, so we need a *safe* 50 gold.
	 */
	bool safe = game.lives() == 1;
	auto risk = probability_risk(lookup_probability(msg.probability));
	return make_tuple(
		safe ? -risk : -msg.reward * risk,
		safe ? -msg.reward : -risk,
		msg.expires_in,
		&msg);
}

/*
 * Filter and sort
 *
 * Recommend filtering out items which we can't afford (leaving sufficient hpot
 * reserve).
 *
 * Recommend against buying a powerup item if we have less of some other powerup
 * item.
 *
 * Only buy healing potion if we're down to the last life.
 */
static auto item_ranker(const Game& game, const Item& item)
{
	enum ItemType {
		HPOT,
		BASIC,
		ADVANCED,
		UNKNOWN
	} type;

	static constexpr auto HPOT_ID = "hpot";
	static constexpr auto HPOT_COST = 50;

	/* Is healing potion (+1 life) */
	if (item.id == HPOT_ID) {
		type = HPOT;
	} else if (item.cost == 100) {
		type = BASIC;
	} else if (item.cost == 300) {
		type = ADVANCED;
	} else {
		type = UNKNOWN;
	}

	/* How much can we afford to spend? (leaving reserve for HPOTs) */
	Number can_spend = game.gold();
	if (type != HPOT) {
		int reserve = 0;
		if (game.turn() > 150) {
			reserve = 7;
		} else if (game.turn() > 100) {
			reserve = 5;
		} else if (game.turn() > 60) {
			reserve = 2;
		} else if (game.lives() < 3) {
			reserve = 1;
		}
		can_spend -= reserve * HPOT_COST;
	}

	/* Can we afford it (leaving enough for hpots) */
	bool can_afford = item.cost <= can_spend;

	/* How many of this do we already own */
	auto& own_items = game.own_items();
	auto it = own_items.find(item);
	int have = it == own_items.end() ? 0 : it->second;

	/* Do we need a healing potion urgently? */
	bool need_hpot = game.lives() == 1;

	bool can_buy;
	switch (type) {
	case HPOT: can_buy = need_hpot; break;
	case BASIC: can_buy = game.turn() < 60; break;
	case ADVANCED: can_buy = true; break;
	default: can_buy = true;
	}

	can_buy = can_buy && can_afford;

	/*
	 * Ordering:
	 *  * we should buy > we shouldn't buy
	 *  * is hpot > is not hpot
	 *  * have less > have more
	 *  * cost more > cost less
	 */
	return make_tuple(!can_buy, type != HPOT, have, item.cost, &item);
}

vector<const Message *> sort_messages(const Game& game)
{
	/* Create rank table */
	vector<decltype(message_ranker(game, std::declval<Message>()))> tmp;
	tmp.reserve(game.messages().size());

	for (const auto& msg : game.messages()) {
		tmp.push_back(message_ranker(game, msg));
	}

	/* Sort rank table */
	sort(tmp.begin(), tmp.end());

	/* Build result */
	vector<const Message *> res;
	res.reserve(tmp.size());

	for (const auto& t : tmp) {
		res.push_back(std::get<3>(t));
	}

	return res;
}

vector<const Item *> sort_items(const Game& game)
{
	/* Create rank table */
	vector<decltype(item_ranker(game, std::declval<Item>()))> tmp;
	tmp.reserve(game.shop_items().size());

	for (const auto& item : game.shop_items()) {
		tmp.push_back(item_ranker(game, item));
	}

	/* Sort rank table */
	sort(tmp.begin(), tmp.end());

	/* Build result */
	vector<const Item *> res;
	res.reserve(tmp.size());

	/* Only add recommended items */
	for (const auto& t : tmp) {
		auto can_buy = ! std::get<0>(t);
		if (can_buy) {
			res.push_back(std::get<4>(t));
		}
	}

	return res;
}

}
