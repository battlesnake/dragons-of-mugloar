#pragma once
/*
 * Set locale to en_US.UTF-8, which should be available on most machines (unlike
 * the far superior en_GB.UTF-8...).
 *
 * This is mainly to standardise the string/float conversions.
 */

void init_locale();
