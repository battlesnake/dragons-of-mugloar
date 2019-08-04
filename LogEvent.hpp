#pragma once
#include <ostream>
#include <unordered_map>
#include <string>

namespace mugloar
{

void log_event(std::ostream& f, const std::unordered_map<std::string, float>& entry);

}
