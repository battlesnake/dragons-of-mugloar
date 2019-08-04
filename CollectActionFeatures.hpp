#pragma once
#include <unordered_set>
#include <string>
#include <vector>

#include "Game.hpp"

namespace mugloar {

/*
 * Extracts features of an action, that we want to use for training
 *
 * Currently:
 *  * individual words
 *  * pairs of consecutive words
 */
void extract_action_features(std::unordered_map<std::string, float>& features, const std::string& type, const std::string& description);

void extract_action_features(std::unordered_map<std::string, float>& features, const Message& message);

void extract_action_features(std::unordered_map<std::string, float>& features, const Item& item);

}
