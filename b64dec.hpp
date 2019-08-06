#pragma once
/*
 * Base-64 decoder
 *
 * TODO:
 * Doesn't do any input validation, will probably go UB on invalid input.
 */
#include <string>

namespace mugloar
{

/* Base-64 decoder (see definition for warning notes and bug) */
std::string b64dec(const std::string& in);

}
