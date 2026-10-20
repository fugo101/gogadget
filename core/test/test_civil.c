#include "gg/gg_civil.h"
#include "gg/gg_types.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

static void test_epoch_zero_is_thursday_1970(void)
{
    gg_civil_t c;
    gg_civil_from_epoch(0, 0, &c);
    TEST_ASSERT_EQUAL(1970, c.year);
    TEST_ASSERT_EQUAL(1, c.month);
    TEST_ASSERT_EQUAL(1, c.day);
    TEST_ASSERT_EQUAL(4, c.weekday);
}

static void test_vietnam_offset_crosses_midnight(void)
{
    /* 2026-10-19 17:00:00 UTC is 2026-10-20 00:00:00 in Vietnam, a Tuesday. */
    int64_t t = (int64_t)gg_days_from_civil(2026, 10, 19) * 86400 + 17 * 3600;
    gg_civil_t c;
    gg_civil_from_epoch(t, GG_UTC_OFFSET_S, &c);
    TEST_ASSERT_EQUAL(2026, c.year);
    TEST_ASSERT_EQUAL(10, c.month);
    TEST_ASSERT_EQUAL(20, c.day);
    TEST_ASSERT_EQUAL(2, c.weekday);
    TEST_ASSERT_EQUAL(0, c.hour);
    TEST_ASSERT_EQUAL(0, c.minute);
}

static void test_negative_epoch(void)
{
    gg_civil_t c;
    gg_civil_from_epoch(-1, 0, &c);
    TEST_ASSERT_EQUAL(1969, c.year);
    TEST_ASSERT_EQUAL(12, c.month);
    TEST_ASSERT_EQUAL(31, c.day);
    TEST_ASSERT_EQUAL(23, c.hour);
    TEST_ASSERT_EQUAL(59, c.second);
    TEST_ASSERT_EQUAL(3, c.weekday);
}

static void test_round_trip_1900_to_2100(void)
{
    int32_t first = gg_days_from_civil(1900, 1, 1);
    int32_t last = gg_days_from_civil(2100, 12, 31);
    for (int32_t z = first; z <= last; z++) {
        gg_civil_t c;
        gg_civil_from_epoch((int64_t)z * 86400, 0, &c);
        TEST_ASSERT_EQUAL_INT32(z, gg_days_from_civil(c.year, c.month, c.day));
        TEST_ASSERT_TRUE(c.day >= 1 && c.day <= gg_days_in_month(c.year, c.month));
    }
}

static void test_leap_years(void)
{
    TEST_ASSERT_TRUE(gg_is_leap_year(2000));
    TEST_ASSERT_FALSE(gg_is_leap_year(1900));
    TEST_ASSERT_FALSE(gg_is_leap_year(2100));
    TEST_ASSERT_TRUE(gg_is_leap_year(2028));
    TEST_ASSERT_EQUAL(29, gg_days_in_month(2028, 2));
    TEST_ASSERT_EQUAL(28, gg_days_in_month(2026, 2));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_epoch_zero_is_thursday_1970);
    RUN_TEST(test_vietnam_offset_crosses_midnight);
    RUN_TEST(test_negative_epoch);
    RUN_TEST(test_round_trip_1900_to_2100);
    RUN_TEST(test_leap_years);
    return UNITY_END();
}
