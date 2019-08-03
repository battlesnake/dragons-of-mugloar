#pragma once
#include <unordered_set>
#include <string>
#include <vector>

namespace mugcollect {

/* Represents features of an action, that we want to use for training */
struct ActionFeatures
{
	/* Set of words and word-pairs */
	std::unordered_set<std::string> words;

	ActionFeatures(const std::string& type, const std::string& name, const std::vector<std::string>& other);

};

}
