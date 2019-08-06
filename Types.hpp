#pragma once
/*
 * Basic type aliases and definitions for the game
 */
#include <string>

namespace mugloar {

/*
 * Alias string, in case we want to use a different (e.g. wide / smart-utf8)
 * string type in future
 */
using String = std::string;

/*
 * Range / precision not specified in the docs.
 * Will assume <double> for now.
 *
 * I'm 99.999% certain that most fields (excluding reputation) are ints, and
 * that (reputation*10) could also be accurately stored as an int.  But since
 * the API calls are our bottleneck, we'll use double/float anyway.
 */
using Number = double;

/* Surrogate ID data types */
using GameId = String;
using AdId = String;
using ItemId = String;

/* String-field cipher types */
enum Format
{
	PLAIN,
	BASE64,
	ROT13
};

}
