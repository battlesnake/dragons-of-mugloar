#pragma once
#include <string>

/* Wrap a string in ANSI VT100-style terminal codes */
template <int begin, int end>
class AnsiCodeWrap
{
	std::string s;

public:
	AnsiCodeWrap(std::string s) :
		s("\x1b[" + std::to_string(begin) + "m" + s + "\x1b[" + std::to_string(end) + "m") { }
	operator std::string () const { return s; }

};

template <int begin, int end>
std::ostream& operator << (std::ostream& os, const AnsiCodeWrap<begin, end>& acw)
{
	os << std::string(acw);
	return os;
}

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
