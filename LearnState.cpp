#include "LearnState.hpp"

namespace detail {

/* Helper for implementing piecewise operations on tuples */
template <typename... Ts, std::size_t... Is, typename BinaryOp>
static inline std::tuple<Ts...> tuple_piecewise_helper(
	const std::tuple<Ts...>& l,
	const std::tuple<Ts...>& r,
	BinaryOp op,
	std::index_sequence<Is...>)
{
	return { op(std::get<Is>(l), std::get<Is>(r)) ... };
}

/* Piecewise operations on tuples */
template <typename... Ts, typename BinaryOp>
static inline std::tuple<Ts...> tuple_piecewise(
	const std::tuple<Ts...>& l,
	const std::tuple<Ts...>& r,
	BinaryOp op)
{
	return tuple_piecewise_helper(l, r, op, std::make_index_sequence<sizeof...(Ts)>{});
}

}


namespace mugcollect
{

State::State(const mugloar::Game& game) :
	vector(
		game.dead() ? 1 : 0,
		game.lives(),
		game.score(),
		game.people_rep(),
		game.state_rep(),
		game.underworld_rep())
{
	/* List of item IDs and counts */
	items.reserve(game.own_items().size());
	for (const auto& item : game.own_items()) {
		auto [it, ignore] = items.try_emplace(item.id, 0);
		it->second++;
	}
}

State::State(const StateVector& v) :
	vector(v)
{
}

/* Calculate piecewise difference between states */
State State::operator - (const State& rhs) const
{
	return detail::tuple_piecewise(vector, rhs.vector,
		[] (auto l, auto r) { return l - r; });
}

}
