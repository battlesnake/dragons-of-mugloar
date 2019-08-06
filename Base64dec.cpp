#include <iterator>
#include <basen.hpp>
#include "Base64dec.hpp"

namespace mugloar
{

/*
 * BUG:
 *
 * DANGER!
 * No validation!!
 *
 * Crap in, crap out (or possibly a crash).
 *
 * TODO: fix
 */
std::string b64dec(const std::string& in)
{
	std::string out;
	(void) bn::extract_partial_bits;
	(void) bn::extract_overlapping_bits;
	bn::decode_b64(in.begin(), in.end(), std::back_inserter(out));
	return out;
}

}
