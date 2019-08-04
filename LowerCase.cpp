#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include "LowerCase.hpp"

using std::string;

/* Unicode-aware conversion to lowercase */
string lowercase(const string& in)
{
	icu::UnicodeString us(in.c_str(), "UTF-8");
	us = us.toLower();
	string out;
	us.toUTF8String(out);
	return out;
}

