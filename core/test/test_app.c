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
    return UNITY_END();
}
