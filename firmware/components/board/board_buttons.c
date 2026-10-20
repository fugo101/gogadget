/*
 * Both buttons are sampled every 5 ms from an esp_timer and fed to core's
 * input state machine (ADR-0008). Awake periods are short on battery and
 * powered on USB, so plain sampling is simpler than edge interrupts.
 */
#include "board/board.h"

#include "board/board_pins.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "gg/gg_input.h"

static const char *TAG = "buttons";

#define SAMPLE_PERIOD_US 5000

static gg_input_t s_input;
static esp_timer_handle_t s_timer;
static board_buttons_cb_t s_cb;
static void *s_ctx;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void sample(void *arg)
{
    (void)arg;
    uint32_t now = now_ms();
    gg_nav_t nav =
        gg_input_sample(&s_input, GG_BTN_KEY, gpio_get_level(BOARD_PIN_BUTTON_KEY) == 0, now);
    if (nav != GG_NAV_NONE) {
        s_cb(nav, s_ctx);
    }
    nav = gg_input_sample(&s_input, GG_BTN_BOOT, gpio_get_level(BOARD_PIN_BUTTON_BOOT) == 0, now);
    if (nav != GG_NAV_NONE) {
        s_cb(nav, s_ctx);
    }
}

esp_err_t board_buttons_init(uint64_t wake_mask, board_buttons_cb_t cb, void *ctx)
{
    /* External pull-ups to the always-on rail; no internal pulls needed. */
    const gpio_config_t in = {
        .pin_bit_mask = (1ULL << BOARD_PIN_BUTTON_KEY) | (1ULL << BOARD_PIN_BUTTON_BOOT),
        .mode = GPIO_MODE_INPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&in), TAG, "gpio");

    s_cb = cb;
    s_ctx = ctx;
    const gg_input_config_t cfg = GG_INPUT_CONFIG_DEFAULT;
    gg_input_init(&s_input, &cfg);
    uint32_t now = now_ms();
    if (wake_mask & (1ULL << BOARD_PIN_BUTTON_KEY)) {
        gg_input_consume_wake_press(&s_input, GG_BTN_KEY, now);
    }
    if (wake_mask & (1ULL << BOARD_PIN_BUTTON_BOOT)) {
        gg_input_consume_wake_press(&s_input, GG_BTN_BOOT, now);
    }

    const esp_timer_create_args_t args = {.callback = sample, .name = "buttons"};
    ESP_RETURN_ON_ERROR(esp_timer_create(&args, &s_timer), TAG, "timer");
    return esp_timer_start_periodic(s_timer, SAMPLE_PERIOD_US);
}
