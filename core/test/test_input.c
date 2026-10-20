#include "gg/gg_input.h"
#include "unity.h"

static gg_input_t in;
static uint32_t now;

void setUp(void)
{
    gg_input_config_t cfg = GG_INPUT_CONFIG_DEFAULT;
    gg_input_init(&in, &cfg);
    now = 1000;
}

void tearDown(void)
{
}

/* Hold `b` at `pressed` for `ms`, sampling every 5 ms; return the first action seen. */
static gg_nav_t hold(gg_button_t b, bool pressed, uint32_t ms)
{
    gg_nav_t first = GG_NAV_NONE;
    for (uint32_t t = 0; t <= ms; t += 5) {
        gg_nav_t nav = gg_input_sample(&in, b, pressed, now);
        if (first == GG_NAV_NONE) {
            first = nav;
        }
        now += 5;
    }
    return first;
}

static void test_key_press_is_next_on_release(void)
{
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, true, 200));
    TEST_ASSERT_EQUAL(GG_NAV_NEXT, hold(GG_BTN_KEY, false, 100));
}

static void test_key_hold_is_back_once(void)
{
    TEST_ASSERT_EQUAL(GG_NAV_BACK, hold(GG_BTN_KEY, true, 1500));
    /* Releasing after a hold produces nothing more. */
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, false, 100));
}

static void test_boot_press_is_enter_on_press(void)
{
    TEST_ASSERT_EQUAL(GG_NAV_ENTER, hold(GG_BTN_BOOT, true, 100));
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_BOOT, true, 2000));
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_BOOT, false, 100));
}

static void test_bounce_shorter_than_debounce_is_ignored(void)
{
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(GG_NAV_NONE, gg_input_sample(&in, GG_BTN_KEY, i % 2 == 0, now));
        now += 5;
    }
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, false, 100));
}

static void test_wake_press_is_consumed(void)
{
    gg_input_consume_wake_press(&in, GG_BTN_KEY, now);
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, true, 1500));
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, false, 100));
    /* The next press works normally. */
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_KEY, true, 100));
    TEST_ASSERT_EQUAL(GG_NAV_NEXT, hold(GG_BTN_KEY, false, 100));
}

static void test_wake_press_on_boot_is_consumed(void)
{
    gg_input_consume_wake_press(&in, GG_BTN_BOOT, now);
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_BOOT, true, 300));
    TEST_ASSERT_EQUAL(GG_NAV_NONE, hold(GG_BTN_BOOT, false, 100));
    TEST_ASSERT_EQUAL(GG_NAV_ENTER, hold(GG_BTN_BOOT, true, 100));
}

static void test_busy_until_released_and_settled(void)
{
    TEST_ASSERT_FALSE(gg_input_busy(&in));
    hold(GG_BTN_KEY, true, 100);
    TEST_ASSERT_TRUE(gg_input_busy(&in));
    hold(GG_BTN_KEY, false, 100);
    TEST_ASSERT_FALSE(gg_input_busy(&in));
}

static void test_timestamps_wrap(void)
{
    now = UINT32_MAX - 50;
    TEST_ASSERT_EQUAL(GG_NAV_BACK, hold(GG_BTN_KEY, true, 1000));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_key_press_is_next_on_release);
    RUN_TEST(test_key_hold_is_back_once);
    RUN_TEST(test_boot_press_is_enter_on_press);
    RUN_TEST(test_bounce_shorter_than_debounce_is_ignored);
    RUN_TEST(test_wake_press_is_consumed);
    RUN_TEST(test_wake_press_on_boot_is_consumed);
    RUN_TEST(test_busy_until_released_and_settled);
    RUN_TEST(test_timestamps_wrap);
    return UNITY_END();
}
