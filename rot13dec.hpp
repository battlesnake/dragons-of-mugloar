#pragma once
/*
 * ROT13 decoder (can also be used as encoder since ROT13 is self-inverse).
 */
#include <string>

namespace mugloar
{

/* ROT13 decoder(/encoder) */
std::string rot13dec(const std::string& in);

}
