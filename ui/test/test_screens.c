/*
 * Golden screenshots (ADR-0005): render fixed states and compare them with
 * ui/test/golden/<screen>-<state>.png. Create or update references with
 * `make golden`, then review the PNG diff.
 */
#include <stdio.h>

#include "gg/gg_app.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "unity.h"

static gg_state_t s;

/* 14:35:42 in Vietnam on a Solar date; the reducer computes the Lunar date. */
static int64_t fixture_epoch(int32_t year, uint32_t month, uint32_t day)
{
    return (int64_t)gg_days_from_civil(year, month, day) * 86400 + 7 * 3600 + 35 * 60 + 42;
}

static void post(gg_msg_t m)
{
    gg_app_handle(&s, &m);
}

static void post_date(int32_t year, uint32_t month, uint32_t day)
{
    post((gg_msg_t){.kind = GG_MSG_TIME,
                    .time = {.epoch_s = fixture_epoch(year, month, day), .valid = true}});
}

void setUp(void)
{
    gg_app_config_t cfg = GG_APP_CONFIG_DEFAULT;
    gg_app_init(&s, &cfg, false, 0);
    post_date(2026, 10, 20); /* Tuesday; lunar 11/9 (ADR-0015 check value) */
    post((gg_msg_t){.kind = GG_MSG_POWER, .power = {.usb = false, .vsys_mv = 3950}});
    post((gg_msg_t){.kind = GG_MSG_NET, .net = {.wifi = GG_WIFI_OFF, .unsynced = false}});
}

void tearDown(void)
{
}

static void expect_screen(const char *name)
{
    ui_render(&s);
    lv_refr_now(NULL);
    char fn[96];
    snprintf(fn, sizeof(fn), "%s.png", name);
    TEST_ASSERT_EQUAL_MESSAGE(LV_TEST_SCREENSHOT_RESULT_PASSED, lv_test_screenshot_compare(fn), fn);
}

static void test_watchface_default(void)
{
    expect_screen("watchface-default");
}

static void test_watchface_unsynced(void)
{
    post((gg_msg_t){.kind = GG_MSG_TIME, .time = {.valid = false}});
    expect_screen("watchface-unsynced");
}

static void test_watchface_usb_wifi(void)
{
    post((gg_msg_t){.kind = GG_MSG_POWER, .power = {.usb = true, .vsys_mv = 4610}});
    post((gg_msg_t){.kind = GG_MSG_NET, .net = {.wifi = GG_WIFI_CONNECTED, .unsynced = false}});
    expect_screen("watchface-usb-wifi");
}

static void test_watchface_leap_month(void)
{
    post_date(2025, 7, 27); /* Sunday; day 3 of the leap 6th month of Ất Tỵ */
    TEST_ASSERT_TRUE(s.clock.lunar_leap);
    expect_screen("watchface-leap-month");
}

static void test_menu(void)
{
    post((gg_msg_t){.kind = GG_MSG_NAV, .nav = GG_NAV_ENTER});
    expect_screen("menu-default");
}

static void test_testpattern(void)
{
    post((gg_msg_t){.kind = GG_MSG_GOTO, .screen = GG_SCREEN_TESTPATTERN});
    expect_screen("testpattern");
}

static void test_sysinfo(void)
{
    post((gg_msg_t){.kind = GG_MSG_GOTO, .screen = GG_SCREEN_SYSINFO});
    expect_screen("sysinfo-battery");
}

int main(void)
{
    lv_init();
    lv_display_t *disp = lv_test_display_create(UI_HOR_RES, UI_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    ui_init(disp, &(ui_config_t){.version = "0.0.0-test"});

    UNITY_BEGIN();
    RUN_TEST(test_watchface_default);
    RUN_TEST(test_watchface_unsynced);
    RUN_TEST(test_watchface_usb_wifi);
    RUN_TEST(test_watchface_leap_month);
    RUN_TEST(test_menu);
    RUN_TEST(test_sysinfo);
    RUN_TEST(test_testpattern);
    return UNITY_END();
}
