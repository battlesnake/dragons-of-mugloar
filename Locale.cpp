#include "Locale.hpp"

void init_locale()
{
	setlocale(LC_ALL, "en_US.utf8");
}
