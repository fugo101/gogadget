#include "gg/gg_battery.h"
#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

static void test_curve_end_points(void)
{
    TEST_ASSERT_EQUAL(100, gg_battery_percent(4250));
    TEST_ASSERT_EQUAL(100, gg_battery_percent(4200));
    TEST_ASSERT_EQUAL(0, gg_battery_percent(3300));
    TEST_ASSERT_EQUAL(0, gg_battery_percent(3000));
    TEST_ASSERT_EQUAL(0, gg_battery_percent(0));
}

static void test_curve_is_monotonic(void)
{
    uint8_t prev = 0;
    for (uint16_t mv = 3000; mv <= 4300; mv++) {
        uint8_t pct = gg_battery_percent(mv);
        TEST_ASSERT_TRUE(pct >= prev);
        prev = pct;
    }
}

static void test_wifi_threshold(void)
{
    TEST_ASSERT_FALSE(gg_battery_wifi_allowed(3599));
    TEST_ASSERT_TRUE(gg_battery_wifi_allowed(3600));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_curve_end_points);
    RUN_TEST(test_curve_is_monotonic);
    RUN_TEST(test_wifi_threshold);
    return UNITY_END();
}
