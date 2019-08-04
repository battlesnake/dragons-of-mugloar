#pragma once
#include <vector>
#include <tuple>
#include <string>
#include <functional>

using MenuAction = std::function<void()>;
using MenuItem = std::tuple<char, std::string, MenuAction>;
using Menu = std::vector<MenuItem>;

/* Display a menu of choices and ask user to choose one */
const MenuAction& menu(const Menu& items);
