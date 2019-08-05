#include <cstdio>
#include <iostream>
#include <stdexcept>

#include "Menu.hpp"
#include "AnsiCodes.hpp"

/* Display single-choice menu and request choice from user */
const MenuAction& menu(const Menu& items)
{
	using std::cerr;
	using std::endl;
	cerr << Strong("Choices:") << endl;
	for (const auto& [key, title, ignore] : items) {
		cerr << "  [" << Strong(key) << "]  " << title << endl;
	}
	cerr << endl;
	cerr << Emph("Press key for desired option followed by <ENTER>: ");
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
		cerr << Red(Strong("Invalid choice:")) << "[" << Emph(c) << "]" << endl;
	} while (true);
}

