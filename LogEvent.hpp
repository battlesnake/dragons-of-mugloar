#pragma once
#include <ostream>
#include <unordered_map>
#include <string>

namespace mugloar
{

/* Emit event features to log file */
void log_event(std::ostream& f, const std::unordered_map<std::string, float>& entry);

}
