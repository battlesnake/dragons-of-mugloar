#include <cstdio>
#include <iostream>
#include <stdexcept>

#include "Menu.hpp"

MenuAction menu(const Menu& items)
{
	using std::cerr;
	using std::endl;
	cerr << "Choices:" << endl;
	for (const auto& [key, title, ignore] : items) {
		cerr << "  [" << key << "]  " << title << endl;
	}
	do {
		char c = getchar();
		if (c == EOF) {
			throw std::runtime_error("Unexpected end-of-input");
		}
		if (isspace(c)) {
			continue;
		}
		for (const auto& [key, ignore, action] : items) {
			if (key == c) {
				cerr << endl;
				return action;
			}
		}
		cerr << "Invalid choice: [" << c << "]" << endl;
	} while (true);
}

