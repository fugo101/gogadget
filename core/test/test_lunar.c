/*
 * Lunar calendar (ADR-0015, docs/lunar.md): check values, the Vietnam-vs-China cases, leap
 * and knife-edge months, an exhaustive round trip, the calendar rules, and the tables
 * against the generator's month list and the Hong Kong Observatory's (HKO) tables.
 */
#include <stdio.h>
#include <string.h>

#include "gg/gg_civil.h"
#include "gg/gg_lunar.h"
#include "gg/gg_types.h"
#include "unity.h"

#ifndef GG_TEST_DATA_DIR
#error "sim/CMakeLists.txt defines GG_TEST_DATA_DIR"
#endif

/* Can and Chi indices, as in gg_lunar.h. */
enum { GIAP, AT, BINH, DINH, MAU, KY, CANH, TAN, NHAM, QUY };
enum { TY, SUU, DAN, MAO, THIN, TI, NGO, MUI, THAN, DAU, TUAT, HOI };

/* Solar terms used below, by apparent solar longitude / 15 degrees. */
enum {
    XUAN_PHAN = 0,
    LAP_HA = 3,
    DAI_THU = 8,
    LAP_THU = 9,
    HAN_LO = 13,
    DONG_CHI = 18,
    VU_THUY = 22,
    KINH_TRAP = 23,
};

void setUp(void)
{
}

void tearDown(void)
{
}

/* ---- Helpers ------------------------------------------------------------------- */

static int32_t first_day(void)
{
    return gg_days_from_civil(1900, 1, 31); /* Tết 1900 */
}

static int32_t last_day(void)
{
    return gg_days_from_civil(2100, 12, 31);
}

static void civil_of(int32_t days, int32_t *y, uint8_t *m, uint8_t *d)
{
    gg_civil_t c;
    gg_civil_from_epoch((int64_t)days * 86400, 0, &c);
    *y = c.year;
    *m = c.month;
    *d = c.day;
}

static void iso(char *buf, size_t len, int32_t days)
{
    int32_t y;
    uint8_t m, d;
    civil_of(days, &y, &m, &d);
    snprintf(buf, len, "%04d-%02u-%02u", (int)y, m, d);
}

/* The Solar date of day 1 of a lunar month as YYYY-MM-DD, or "none". */
static const char *month_start(int16_t year, uint8_t month, bool leap)
{
    static char buf[16];
    gg_lunar_date_t l = {.year = year, .month = month, .day = 1, .leap = leap};
    int32_t y;
    uint8_t m, d;
    if (!gg_lunar_to_solar(&l, &y, &m, &d)) {
        return "none";
    }
    snprintf(buf, sizeof(buf), "%04d-%02u-%02u", (int)y, m, d);
    return buf;
}

static int32_t month_start_days(int16_t year, uint8_t month, bool leap)
{
    gg_lunar_date_t l = {.year = year, .month = month, .day = 1, .leap = leap};
    int32_t y;
    uint8_t m, d;
    TEST_ASSERT_TRUE(gg_lunar_to_solar(&l, &y, &m, &d));
    return gg_days_from_civil(y, m, d);
}

static void assert_lunar(int16_t year, uint8_t month, uint8_t day, bool leap, int32_t sy,
                         uint8_t sm, uint8_t sd)
{
    gg_lunar_date_t l;
    TEST_ASSERT_TRUE(gg_lunar_from_solar(sy, sm, sd, &l));
    TEST_ASSERT_EQUAL_INT16(year, l.year);
    TEST_ASSERT_EQUAL_UINT8(month, l.month);
    TEST_ASSERT_EQUAL_UINT8(day, l.day);
    TEST_ASSERT_EQUAL(leap, l.leap);
}

static void assert_can_chi(uint8_t stem, uint8_t branch, gg_can_chi_t cc)
{
    TEST_ASSERT_EQUAL_UINT8(stem, cc.stem);
    TEST_ASSERT_EQUAL_UINT8(branch, cc.branch);
}

static void assert_term_begins(uint8_t term, int32_t y, uint8_t m, uint8_t d)
{
    gg_solar_term_t t;
    TEST_ASSERT_TRUE(gg_lunar_solar_term(y, m, d, &t));
    TEST_ASSERT_EQUAL_UINT8(term, t.term);
    TEST_ASSERT_TRUE(t.first_day);
}

static void fail_at(int32_t days, const char *what)
{
    char msg[96], date[16];
    iso(date, sizeof(date), days);
    snprintf(msg, sizeof(msg), "%s: %s", date, what);
    TEST_FAIL_MESSAGE(msg);
}

/* ---- Check values and known cases ---------------------------------------------- */

static void test_check_value(void)
{
    /* ADR-0015: 2026-10-20 is day 11 of month 9 of Bính Ngọ, a Đinh Mão day of a Mậu Tuất
     * month. */
    assert_lunar(2026, 9, 11, false, 2026, 10, 20);
    assert_can_chi(BINH, NGO, gg_lunar_can_chi_year(2026));
    assert_can_chi(MAU, TUAT, gg_lunar_can_chi_month(2026, 9));
    assert_can_chi(DINH, MAO, gg_lunar_can_chi_day(2026, 10, 20));
}

static void test_tet_where_vietnam_and_china_differ(void)
{
    /* New moons between 23:00 and 24:00 in Vietnam (UTC+7), after midnight in China
     * (UTC+8). China's dates (HKO) are in the comments. */
    TEST_ASSERT_EQUAL_STRING("1968-01-29", month_start(1968, 1, false)); /* 1968-01-30 */
    TEST_ASSERT_EQUAL_STRING("1969-02-16", month_start(1969, 1, false)); /* 1969-02-17 */
    TEST_ASSERT_EQUAL_STRING("1985-01-21", month_start(1985, 1, false)); /* 1985-02-20 */
    TEST_ASSERT_EQUAL_STRING("2007-02-17", month_start(2007, 1, false)); /* 2007-02-18 */
    TEST_ASSERT_EQUAL_STRING("2030-02-02", month_start(2030, 1, false)); /* 2030-02-03 */
    TEST_ASSERT_EQUAL_STRING("2053-02-18", month_start(2053, 1, false)); /* 2053-02-19 */
    TEST_ASSERT_EQUAL_STRING("2026-02-17", month_start(2026, 1, false)); /* the same */
    /* Trung Thu (15/8) 2034: China 2034-09-27. */
    assert_lunar(2034, 8, 15, false, 2034, 9, 26);
}

static void test_leap_months(void)
{
    /* 1984-85: China has a leap month 10 from 1984-11-23 because Đông chí fell on 22/12
     * in UTC+8, Vietnam a leap month 2 from 1985-03-21. */
    TEST_ASSERT_EQUAL_UINT8(0, gg_lunar_leap_month(1984));
    TEST_ASSERT_EQUAL_STRING("1984-11-23", month_start(1984, 11, false));
    TEST_ASSERT_EQUAL_UINT8(2, gg_lunar_leap_month(1985));
    TEST_ASSERT_EQUAL_STRING("1985-03-21", month_start(1985, 2, true));
    /* 1987: Vietnam leap 7 from 1987-08-24, China leap 6 from 1987-07-26. */
    TEST_ASSERT_EQUAL_UINT8(7, gg_lunar_leap_month(1987));
    TEST_ASSERT_EQUAL_STRING("1987-07-26", month_start(1987, 7, false));
    TEST_ASSERT_EQUAL_STRING("1987-08-24", month_start(1987, 7, true));
    /* The same leap month a day earlier than China's (1995-09-25, 2031-04-22). */
    TEST_ASSERT_EQUAL_UINT8(8, gg_lunar_leap_month(1995));
    TEST_ASSERT_EQUAL_STRING("1995-09-24", month_start(1995, 8, true));
    TEST_ASSERT_EQUAL_UINT8(3, gg_lunar_leap_month(2031));
    TEST_ASSERT_EQUAL_STRING("2031-04-21", month_start(2031, 3, true));
    /* 2033: leap 11 in both calendars, the only leap 11 or 12 in 1900-2100. */
    TEST_ASSERT_EQUAL_UINT8(11, gg_lunar_leap_month(2033));
    TEST_ASSERT_EQUAL_STRING("2033-12-22", month_start(2033, 11, true));
    TEST_ASSERT_EQUAL_STRING("2034-01-20", month_start(2033, 12, false));
    /* 2025 (the leap-month golden screenshot): leap 6 from 2025-07-25, 29 days. */
    TEST_ASSERT_EQUAL_UINT8(6, gg_lunar_leap_month(2025));
    TEST_ASSERT_EQUAL_STRING("2025-07-25", month_start(2025, 6, true));
    TEST_ASSERT_EQUAL_UINT8(29, gg_lunar_month_days(2025, 6, true));
    assert_lunar(2025, 6, 3, true, 2025, 7, 27);
    /* A leap month exists only where the table has one. */
    TEST_ASSERT_EQUAL_UINT8(0, gg_lunar_leap_month(2026));
    TEST_ASSERT_EQUAL_STRING("none", month_start(2026, 6, true));
    TEST_ASSERT_EQUAL_UINT8(0, gg_lunar_month_days(2026, 6, true));
}

static void test_knife_edge_months(void)
{
    /* New moons within 76 s of Hanoi midnight: the month starts on the date of the JPL
     * DE440s instant, in UTC with no leap second after the last announced one (policy in
     * docs/lunar.md). Hồ Ngọc Đức's widely used table or algorithm gives the other date. */
    TEST_ASSERT_EQUAL_STRING("2054-05-08", month_start(2054, 4, false));  /* 00:01:00.64, +60.6 s */
    TEST_ASSERT_EQUAL_STRING("2072-12-09", month_start(2072, 11, false)); /* 23:59:56.45, -3.6 s */
    TEST_ASSERT_EQUAL_STRING("2077-11-16", month_start(2077, 10, false)); /* 00:01:14.59, +74.6 s */
    TEST_ASSERT_EQUAL_STRING("2085-10-19", month_start(2085, 9, false));  /* 00:01:15.94, +75.9 s */
}

static void test_zone_switch(void)
{
    /* UTC+7 from 1968-01-01 (docs/lunar.md, Time). The nearest new moons whose date
     * depends on the zone: 1967 month 11 (00:09:39 on 2/12 in UTC+8, 23:09:39 on 1/12 in
     * UTC+7) and Tết 1968 (00:29:10 on 30/1 in UTC+8, 23:29:10 on 29/1 in UTC+7). A
     * switch on the decree date (1967-08-08) would start month 11 on 1967-12-01. */
    TEST_ASSERT_EQUAL_STRING("1967-12-02", month_start(1967, 11, false));
    TEST_ASSERT_EQUAL_STRING("1968-01-29", month_start(1968, 1, false));
}

static void test_knife_edge_solar_terms(void)
{
    /* Solar terms within 25 s of local midnight (UTC+7, or UTC+8 before 1968). */
    assert_term_begins(DAI_THU, 2038, 7, 22);   /* 23:59:56.12, -3.9 s */
    assert_term_begins(LAP_THU, 2060, 8, 6);    /* 23:59:37.79, -22.2 s */
    assert_term_begins(HAN_LO, 2098, 10, 7);    /* 23:59:50.09, -9.9 s */
    assert_term_begins(LAP_HA, 1911, 5, 7);     /* 00:00:16.02, +16.0 s */
    assert_term_begins(VU_THUY, 1923, 2, 19);   /* 23:59:40.31, -19.7 s */
    assert_term_begins(DONG_CHI, 1951, 12, 23); /* 00:00:01.53 in UT1, +1.5 s; HKO agrees.
                                                 * TAI - 10 s as "UTC" would give 22/12. */
}

static void test_solar_terms(void)
{
    gg_solar_term_t t;
    assert_term_begins(XUAN_PHAN, 2026, 3, 20);
    TEST_ASSERT_TRUE(gg_lunar_solar_term(2026, 3, 21, &t));
    TEST_ASSERT_EQUAL_UINT8(XUAN_PHAN, t.term);
    TEST_ASSERT_FALSE(t.first_day);
    TEST_ASSERT_TRUE(gg_lunar_solar_term(2026, 3, 19, &t));
    TEST_ASSERT_EQUAL_UINT8(KINH_TRAP, t.term);
    /* Early January follows the previous year's Đông chí. */
    TEST_ASSERT_TRUE(gg_lunar_solar_term(2026, 1, 1, &t));
    TEST_ASSERT_EQUAL_UINT8(DONG_CHI, t.term);
    TEST_ASSERT_FALSE(t.first_day);
    assert_term_begins(DONG_CHI, 2026, 12, 22);
    TEST_ASSERT_FALSE(gg_lunar_solar_term(1900, 1, 30, &t));
    TEST_ASSERT_FALSE(gg_lunar_solar_term(2101, 1, 1, &t));
    TEST_ASSERT_FALSE(gg_lunar_solar_term(2026, 2, 29, &t));
}

static void test_can_chi(void)
{
    assert_can_chi(MAU, NGO, gg_lunar_can_chi_day(2000, 1, 1));    /* JDN 2451545 */
    assert_can_chi(DINH, MAO, gg_lunar_can_chi_day(2026, 12, 19)); /* 60 days after the check */
    assert_can_chi(GIAP, TY, gg_lunar_can_chi_year(1984));
    assert_can_chi(CANH, TY, gg_lunar_can_chi_year(1900));
    assert_can_chi(CANH, THAN, gg_lunar_can_chi_year(2100));
    /* Month 1 of a Giáp or Kỷ year is Bính Dần, of an Ất year Mậu Dần; month 11 is Tý. */
    assert_can_chi(BINH, DAN, gg_lunar_can_chi_month(1984, 1));
    assert_can_chi(BINH, DAN, gg_lunar_can_chi_month(1989, 1));
    assert_can_chi(MAU, DAN, gg_lunar_can_chi_month(1985, 1));
    TEST_ASSERT_EQUAL_UINT8(TY, gg_lunar_can_chi_month(2026, 11).branch);
    /* The months run on across the new year: Tân Sửu, then Nhâm Dần. */
    assert_can_chi(TAN, SUU, gg_lunar_can_chi_month(2026, 12));
    assert_can_chi(NHAM, DAN, gg_lunar_can_chi_month(2027, 1));
    /* The leap 6th month of 2025 reuses month 6's Quý Mùi. */
    assert_can_chi(QUY, MUI, gg_lunar_can_chi_month(2025, 6));
}

static void test_range_edges(void)
{
    gg_lunar_date_t l;
    int32_t y;
    uint8_t m, d;
    TEST_ASSERT_FALSE(gg_lunar_from_solar(1900, 1, 30, &l));
    assert_lunar(1900, 1, 1, false, 1900, 1, 31);
    assert_lunar(2100, 12, 1, false, 2100, 12, 31);
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2101, 1, 1, &l));
    TEST_ASSERT_FALSE(gg_lunar_from_solar(1899, 12, 31, &l));
    /* Lunar dates outside the range, or that do not exist. */
    l = (gg_lunar_date_t){.year = 1899, .month = 12, .day = 1};
    TEST_ASSERT_FALSE(gg_lunar_to_solar(&l, &y, &m, &d));
    l = (gg_lunar_date_t){.year = 2100, .month = 12, .day = 2}; /* 2101-01-01 */
    TEST_ASSERT_FALSE(gg_lunar_to_solar(&l, &y, &m, &d));
    l = (gg_lunar_date_t){.year = 2026, .month = 13, .day = 1};
    TEST_ASSERT_FALSE(gg_lunar_to_solar(&l, &y, &m, &d));
    l = (gg_lunar_date_t){.year = 2026, .month = 9, .day = 0};
    TEST_ASSERT_FALSE(gg_lunar_to_solar(&l, &y, &m, &d));
    l = (gg_lunar_date_t){.year = 2025, .month = 6, .day = 30, .leap = true}; /* 29 days */
    TEST_ASSERT_FALSE(gg_lunar_to_solar(&l, &y, &m, &d));
    /* Solar dates that do not exist. */
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2026, 2, 29, &l));
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2026, 4, 31, &l));
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2026, 0, 1, &l));
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2026, 13, 1, &l));
    TEST_ASSERT_FALSE(gg_lunar_from_solar(2026, 1, 0, &l));
    TEST_ASSERT_TRUE(gg_lunar_from_solar(2024, 2, 29, &l));
    TEST_ASSERT_EQUAL_UINT8(0, gg_lunar_leap_month(1899));
    TEST_ASSERT_EQUAL_UINT8(0, gg_lunar_month_days(2101, 1, false));
}

/* ---- Every day ----------------------------------------------------------------- */

static void test_round_trip_every_day(void)
{
    gg_lunar_date_t prev = {0};
    for (int32_t z = first_day(); z <= last_day(); z++) {
        int32_t y, y2;
        uint8_t m, d, m2, d2;
        gg_lunar_date_t l;
        civil_of(z, &y, &m, &d);
        if (!gg_lunar_from_solar(y, m, d, &l)) {
            fail_at(z, "no Lunar date");
        }
        if (!gg_lunar_to_solar(&l, &y2, &m2, &d2) || gg_days_from_civil(y2, m2, d2) != z) {
            fail_at(z, "does not round-trip");
        }
        if (z == first_day()) {
            TEST_ASSERT_TRUE(l.year == 1900 && l.month == 1 && l.day == 1 && !l.leap);
        } else if (l.day != 1) {
            if (l.year != prev.year || l.month != prev.month || l.leap != prev.leap ||
                l.day != prev.day + 1) {
                fail_at(z, "does not follow the day before");
            }
        } else {
            bool full = prev.day == gg_lunar_month_days(prev.year, prev.month, prev.leap);
            bool leap_next = l.leap && !prev.leap && l.month == prev.month && l.year == prev.year;
            bool month_next =
                !l.leap && ((l.year == prev.year && l.month == prev.month + 1) ||
                            (l.year == prev.year + 1 && l.month == 1 && prev.month == 12));
            if (!full || !(leap_next || month_next)) {
                fail_at(z, "starts a month out of order");
            }
        }
        prev = l;
    }
}

/* ---- The rules (docs/lunar.md) ------------------------------------------------- */

static void test_months_and_years_have_valid_lengths(void)
{
    for (int16_t y = GG_LUNAR_FIRST_YEAR; y <= GG_LUNAR_LAST_YEAR; y++) {
        uint8_t leap = gg_lunar_leap_month(y);
        TEST_ASSERT_TRUE(leap <= 12);
        uint32_t days = 0;
        for (uint8_t m = 1; m <= 12; m++) {
            uint8_t n = gg_lunar_month_days(y, m, false);
            TEST_ASSERT_TRUE(n == 29 || n == 30);
            days += n;
            uint8_t nl = gg_lunar_month_days(y, m, true);
            TEST_ASSERT_TRUE(m == leap ? nl == 29 || nl == 30 : nl == 0);
            days += nl;
        }
        /* 12 months: 353-355 days; 13 months: 383-385. */
        TEST_ASSERT_TRUE(leap ? days >= 383 && days <= 385 : days >= 353 && days <= 355);
        if (y < GG_LUNAR_LAST_YEAR) {
            TEST_ASSERT_EQUAL_INT32(
                month_start_days((int16_t)(y + 1), 1, false) - month_start_days(y, 1, false), days);
        }
    }
}

static void test_month_11_holds_the_winter_solstice(void)
{
    for (int32_t y = GG_LUNAR_FIRST_YEAR; y <= GG_LUNAR_LAST_YEAR; y++) {
        int found = 0;
        for (uint8_t d = 1; d <= 31; d++) {
            gg_solar_term_t t;
            TEST_ASSERT_TRUE(gg_lunar_solar_term(y, 12, d, &t));
            if (t.first_day && t.term == GG_SOLAR_TERM_WINTER_SOLSTICE) {
                gg_lunar_date_t l;
                TEST_ASSERT_TRUE(gg_lunar_from_solar(y, 12, d, &l));
                TEST_ASSERT_EQUAL_INT16(y, l.year);
                TEST_ASSERT_EQUAL_UINT8(11, l.month);
                TEST_ASSERT_FALSE(l.leap);
                found++;
            }
        }
        TEST_ASSERT_EQUAL_INT(1, found);
    }
}

static void test_leap_month_has_no_principal_term(void)
{
    int leap_years = 0;
    for (int16_t y = GG_LUNAR_FIRST_YEAR; y <= GG_LUNAR_LAST_YEAR; y++) {
        uint8_t leap = gg_lunar_leap_month(y);
        if (leap == 0) {
            continue;
        }
        leap_years++;
        int32_t start = month_start_days(y, leap, true);
        for (int32_t z = start; z < start + gg_lunar_month_days(y, leap, true); z++) {
            int32_t sy;
            uint8_t sm, sd;
            gg_solar_term_t t;
            civil_of(z, &sy, &sm, &sd);
            TEST_ASSERT_TRUE(gg_lunar_solar_term(sy, sm, sd, &t));
            if (t.first_day && t.term % 2 == 0) {
                fail_at(z, "a principal term in a leap month");
            }
        }
    }
    TEST_ASSERT_EQUAL_INT(74, leap_years); /* tools/lunar/generate.py reports the same */
}

static void test_solar_terms_follow_each_other(void)
{
    gg_solar_term_t prev;
    int32_t prev_start = 0;
    int starts = 0;
    TEST_ASSERT_TRUE(gg_lunar_solar_term(1900, 1, 31, &prev));
    for (int32_t z = first_day() + 1; z <= last_day(); z++) {
        int32_t y;
        uint8_t m, d;
        gg_solar_term_t t;
        civil_of(z, &y, &m, &d);
        TEST_ASSERT_TRUE(gg_lunar_solar_term(y, m, d, &t));
        if (!t.first_day) {
            if (t.term != prev.term) {
                fail_at(z, "the term changes without a first day");
            }
        } else {
            /* Terms are 14 to 16 days apart. */
            if (t.term != (prev.term + 1) % GG_SOLAR_TERM_COUNT ||
                (starts > 0 && (z - prev_start < 14 || z - prev_start > 16))) {
                fail_at(z, "a term out of order");
            }
            prev_start = z;
            starts++;
        }
        prev = t;
    }
    TEST_ASSERT_EQUAL_INT(201 * 24 - 2, starts); /* from 1900-02-04 to 2100-12-22 */
}

/* ---- Data files ---------------------------------------------------------------- */

typedef struct {
    int16_t year;
    uint8_t month;
    bool leap;
    int32_t start; /* days since 1970-01-01 */
    uint8_t length;
} month_row_t;

#define MAX_MONTHS 2600

static FILE *open_data(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s%s", GG_TEST_DATA_DIR, name);
    FILE *f = fopen(path, "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, path);
    return f;
}

/* The next data row: skips comments and the header line. */
static bool next_row(FILE *f, char *line, int len)
{
    while (fgets(line, len, f) != NULL) {
        if (line[0] >= '0' && line[0] <= '9') {
            return true;
        }
    }
    return false;
}

/* A reason generate.py writes for a month the time zone explains (not "HKO differs"). */
static bool explained_month(const char *reason)
{
    static const char *const forms[] = {
        "starts earlier: ", "starts later: ", "ends earlier: ", "ends later: ", "HKO numbers it "};
    for (size_t i = 0; i < GG_ARRAY_LEN(forms); i++) {
        if (strncmp(reason, forms[i], strlen(forms[i])) == 0) {
            return true;
        }
    }
    return false;
}

static bool same_label(const month_row_t *a, int16_t year, uint8_t month, bool leap)
{
    return a->year == year && a->month == month && a->leap == leap;
}

static void label_text(char *buf, size_t len, int16_t year, uint8_t month, bool leap)
{
    snprintf(buf, len, "%d %s%u", year, leap ? "leap " : "", month);
}

static void test_matches_generator_month_list(void)
{
    FILE *f = open_data("lunar_months.csv");
    char line[256];
    int rows = 0, leaps = 0;
    while (next_row(f, line, sizeof(line))) {
        int year, month, leap, y, m, d, length;
        TEST_ASSERT_EQUAL_INT(
            7, sscanf(line, "%d,%d,%d,%d-%d-%d,%d", &year, &month, &leap, &y, &m, &d, &length));
        int32_t start = gg_days_from_civil(y, (uint32_t)m, (uint32_t)d);
        char want[16];
        iso(want, sizeof(want), start);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(start <= last_day() ? want : "none",
                                         month_start((int16_t)year, (uint8_t)month, leap), line);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(length, gg_lunar_month_days(year, (uint8_t)month, leap),
                                        line);
        rows++;
        leaps += leap;
    }
    fclose(f);
    TEST_ASSERT_EQUAL_INT(201 * 12 + 74, rows);
    TEST_ASSERT_EQUAL_INT(74, leaps);
}

static size_t load_hko(month_row_t *rows)
{
    FILE *f = open_data("hko_month_starts.csv");
    char line[256];
    size_t n = 0;
    while (next_row(f, line, sizeof(line))) {
        int year, month, leap, y, m, d;
        TEST_ASSERT_EQUAL_INT(6,
                              sscanf(line, "%d,%d,%d,%d-%d-%d", &year, &month, &leap, &y, &m, &d));
        TEST_ASSERT_TRUE(n < MAX_MONTHS);
        rows[n++] = (month_row_t){.year = (int16_t)year,
                                  .month = (uint8_t)month,
                                  .leap = leap != 0,
                                  .start = gg_days_from_civil(y, (uint32_t)m, (uint32_t)d)};
    }
    fclose(f);
    return n;
}

/*
 * Day by day from the start of the month in progress on 1901-01-01 (HKO's first file
 * begins there) to 2100-12-31: every month in which some day gets another lunar date
 * than in HKO's Chinese calendar must be listed in hko_differences.csv, with the reason,
 * and every listed month must differ.
 */
static void test_matches_hko_except_listed_differences(void)
{
    static month_row_t hko[MAX_MONTHS], found[128];
    size_t hko_count = load_hko(hko), found_count = 0;
    TEST_ASSERT_EQUAL(2475, hko_count);
    TEST_ASSERT_TRUE(hko[0].start <= gg_days_from_civil(1901, 1, 1)); /* all of 1901 */

    size_t i = 0;
    for (int32_t z = hko[0].start; z <= last_day(); z++) {
        while (i + 1 < hko_count && hko[i + 1].start <= z) {
            i++;
        }
        int32_t y;
        uint8_t m, d;
        gg_lunar_date_t l;
        civil_of(z, &y, &m, &d);
        TEST_ASSERT_TRUE(gg_lunar_from_solar(y, m, d, &l));
        if (same_label(&hko[i], l.year, l.month, l.leap) && l.day == z - hko[i].start + 1) {
            continue;
        }
        if (found_count == 0 || !same_label(&found[found_count - 1], l.year, l.month, l.leap)) {
            TEST_ASSERT_TRUE(found_count < GG_ARRAY_LEN(found));
            found[found_count++] = (month_row_t){.year = l.year, .month = l.month, .leap = l.leap};
        }
    }

    FILE *f = open_data("hko_differences.csv");
    char line[512];
    size_t listed = 0;
    while (next_row(f, line, sizeof(line))) {
        int year, month, leap, y, m, d, reason = 0;
        char hko_start[16];
        TEST_ASSERT_EQUAL_INT_MESSAGE(7,
                                      sscanf(line, "%d,%d,%d,%d-%d-%d,%15[^,],%n", &year, &month,
                                             &leap, &y, &m, &d, hko_start, &reason),
                                      line);
        TEST_ASSERT_TRUE_MESSAGE(reason > 0 && explained_month(line + reason), line);
        char want[32], got[32];
        label_text(want, sizeof(want), (int16_t)year, (uint8_t)month, leap);
        if (listed >= found_count) {
            TEST_FAIL_MESSAGE("hko_differences.csv lists a month that does not differ");
        }
        label_text(got, sizeof(got), found[listed].year, found[listed].month, found[listed].leap);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(want, got, "first unlisted or stale difference");

        /* The start dates in the file are right, here and in HKO's table. */
        char start[16];
        snprintf(start, sizeof(start), "%04d-%02d-%02d", y, m, d);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(start, month_start((int16_t)year, (uint8_t)month, leap),
                                         line);
        const char *theirs = "-";
        char theirs_buf[16];
        for (size_t k = 0; k < hko_count; k++) {
            if (same_label(&hko[k], (int16_t)year, (uint8_t)month, leap)) {
                iso(theirs_buf, sizeof(theirs_buf), hko[k].start);
                theirs = theirs_buf;
            }
        }
        TEST_ASSERT_EQUAL_STRING_MESSAGE(theirs, hko_start, line);
        listed++;
    }
    fclose(f);
    TEST_ASSERT_EQUAL_MESSAGE(found_count, listed, "differences missing from hko_differences.csv");
    TEST_ASSERT_EQUAL(68, listed); /* docs/lunar.md explains them; a new count needs a reason */
}

/* The date of every solar term the API reports, by Gregorian year and term number, or
 * NO_TERM. */
#define NO_TERM INT32_MIN

static void our_terms(int32_t terms[][GG_SOLAR_TERM_COUNT])
{
    for (int y = 0; y <= GG_LUNAR_LAST_YEAR - GG_LUNAR_FIRST_YEAR; y++) {
        for (int k = 0; k < GG_SOLAR_TERM_COUNT; k++) {
            terms[y][k] = NO_TERM;
        }
    }
    for (int32_t z = first_day(); z <= last_day(); z++) {
        int32_t y;
        uint8_t m, d;
        gg_solar_term_t t;
        civil_of(z, &y, &m, &d);
        TEST_ASSERT_TRUE(gg_lunar_solar_term(y, m, d, &t));
        if (t.first_day) {
            TEST_ASSERT_EQUAL_INT32(NO_TERM, terms[y - GG_LUNAR_FIRST_YEAR][t.term]);
            terms[y - GG_LUNAR_FIRST_YEAR][t.term] = z;
        }
    }
}

/* Every solar term of 1900-2100 in the generator's list, date by date. */
static void test_matches_generator_term_list(void)
{
    static int32_t ours[GG_LUNAR_LAST_YEAR - GG_LUNAR_FIRST_YEAR + 1][GG_SOLAR_TERM_COUNT];
    our_terms(ours);
    FILE *f = open_data("lunar_terms.csv");
    char line[128];
    int rows = 0, before_range = 0;
    while (next_row(f, line, sizeof(line))) {
        int y, m, d, term;
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sscanf(line, "%d-%d-%d,%d", &y, &m, &d, &term), line);
        TEST_ASSERT_TRUE_MESSAGE(term >= 0 && term < GG_SOLAR_TERM_COUNT, line);
        int32_t z = gg_days_from_civil(y, (uint32_t)m, (uint32_t)d);
        gg_solar_term_t t;
        if (z < first_day()) { /* Tiểu hàn and Đại hàn 1900, before the range */
            TEST_ASSERT_FALSE_MESSAGE(gg_lunar_solar_term(y, (uint8_t)m, (uint8_t)d, &t), line);
            before_range++;
        } else {
            TEST_ASSERT_EQUAL_INT32_MESSAGE(z, ours[y - GG_LUNAR_FIRST_YEAR][term], line);
        }
        rows++;
    }
    fclose(f);
    TEST_ASSERT_EQUAL_INT(2, before_range);
    TEST_ASSERT_EQUAL_INT(201 * 24, rows); /* with the 201 * 24 - 2 starts the API reports */
}

/*
 * Every solar term of 1901-2100 against HKO's date: each term on another date must be
 * listed in hko_term_differences.csv, in date order, and every listed term must differ.
 * The zone explains all but three, which HKO dates differently from DE440s in its own
 * zone too (docs/lunar.md).
 */
static void test_terms_match_hko_except_listed_differences(void)
{
    static int32_t ours[GG_LUNAR_LAST_YEAR - GG_LUNAR_FIRST_YEAR + 1][GG_SOLAR_TERM_COUNT];
    static struct {
        int32_t ours, hko;
        uint8_t term;
    } found[256];
    our_terms(ours);
    FILE *f = open_data("hko_solar_terms.csv");
    char line[512];
    size_t rows = 0, found_count = 0;
    while (next_row(f, line, sizeof(line))) {
        int y, m, d, term;
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, sscanf(line, "%d-%d-%d,%d", &y, &m, &d, &term), line);
        TEST_ASSERT_TRUE_MESSAGE(y >= 1901 && term >= 0 && term < GG_SOLAR_TERM_COUNT, line);
        int32_t theirs = gg_days_from_civil(y, (uint32_t)m, (uint32_t)d);
        int32_t mine = ours[y - GG_LUNAR_FIRST_YEAR][term];
        TEST_ASSERT_TRUE_MESSAGE(mine != NO_TERM, line);
        if (mine != theirs) {
            TEST_ASSERT_TRUE(found_count < GG_ARRAY_LEN(found));
            found[found_count].ours = mine;
            found[found_count].hko = theirs;
            found[found_count++].term = (uint8_t)term;
        }
        rows++;
    }
    fclose(f);
    TEST_ASSERT_EQUAL(200 * 24, rows);

    f = open_data("hko_term_differences.csv");
    size_t listed = 0, known = 0;
    static const char *const known_terms[] = {"1912-11-22", "1913-09-23", "1979-01-20"};
    while (next_row(f, line, sizeof(line))) {
        int y, m, d, term, hy, hm, hd, reason = 0;
        TEST_ASSERT_EQUAL_INT_MESSAGE(
            7, sscanf(line, "%d-%d-%d,%d,%d-%d-%d,%n", &y, &m, &d, &term, &hy, &hm, &hd, &reason),
            line);
        if (listed >= found_count) {
            TEST_FAIL_MESSAGE("hko_term_differences.csv lists a term that does not differ");
        }
        char want[16], got[16];
        snprintf(want, sizeof(want), "%04d-%02d-%02d", y, m, d);
        iso(got, sizeof(got), found[listed].ours);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(want, got, "first unlisted or stale term difference");
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(found[listed].term, term, line);
        TEST_ASSERT_EQUAL_INT32_MESSAGE(found[listed].hko,
                                        gg_days_from_civil(hy, (uint32_t)hm, (uint32_t)hd), line);
        const char *why = line + reason;
        if (strncmp(why, "known HKO deviation: ", 21) == 0) {
            /* Only the three that docs/lunar.md discusses. */
            TEST_ASSERT_TRUE_MESSAGE(known < GG_ARRAY_LEN(known_terms), line);
            TEST_ASSERT_EQUAL_STRING_MESSAGE(known_terms[known], want, line);
            known++;
        } else {
            TEST_ASSERT_TRUE_MESSAGE(reason > 0 && strncmp(why, "zone: ", 6) == 0, line);
        }
        listed++;
    }
    fclose(f);
    TEST_ASSERT_EQUAL_MESSAGE(found_count, listed,
                              "differences missing from hko_term_differences.csv");
    TEST_ASSERT_EQUAL(GG_ARRAY_LEN(known_terms), known);
    TEST_ASSERT_EQUAL(143, listed); /* docs/lunar.md explains them; a new count needs a reason */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_check_value);
    RUN_TEST(test_tet_where_vietnam_and_china_differ);
    RUN_TEST(test_leap_months);
    RUN_TEST(test_knife_edge_months);
    RUN_TEST(test_zone_switch);
    RUN_TEST(test_knife_edge_solar_terms);
    RUN_TEST(test_solar_terms);
    RUN_TEST(test_can_chi);
    RUN_TEST(test_range_edges);
    RUN_TEST(test_round_trip_every_day);
    RUN_TEST(test_months_and_years_have_valid_lengths);
    RUN_TEST(test_month_11_holds_the_winter_solstice);
    RUN_TEST(test_leap_month_has_no_principal_term);
    RUN_TEST(test_solar_terms_follow_each_other);
    RUN_TEST(test_matches_generator_month_list);
    RUN_TEST(test_matches_hko_except_listed_differences);
    RUN_TEST(test_matches_generator_term_list);
    RUN_TEST(test_terms_match_hko_except_listed_differences);
    return UNITY_END();
}
