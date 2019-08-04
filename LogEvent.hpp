#pragma once
#include <fstream>
#include <unordered_map>
#include <string>

namespace mugloar
{

void log_event(std::ofstream& f, const std::unordered_map<std::string, float>& entry);

}
