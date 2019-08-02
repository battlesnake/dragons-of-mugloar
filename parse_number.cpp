#include <stdexcept>
#include "parse_number.hpp"

namespace mugloar {

/*
 * We do number parsing ourselves rather than using rapidjson GetInt64, as we
 * want to constrain the syntax even further.
 *
 * Specifically:
 *  * throw if radix/fractional
 *  * throw if scientific format
 *  * throw on out-of-range ("sane" range hardcoded in for now)
 *
 */
Number parse_number(const String& str)
{
	/* Strict input format + must consume entire string + no padding */
	/*
	 * We'll relax this or switch to standard-library depending on what
	 * undocumented formats we find from the API.
	 *
	 * For now, we're basically only accepting JSON-compatible integers.
	 */
	Number value = 0;
	bool negative = false;
	auto it = str.begin();
	auto end = str.end();

	if (it == end) {
		goto invalid_input;
	}

	if (*it == '-') {
		++it;
		negative = true;
	} else if (*it == '+') {
		++it;
		/* No explicit positive-sign for mantissa (as per JSON spec) */
		goto invalid_input;
	}

	/* No leading zeroes for non-zero integers (as per JSON spec) */
	if (*it == '0') {
		++it;
		if (it != end) {
			goto invalid_input;
		}
	}

	for (; it != end; ++it) {
		char c = *it;
		if (c < '0' && c > '9') {
			goto invalid_input;
		}
		value = value * 10 + (c - '0');

		/* Enforce magnitude range for now */
		if (value > 1'000'000'000) {
			goto invalid_input;
		}
	}

	return value * (negative ? -1 : +1);

invalid_input:
	throw std::runtime_error("Invalid number: <" + str + ">");
}

}
