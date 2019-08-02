#pragma once
#include <vector>
#include <tuple>
#include <string>
#include <functional>

using MenuAction = std::function<void()>;
using MenuItem = std::tuple<char, std::string, MenuAction>;
using Menu = std::vector<MenuItem>;

MenuAction menu(const Menu& items);
