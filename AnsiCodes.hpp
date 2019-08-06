#pragma once
/*
 * Hacky decorators for adding ANSI VT10x formatting to strings for printing to
 * terminal
 */
#include <string>

/* Wrap a string in ANSI VT100-style terminal codes */
template <int begin, int end>
class AnsiCodeWrap
{
	std::string s;

public:
	AnsiCodeWrap(const std::string& s) :
		s("\x1b[" + std::to_string(begin) + "m" + s + "\x1b[" + std::to_string(end) + "m") { }

	/*
	 * These constructors are somewhat hacky, sacrificing good C++ style for
	 * convenience with iostream usage.
	 */
	AnsiCodeWrap(const char *s) :
		AnsiCodeWrap(std::string(s)) { }

	AnsiCodeWrap(char c) :
		AnsiCodeWrap(std::string(1, c)) { }

	template <int begin2, int end2>
	AnsiCodeWrap(const AnsiCodeWrap<begin2, end2>& acw) :
		AnsiCodeWrap(std::string(acw)) { }

	template <typename T>
	AnsiCodeWrap(const T& value) :
		AnsiCodeWrap(std::to_string(value)) { }

	operator const std::string& () const { return s; }

};

template <int begin, int end>
std::ostream& operator << (std::ostream& os, const AnsiCodeWrap<begin, end>& acw)
{
	os << std::string(acw);
	return os;
}

/* VT10x codes which work on most Unix terminals */

using Strong = AnsiCodeWrap<1, 22>;
using Emph = AnsiCodeWrap<4, 24>;

template <int c>
using Color = AnsiCodeWrap<30 + c, 37>;

using Black = Color<0>;
using Red = Color<1>;
using Green = Color<2>;
using Yellow = Color<3>;
using Blue = Color<4>;
using Magenta = Color<5>;
using Cyan = Color<6>;
using White = Color<7>;
