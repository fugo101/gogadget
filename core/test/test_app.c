#include "gg/gg_app.h"
#include "unity.h"

static gg_state_t s;

void setUp(void)
{
    gg_app_config_t cfg = GG_APP_CONFIG_DEFAULT;
    gg_app_init(&s, &cfg, false, 0);
}

void tearDown(void)
{
}

static gg_effects_t nav(gg_nav_t n, uint32_t at)
{
    gg_msg_t m = {.kind = GG_MSG_NAV, .at_ms = at, .nav = n};
    return gg_app_handle(&s, &m);
}

static gg_effects_t tick(uint32_t at)
{
    gg_msg_t m = {.kind = GG_MSG_TICK, .at_ms = at};
    return gg_app_handle(&s, &m);
}

static gg_effects_t power(bool usb, uint16_t mv, uint32_t at)
{
    gg_msg_t m = {.kind = GG_MSG_POWER, .at_ms = at, .power = {.usb = usb, .vsys_mv = mv}};
    return gg_app_handle(&s, &m);
}

static void test_glance_ends_after_glance_time(void)
{
    TEST_ASSERT_FALSE(tick(9999).enter_standby);
    TEST_ASSERT_TRUE(tick(10000).enter_standby);
}

static void test_button_activity_extends_glance(void)
{
    nav(GG_NAV_NEXT, 8000);
    TEST_ASSERT_FALSE(tick(17999).enter_standby);
    TEST_ASSERT_TRUE(tick(18000).enter_standby);
}

static void test_back_on_watchface_enters_standby(void)
{
    TEST_ASSERT_TRUE(nav(GG_NAV_BACK, 100).enter_standby);
}

static void test_menu_navigation(void)
{
    TEST_ASSERT_TRUE(nav(GG_NAV_ENTER, 100).render);
    TEST_ASSERT_EQUAL(GG_SCREEN_MENU, s.screen);
    TEST_ASSERT_EQUAL(0, s.menu_focus);

    nav(GG_NAV_NEXT, 200); /* single item: wraps to itself */
    TEST_ASSERT_EQUAL(0, s.menu_focus);

    nav(GG_NAV_ENTER, 300);
    TEST_ASSERT_EQUAL(GG_SCREEN_SYSINFO, s.screen);

    nav(GG_NAV_BACK, 400);
    TEST_ASSERT_EQUAL(GG_SCREEN_MENU, s.screen);
    gg_effects_t fx = nav(GG_NAV_BACK, 500);
    TEST_ASSERT_EQUAL(GG_SCREEN_WATCHFACE, s.screen);
    TEST_ASSERT_FALSE(fx.enter_standby);
}

static void test_usb_switches_to_always_on_and_back(void)
{
    gg_effects_t fx = power(true, 4600, 1000);
    TEST_ASSERT_EQUAL(GG_MODE_ALWAYS_ON, s.mode);
    TEST_ASSERT_EQUAL(s.cfg.brightness_usb, fx.brightness);
    TEST_ASSERT_FALSE(tick(60000).enter_standby);

    fx = power(false, 4100, 70000);
    TEST_ASSERT_EQUAL(GG_MODE_GLANCE, s.mode);
    TEST_ASSERT_EQUAL(s.cfg.brightness_battery, fx.brightness);
    /* Unplugging starts a fresh Glance. */
    TEST_ASSERT_FALSE(tick(79999).enter_standby);
    TEST_ASSERT_TRUE(tick(80000).enter_standby);
}

static void test_battery_level_only_on_battery(void)
{
    power(false, 4200, 0);
    TEST_ASSERT_TRUE(s.status.battery_known);
    TEST_ASSERT_EQUAL(100, s.status.battery_pct);
    power(true, 4600, 10);
    TEST_ASSERT_EQUAL(100, s.status.battery_pct); /* USB reading does not count */
}

static void test_always_on_pixel_shift(void)
{
    power(true, 4600, 0);
    TEST_ASSERT_FALSE(tick(179999).render);
    TEST_ASSERT_TRUE(tick(180000).render);
    TEST_ASSERT_TRUE(s.shift_x != 0 || s.shift_y != 0);
    power(false, 4100, 200000);
    TEST_ASSERT_EQUAL(0, s.shift_x);
    TEST_ASSERT_EQUAL(0, s.shift_y);
}

static void test_time_updates_render_only_on_change(void)
{
    gg_msg_t m = {.kind = GG_MSG_TIME, .time = {.epoch_s = 1800000000, .valid = true}};
    TEST_ASSERT_TRUE(gg_app_handle(&s, &m).render);
    TEST_ASSERT_TRUE(s.clock.valid);
    TEST_ASSERT_FALSE(gg_app_handle(&s, &m).render);
    m.time.valid = false;
    TEST_ASSERT_TRUE(gg_app_handle(&s, &m).render);
    TEST_ASSERT_FALSE(s.clock.valid);
}

/* Epoch seconds of a Vietnamese local time (UTC+7). */
static int64_t local_epoch(int32_t y, uint32_t mo, uint32_t d, int h, int mi, int sec)
{
    return (int64_t)gg_days_from_civil(y, mo, d) * 86400 + h * 3600 + mi * 60 + sec -
           GG_UTC_OFFSET_S;
}

static void post_time(int64_t epoch_s, bool valid)
{
    gg_msg_t m = {.kind = GG_MSG_TIME, .time = {.epoch_s = epoch_s, .valid = valid}};
    gg_app_handle(&s, &m);
}

static void assert_lunar(uint8_t day, uint8_t month, bool leap)
{
    TEST_ASSERT_TRUE(s.clock.lunar_valid);
    TEST_ASSERT_EQUAL_UINT8(day, s.clock.lunar_day);
    TEST_ASSERT_EQUAL_UINT8(month, s.clock.lunar_month);
    TEST_ASSERT_EQUAL(leap, s.clock.lunar_leap);
}

static void test_time_fills_lunar_date(void)
{
    TEST_ASSERT_FALSE(s.clock.lunar_valid);
    post_time(local_epoch(2026, 10, 20, 14, 35, 42), true); /* ADR-0015 check value */
    assert_lunar(11, 9, false);
}

static void test_lunar_date_changes_at_local_midnight(void)
{
    post_time(local_epoch(2026, 10, 20, 23, 59, 59), true);
    assert_lunar(11, 9, false);
    post_time(local_epoch(2026, 10, 21, 0, 0, 0), true);
    assert_lunar(12, 9, false);
    /* Tết 2027: the last day of month 12 (29 days), then day 1 of month 1. */
    post_time(local_epoch(2027, 2, 5, 23, 59, 59), true);
    assert_lunar(29, 12, false);
    post_time(local_epoch(2027, 2, 6, 0, 0, 0), true);
    assert_lunar(1, 1, false);
}

static void test_leap_month_sets_flag(void)
{
    post_time(local_epoch(2025, 7, 24, 12, 0, 0), true);
    assert_lunar(30, 6, false);
    post_time(local_epoch(2025, 7, 27, 12, 0, 0), true); /* leap 6th month from 2025-07-25 */
    assert_lunar(3, 6, true);
    post_time(local_epoch(2025, 8, 23, 12, 0, 0), true);
    assert_lunar(1, 7, false);
}

static void test_lunar_date_outside_range_is_not_valid(void)
{
    post_time(local_epoch(2101, 3, 1, 12, 0, 0), true);
    TEST_ASSERT_TRUE(s.clock.valid);
    TEST_ASSERT_FALSE(s.clock.lunar_valid);
    post_time(local_epoch(2100, 12, 31, 12, 0, 0), true);
    TEST_ASSERT_TRUE(s.clock.lunar_valid);
}

static void test_lost_time_clears_lunar_date(void)
{
    post_time(local_epoch(2026, 10, 20, 8, 0, 0), true);
    post_time(0, false);
    TEST_ASSERT_FALSE(s.clock.lunar_valid);
    post_time(local_epoch(2026, 10, 20, 8, 0, 1), true); /* the same date again */
    assert_lunar(11, 9, false);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_glance_ends_after_glance_time);
    RUN_TEST(test_button_activity_extends_glance);
    RUN_TEST(test_back_on_watchface_enters_standby);
    RUN_TEST(test_menu_navigation);
    RUN_TEST(test_usb_switches_to_always_on_and_back);
    RUN_TEST(test_battery_level_only_on_battery);
    RUN_TEST(test_always_on_pixel_shift);
    RUN_TEST(test_time_updates_render_only_on_change);
    RUN_TEST(test_time_fills_lunar_date);
    RUN_TEST(test_lunar_date_changes_at_local_midnight);
    RUN_TEST(test_leap_month_sets_flag);
    RUN_TEST(test_lunar_date_outside_range_is_not_valid);
    RUN_TEST(test_lost_time_clears_lunar_date);
    return UNITY_END();
}
