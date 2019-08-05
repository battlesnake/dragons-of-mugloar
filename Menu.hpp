#pragma once
#include <vector>
#include <tuple>
#include <string>
#include <functional>

/* Action associated with a menu item */
using MenuAction = std::function<void()>;

/* Menu item (hotkey, title, action) */
using MenuItem = std::tuple<char, std::string, MenuAction>;

/* Menu */
using Menu = std::vector<MenuItem>;

/* Display a menu of choices and ask user to choose one, return chosen action */
const MenuAction& menu(const Menu& items);
