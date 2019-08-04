#include "rot13dec.hpp"

namespace mugloar
{

/* ROT13 decoder(/encoder) */
std::string rot13dec(const std::string& in)
{
	std::string out { in };
	for (auto& c : out) {
		if (c >= 'A' && c <= 'M' || c >= 'a' && c <= 'm') {
			c += 13;
		} else if (c >= 'N' && c <= 'Z' || c >= 'n' && c <= 'z') {
			c -= 13;
		}
	}
	return out;
}

}
