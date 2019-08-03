#include "CollectActionFeatures.hpp"

using std::string;
using std::string_view;
using std::vector;

/* Helper functions for decomposing text strings to words */
namespace detail {

/* Emit ordered list of words found in string */
static void words_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]) || it + 1 == size;
		if (wordend) {
			if (it > begin) {
				res.emplace_back(sv.substr(begin, it - begin));
			}
			begin = it + 1;
		}
	}
}

/* Emit ordered list of adjacent pairs of words found in string */
static void word_pairs_of(vector<string_view>& res, const string_view& sv)
{
	const auto size = sv.size();
	for (size_t it = 0, begin = 0, prebegin = 0; it < size; ++it) {
		bool wordend = isspace(sv[it]) || it + 1 == size;
		if (wordend) {
			if (it > begin) {
				if (prebegin < begin && begin < it) {
					res.emplace_back(sv.substr(prebegin, it - prebegin));
				}
				prebegin = begin;
			}
			begin = it + 1;
		}
	}
}

}


namespace mugcollect {

ActionFeatures::ActionFeatures(const string& type, const string& name, const std::vector<string>& other)
{
	vector<string_view> name_words;
	words.reserve(100);
	detail::words_of(name_words, name);
	detail::word_pairs_of(name_words, name);

	words.emplace(type);

	for (const auto& w : name_words) {
		words.emplace(std::string(w));
	}

	for (auto& w : other) {
		words.emplace(std::move(w));
	}
}

}
