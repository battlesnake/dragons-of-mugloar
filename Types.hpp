#pragma once
#include <string>

namespace mugloar {

/*
 * Alias string, in case we want to use a different (e.g. wide / smart-utf8)
 * string type in future
 */
using String = std::string;

/*
 * Range / precision not specified in the docs.
 * Will assume <int> for now.
 *
 * Ensure there's extra validation in input-parsing, in case we're wrong.
 */
using Number = int;

/* Surrogate ID data types */
using GameId = String;
using AdId = String;
using ItemId = String;

}
