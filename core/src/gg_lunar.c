#include "gg/gg_lunar.h"

#include "gg/gg_civil.h"
#include "gg_lunar_data.h"

/* Julian Day Number of 1970-01-01, day 0 of gg_days_from_civil(). */
#define JDN_1970_01_01 2440588

static bool in_tables(int32_t lunar_year)
{
    return lunar_year >= GG_LUNAR_FIRST_YEAR && lunar_year <= GG_LUNAR_LAST_YEAR;
}

static uint32_t record(int32_t lunar_year)
{
    return gg_lunar_years[lunar_year - GG_LUNAR_FIRST_YEAR];
}

static uint8_t leap_of(uint32_t rec)
{
    return (uint8_t)((rec >> GG_LUNAR_LEAP_SHIFT) & GG_LUNAR_LEAP_MASK);
}

static uint8_t months_in(uint32_t rec)
{
    return leap_of(rec) != 0 ? 13 : 12;
}

/* Days in the month at a calendar-order position (0 = month 1). */
static uint8_t days_at(uint32_t rec, uint8_t pos)
{
    return (uint8_t)(29 + ((rec >> pos) & 1u));
}

/* Calendar-order position of a month, or -1 when the year has no such month. */
static int position(uint32_t rec, uint8_t month, bool leap)
{
    uint8_t lm = leap_of(rec);
    if (month < 1 || month > 12 || (leap && month != lm)) {
        return -1;
    }
    return month - 1 + (lm != 0 && (month > lm || leap));
}

/* Days since 1970-01-01 of Tết, day 1 of month 1. */
static int32_t tet(int32_t lunar_year)
{
    uint32_t offset = (record(lunar_year) >> GG_LUNAR_TET_SHIFT) & GG_LUNAR_TET_MASK;
    return gg_days_from_civil(lunar_year, 1, 1) + (int32_t)offset;
}

static int32_t last_day(void)
{
    return gg_days_from_civil(GG_LUNAR_LAST_YEAR, 12, 31);
}

/* Days since 1970-01-01 of a valid Solar date inside the supported range. */
static bool solar_days(int32_t year, uint8_t month, uint8_t day, int32_t *out)
{
    if (year < GG_LUNAR_FIRST_YEAR || year > GG_LUNAR_LAST_YEAR || month < 1 || month > 12 ||
        day < 1 || day > gg_days_in_month(year, month)) {
        return false;
    }
    *out = gg_days_from_civil(year, month, day);
    return *out >= tet(GG_LUNAR_FIRST_YEAR);
}

bool gg_lunar_from_solar(int32_t year, uint8_t month, uint8_t day, gg_lunar_date_t *out)
{
    int32_t z;
    if (!solar_days(year, month, day, &z)) {
        return false;
    }
    /* Days before Tết belong to the lunar year that began in the previous Gregorian year. */
    int32_t lunar_year = z < tet(year) ? year - 1 : year;
    uint32_t rec = record(lunar_year);
    uint8_t lm = leap_of(rec);
    int32_t offset = z - tet(lunar_year);
    for (uint8_t pos = 0; pos < months_in(rec); pos++) {
        uint8_t len = days_at(rec, pos);
        if (offset < len) {
            out->year = (int16_t)lunar_year;
            out->month = (uint8_t)(lm == 0 || pos < lm ? pos + 1 : pos);
            out->day = (uint8_t)(offset + 1);
            out->leap = lm != 0 && pos == lm;
            return true;
        }
        offset -= len;
    }
    return false; /* not reached: the next Tết comes first */
}

bool gg_lunar_to_solar(const gg_lunar_date_t *lunar, int32_t *year, uint8_t *month, uint8_t *day)
{
    if (!in_tables(lunar->year)) {
        return false;
    }
    uint32_t rec = record(lunar->year);
    int pos = position(rec, lunar->month, lunar->leap);
    if (pos < 0 || lunar->day < 1 || lunar->day > days_at(rec, (uint8_t)pos)) {
        return false;
    }
    int32_t z = tet(lunar->year) + lunar->day - 1;
    for (uint8_t i = 0; i < pos; i++) {
        z += days_at(rec, i);
    }
    if (z > last_day()) {
        return false;
    }
    gg_civil_t c;
    gg_civil_from_epoch((int64_t)z * 86400, 0, &c);
    *year = c.year;
    *month = c.month;
    *day = c.day;
    return true;
}

uint8_t gg_lunar_leap_month(int32_t lunar_year)
{
    return in_tables(lunar_year) ? leap_of(record(lunar_year)) : 0;
}

uint8_t gg_lunar_month_days(int32_t lunar_year, uint8_t month, bool leap)
{
    if (!in_tables(lunar_year)) {
        return 0;
    }
    uint32_t rec = record(lunar_year);
    int pos = position(rec, month, leap);
    return pos < 0 ? 0 : days_at(rec, (uint8_t)pos);
}

static uint8_t mod(int64_t a, uint8_t n)
{
    int64_t r = a % n;
    return (uint8_t)(r < 0 ? r + n : r);
}

gg_can_chi_t gg_lunar_can_chi_year(int32_t lunar_year)
{
    return (gg_can_chi_t){.stem = mod((int64_t)lunar_year + 6, 10),
                          .branch = mod((int64_t)lunar_year + 8, 12)};
}

gg_can_chi_t gg_lunar_can_chi_month(int32_t lunar_year, uint8_t month)
{
    return (gg_can_chi_t){.stem = mod(12 * (int64_t)lunar_year + month + 3, 10),
                          .branch = mod(month + 1, 12)};
}

gg_can_chi_t gg_lunar_can_chi_day(int32_t year, uint8_t month, uint8_t day)
{
    int64_t jdn = (int64_t)gg_days_from_civil(year, month, day) + JDN_1970_01_01;
    return (gg_can_chi_t){.stem = mod(jdn + 9, 10), .branch = mod(jdn + 1, 12)};
}

/* Day of month of the solar term at a date-order position of a Gregorian year. */
static uint8_t term_day(int32_t year, int pos)
{
    const uint8_t *row = gg_solar_terms[year - GG_LUNAR_FIRST_YEAR];
    return (uint8_t)(gg_solar_term_base[pos] + ((row[pos / 4] >> (2 * (pos % 4))) & 3u));
}

bool gg_lunar_solar_term(int32_t year, uint8_t month, uint8_t day, gg_solar_term_t *out)
{
    int32_t z;
    if (!solar_days(year, month, day, &z)) {
        return false;
    }
    /* Every Gregorian month holds two terms, at positions 2 (month - 1) and the next. */
    int first = 2 * (month - 1);
    int pos = first - 1; /* before both: the last term of the previous month */
    if (day >= term_day(year, first + 1)) {
        pos = first + 1;
    } else if (day >= term_day(year, first)) {
        pos = first;
    }
    out->first_day = pos >= first && day == term_day(year, pos);
    /* Early January follows Đông chí of the previous year (position 23, pos == -1). */
    out->term =
        (uint8_t)((pos + GG_SOLAR_TERM_COUNT + GG_SOLAR_TERM_FIRST_OF_YEAR) % GG_SOLAR_TERM_COUNT);
    return true;
}
