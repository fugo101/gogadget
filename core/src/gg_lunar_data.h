/*
 * The tables behind gg_lunar.c. tools/lunar/generate.py writes them into gg_lunar_data.c
 * (docs/lunar.md); this header fixes their layout.
 */
#pragma once

#include <stdint.h>

#include "gg/gg_lunar.h"

#define GG_LUNAR_YEAR_COUNT (GG_LUNAR_LAST_YEAR - GG_LUNAR_FIRST_YEAR + 1)

/*
 * One record per lunar year, from GG_LUNAR_FIRST_YEAR:
 *   bits  0-12  month lengths in calendar order, the leap month right after the month it
 *               repeats: 1 = 30 days, 0 = 29 days (bit 12 is 0 in a year of 12 months)
 *   bits 13-16  the leap month: 0 = none, otherwise the number of the month it repeats
 *   bits 17-22  Tết: days from 1 January of the Gregorian year with the same number to
 *               day 1 of month 1
 */
#define GG_LUNAR_LEAP_SHIFT 13
#define GG_LUNAR_LEAP_MASK 0xfu
#define GG_LUNAR_TET_SHIFT 17
#define GG_LUNAR_TET_MASK 0x3fu
extern const uint32_t gg_lunar_years[GG_LUNAR_YEAR_COUNT];

/*
 * Solar terms per Gregorian year, from GG_LUNAR_FIRST_YEAR, in date order. Position p
 * (0 = Tiểu hàn, about 6 January) is term (p + GG_SOLAR_TERM_FIRST_OF_YEAR) % 24 and falls
 * in month p / 2 + 1, on day gg_solar_term_base[p] plus a 2-bit offset held in bits
 * 2 * (p % 4) of byte p / 4 of the year's row.
 */
#define GG_SOLAR_TERM_FIRST_OF_YEAR 19
extern const uint8_t gg_solar_term_base[GG_SOLAR_TERM_COUNT];
extern const uint8_t gg_solar_terms[GG_LUNAR_YEAR_COUNT][GG_SOLAR_TERM_COUNT / 4];
