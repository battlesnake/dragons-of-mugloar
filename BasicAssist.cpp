#include <algorithm>
#include <tuple>
#include <string>
#include <vector>

#include "BasicAssist.hpp"

using std::vector;
using std::tuple;
using std::string;
using std::sort;
using std::make_tuple;

namespace mugloar
{

/* Healing potion item id */
static constexpr auto HPOT_ID = "hpot";

/* Healing potion cost */
static constexpr auto HPOT_COST = 50;

/* Estimated cost of wasting a turn (multiplied by turn number) */
static constexpr auto TURN_COST = 0.5f;

/*
 * Cap on level:turn threshold
 *
 * Reducing this below 2 allows us to spend a higher % of turns on growing score
 * instead of on spending gold.
 * Too close to 1 though, and we spend too many turns buying healing potions.
 *
 * A value of 1.2 results in us spending 20% of turns buying healing potions in
 * the late-game.
 *
 * A value of 1.4 results in us spending less than 1% of turns buying healing
 * potions in the late-game.
 */
static constexpr auto LEVEL_TURN_CAP = 1.3;

/* How much can we afford to spend? (leaving reserve for HPOTs) */
static Number can_spend(const Game& game)
{
	int reserve = 0;
	if (game.turn() > 200) {
		reserve = 5;
	} else if (game.turn() > 50) {
		reserve = 2;
	} else if (game.lives() < 3) {
		reserve = 1;
	}
	reserve -= game.lives() - 1;
	return game.gold() - std::max(0, reserve) * HPOT_COST;
}

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
	 *
	 * Subtract from reward the cost of healing potion weighted by risk of death.
	 *
	 * Also consider reputation risk, unless we're on our last life.
	 *
	 * Moves which push us over a cost boundary (50/100/300) get extra
	 * reward of potentially saving us a turn.
	 */
	bool safe = game.lives() == 1;
	auto risk = probability_risk(lookup_probability(msg.probability));
	auto hpot_loss = (1 - risk) * HPOT_COST;
	auto turn_loss = game.turn() * TURN_COST;
	/* Calculate risk due to reputation change */
	static const vector<tuple<string, float, float, float>> rep_changes {
		{ "Help ", +1, 0, 0 },
		// { "Help ", +0.3 /* to ? */, 0, 0 },
		// { "Help ", +0.1 /* to sell? */, 0, 0 },
		// { "Help ", +1.0 /* to write? */, 0, 0 },
		{ "Investigate ", -0.1, +1, 0 },
		{ "Create an advertisement ", +1, 0, 0 },
		{ "Escort ", +1, 0, 0 },
		// { "Escort ", 0.1, 0, 0 },
		{ "Rescue ", 0.1, 0, 0 },
		{ "Steal ", +1, -2, 0 },
		{ "Infiltrate ", 0, +2, -1 },
		/* Unknown */
		{ "Kill ", 0, 0, 0 },
	};
	tuple<string, float, float, float> rep_change { "None", 0, 0, 0 };
	for (const auto& t : rep_changes) {
		const auto& prefix = std::get<0>(t);
		if (msg.message.substr(0, prefix.size()) == prefix) {
			rep_change = t;
			break;
		}
	}
	const auto& [_prefix, r_p, r_s, r_u] = rep_change;
	auto change_l1 = r_p + r_s + r_u;
	auto rep_loss = (change_l1 < 0 ? 100 : 10) * -change_l1;

	/* Calculate bonus due to item cost boundary */
	auto gold = can_spend(game);
	auto cross = [&] (Number threshold) {
		return (gold < threshold) && (gold + msg.reward >= threshold);
	};
	auto cross_gain = cross(100) || cross(300) ? TURN_COST : 0;

	return make_tuple(
		safe ? -risk : -((msg.reward + cross_gain) * risk - hpot_loss - rep_loss - turn_loss),
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

	/* Can we afford it (leaving enough for hpots) */
	bool can_afford = item.cost <= (type == HPOT ? game.gold() : can_spend(game));

	/* How many of this do we already own */
	auto& own_items = game.own_items();
	auto it = own_items.find(item);
	int have = it == own_items.end() ? 0 : it->second;

	/* Do we need a healing potion urgently? */
	bool need_hpot = game.lives() == 1;

	bool can_buy;
	switch (type) {
	/*
	 * Don't spend turns on hpots unless we need them.  Instead, reserve
	 * enough cash to be able to buy several as needed (implemented in
	 * can_spend function).
	 */
	case HPOT: can_buy = need_hpot; break;
	/*
	 * Only buy one of each basic.  We need them to survive long enough to
	 * get onto the big items, but they are a pretty inefficient way to use
	 * a turn in the late game.
	 */
	case BASIC: can_buy = have == 0; break;
	/*
	 * Cap the level/turn ratio; buying stuff doesn't increase score so if
	 * we buy level-ups at every possible chance then our score grows too
	 * slowly.
	 *
	 * Settle for "almost invincible" (1.2 ratio) and rely on the fact that
	 * once we pass the point of insane growth, we're reserving enough gold
	 * for 5 health potions, so we're pretty tough against statistical
	 * anomalies too (regarding mission failures).
	 */
	case ADVANCED: can_buy = game.level() < game.turn() * LEVEL_TURN_CAP; break;
	/*
	 * Unknown mysterious items that we haven't come across yet?
	 */
	default: can_buy = true;
	}

	/* Pretty self-explanatory */
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
