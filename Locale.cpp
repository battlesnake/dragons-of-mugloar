#include <locale.h>

#include "Locale.hpp"

/* Default to en_US locale, mainly set for float/string conversion */
void init_locale()
{
	setlocale(LC_ALL, "en_US.utf8");
}
