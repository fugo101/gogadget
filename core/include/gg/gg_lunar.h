/*
 * Vietnamese lunar calendar (âm lịch): Solar <-> Lunar dates, leap months, Can Chi and
 * solar terms (ADR-0015, docs/lunar.md).
 *
 * Integer-only and table-driven. tools/lunar/generate.py computes the tables from the JPL
 * DE440s ephemeris with Vietnam's rules: dates in UTC+8 before 1968 and UTC+7 from 1968.
 * The tables hold lunar years 1900-2100; conversions cover the Solar dates from Tết 1900
 * (1900-01-31) to 2100-12-31 and return false outside them or for a date that does not
 * exist.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define GG_LUNAR_FIRST_YEAR 1900
#define GG_LUNAR_LAST_YEAR 2100

typedef struct {
    int16_t year;  /* lunar year, numbered like the Gregorian year in which it begins */
    uint8_t month; /* 1-12 */
    uint8_t day;   /* 1-30 */
    bool leap;     /* the leap month, which repeats the number .month */
} gg_lunar_date_t;

/* The Lunar date of a Solar (Gregorian) date. */
bool gg_lunar_from_solar(int32_t year, uint8_t month, uint8_t day, gg_lunar_date_t *out);

/* The Solar date of a Lunar date. */
bool gg_lunar_to_solar(const gg_lunar_date_t *lunar, int32_t *year, uint8_t *month, uint8_t *day);

/* The month a lunar year repeats as its leap month, or 0 when it has none or lies outside
 * the tables. */
uint8_t gg_lunar_leap_month(int32_t lunar_year);

/* Days in a lunar month (29 or 30), or 0 when the year has no such month. */
uint8_t gg_lunar_month_days(int32_t lunar_year, uint8_t month, bool leap);

/*
 * Can Chi: a pair of a stem (Can: 0 Giáp, 1 Ất, 2 Bính, 3 Đinh, 4 Mậu, 5 Kỷ, 6 Canh,
 * 7 Tân, 8 Nhâm, 9 Quý) and a branch (Chi: 0 Tý, 1 Sửu, 2 Dần, 3 Mão, 4 Thìn, 5 Tỵ, 6 Ngọ,
 * 7 Mùi, 8 Thân, 9 Dậu, 10 Tuất, 11 Hợi). Both advance by one per year, month or day, so
 * the pairs repeat every 60.
 */
typedef struct {
    uint8_t stem;
    uint8_t branch;
} gg_can_chi_t;

/* Lunar year Y: stem (Y + 6) mod 10, branch (Y + 8) mod 12; year 4 was Giáp Tý and 2026
 * is Bính Ngọ. Defined for every int32_t year. */
gg_can_chi_t gg_lunar_can_chi_year(int32_t lunar_year);

/*
 * Month M of lunar year Y: branch (M + 1) mod 12, so month 1 is Dần and month 11 Tý. The
 * months count through the pairs 12 a year, so the stem is (12 Y + M + 3) mod 10, which
 * makes month 1 of a Giáp or Kỷ year Bính Dần. A leap month has no pair of its own and
 * reuses the one of the month it repeats, as Vietnamese almanacs print it. False unless
 * the year is in the tables and the month is 1-12.
 */
bool gg_lunar_can_chi_month(int32_t lunar_year, uint8_t month, gg_can_chi_t *out);

/* A day, given as its Solar date: with J its Julian Day Number, stem (J + 9) mod 10 and
 * branch (J + 1) mod 12; 2000-01-01 (J = 2451545) was Mậu Ngọ. False unless the date
 * exists and its year is GG_LUNAR_FIRST_YEAR-GG_LUNAR_LAST_YEAR (also 1-30 January 1900,
 * before the conversions' range). */
bool gg_lunar_can_chi_day(int32_t year, uint8_t month, uint8_t day, gg_can_chi_t *out);

/*
 * Solar terms (tiết khí): term k begins on the local date on which the Sun's apparent
 * longitude reaches 15k degrees, so 0 is Xuân phân (March equinox), 6 Hạ chí, 12 Thu phân
 * and 18 Đông chí (December solstice). Even terms are the principal terms (trung khí).
 */
#define GG_SOLAR_TERM_COUNT 24
#define GG_SOLAR_TERM_WINTER_SOLSTICE 18

typedef struct {
    uint8_t term;   /* the term in effect: the last one that began on or before the date */
    bool first_day; /* that term begins on this date */
} gg_solar_term_t;

/* The solar term of a Solar date (same range as the conversions). */
bool gg_lunar_solar_term(int32_t year, uint8_t month, uint8_t day, gg_solar_term_t *out);
