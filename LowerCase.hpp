#pragma once
/*
 * Unicode-aware (UTF-8 specifically) function to convert strings to lowercase)
 */
#include <string>

/* Convert string to lowercase, assuming UTF-8 encoding */
std::string lowercase(const std::string& in);
