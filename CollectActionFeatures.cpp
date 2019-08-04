#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>

#include "CollectActionFeatures.hpp"

using std::string;
using std::string_view;
using std::vector;
using std::unordered_map;
using std::to_string;

/* Helper functions for decomposing text strings to words */
namespace detail {

/* Convert string to lowercase, assuming UTF-8 encoding */
static string lowercase(const string& in)
{
	icu::UnicodeString us(in.c_str(), "UTF-8");
	us = us.toLower();
	string out;
	us.toUTF8String(out);
	return out;
}

/* Emit ordered list of words found in string */
static void words_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]);
		bool strend = it + 1 == size;
		if (wordend) {
			if (it > begin) {
				res.emplace_back(sv.substr(begin, it - begin));
			}
			begin = it + 1;
		} else if (strend) {
			res.emplace_back(sv.substr(begin, size - begin));
		}
	}
}

/* Emit ordered list of adjacent pairs of words found in string */
static void word_pairs_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0, prebegin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]);
		bool strend = it + 1 == size;
		if (wordend) {
			if (it > begin) {
				if (prebegin < begin && begin < it) {
					res.emplace_back(sv.substr(prebegin, it - prebegin));
				}
				prebegin = begin;
			}
			begin = it + 1;
		} else if (strend) {
			if (prebegin < begin && begin < size) {
				res.emplace_back(sv.substr(prebegin, size - prebegin));
			}
		}
	}
}

}


namespace mugloar {

void extract_action_features(unordered_map<string, float>& features, const string& type, const string& description)
{
	vector<string_view> name_words;
	name_words.reserve(10000);
	detail::words_of(name_words, description);
	detail::word_pairs_of(name_words, description);

	features["action:" + type] = 1;

	for (const auto& w : name_words) {
		features[detail::lowercase(string(w))] = 1;
	}
}

void extract_action_features(unordered_map<string, float>& features, const Message& message)
{
	extract_action_features(features, "solve", message.message);
	if (message.cipher == PLAIN) {
		features["cipher:none"] = 1;
	} else {
		features["cipher:" + to_string(int(message.cipher))] = 1;
	}
	features["probability:" + detail::lowercase(message.probability)] = 1;
}

void extract_action_features(unordered_map<string, float>& features, const Item& item)
{
	extract_action_features(features, "buy", item.name);
}

}
