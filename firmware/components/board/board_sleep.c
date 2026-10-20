/*
 * Deep sleep entry and wake (ADR-0009, docs/hardware/power-budget.md):
 * backlight off -> SLPIN -> V3V rail off -> every LCD pin parked at a defined
 * level -> buttons released -> ext1 wake armed -> deep sleep.
 */
#include "board/board.h"

#include "board/board_pins.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_timer.h"

typedef struct {
    gpio_num_t pin;
    uint8_t level;
} parked_pin_t;

/* CS, RD, WR and RST idle HIGH; DC and the data bus LOW (never hold the
 * strapping pins GPIO45/46 HIGH). GPIO38 needs nothing: the AW9364 pulls EN down. */
static const parked_pin_t k_parked[] = {
    {BOARD_PIN_LCD_CS, 1}, {BOARD_PIN_LCD_RD, 1},   {BOARD_PIN_LCD_WR, 1}, {BOARD_PIN_LCD_RST, 1},
    {BOARD_PIN_LCD_DC, 0}, {BOARD_PIN_LCD_D0, 0},   {BOARD_PIN_LCD_D1, 0}, {BOARD_PIN_LCD_D2, 0},
    {BOARD_PIN_LCD_D3, 0}, {BOARD_PIN_LCD_D4, 0},   {BOARD_PIN_LCD_D5, 0}, {BOARD_PIN_LCD_D6, 0},
    {BOARD_PIN_LCD_D7, 0}, {BOARD_PIN_POWER_EN, 0},
};

#define BUTTON_MASK ((1ULL << BOARD_PIN_BUTTON_KEY) | (1ULL << BOARD_PIN_BUTTON_BOOT))
#define RELEASE_STABLE_US (50 * 1000)
#define RELEASE_TIMEOUT_US (3 * 1000 * 1000)

static void output(gpio_num_t pin, uint8_t level)
{
    const gpio_config_t cfg = {.pin_bit_mask = 1ULL << pin, .mode = GPIO_MODE_OUTPUT};
    gpio_set_level(pin, level);
    gpio_config(&cfg);
    gpio_set_level(pin, level);
}

void board_sleep_release_holds(void)
{
    /* Drive each pin to its held level before releasing, so nothing glitches. */
    for (size_t i = 0; i < sizeof(k_parked) / sizeof(k_parked[0]); i++) {
        output(k_parked[i].pin, k_parked[i].level);
        gpio_hold_dis(k_parked[i].pin);
    }
    gpio_deep_sleep_hold_dis();
}

uint64_t board_wake_buttons(void)
{
    if (esp_sleep_get_wakeup_causes() & (1UL << ESP_SLEEP_WAKEUP_EXT1)) {
        return esp_sleep_get_ext1_wakeup_status() & BUTTON_MASK;
    }
    return 0;
}

/* Returns the buttons that may wake the device: those that read released for
 * RELEASE_STABLE_US. A button held past the timeout is left out, otherwise
 * ANY_LOW would wake the device again at once. */
static uint64_t released_buttons(void)
{
    int64_t start = esp_timer_get_time();
    int64_t stable_since = start;
    while (esp_timer_get_time() - start < RELEASE_TIMEOUT_US) {
        bool released =
            gpio_get_level(BOARD_PIN_BUTTON_KEY) && gpio_get_level(BOARD_PIN_BUTTON_BOOT);
        int64_t now = esp_timer_get_time();
        if (!released) {
            stable_since = now;
        } else if (now - stable_since >= RELEASE_STABLE_US) {
            return BUTTON_MASK;
        }
        esp_rom_delay_us(1000);
    }
    uint64_t mask = 0;
    if (gpio_get_level(BOARD_PIN_BUTTON_KEY)) {
        mask |= 1ULL << BOARD_PIN_BUTTON_KEY;
    }
    if (gpio_get_level(BOARD_PIN_BUTTON_BOOT)) {
        mask |= 1ULL << BOARD_PIN_BUTTON_BOOT;
    }
    return mask;
}

void board_sleep_enter(uint64_t timer_us)
{
    board_backlight_set(0);
    board_display_sleep(); /* SLPIN: ~6 mA -> ~20 uA, and no ghosting on matte panels */

    for (size_t i = 0; i < sizeof(k_parked) / sizeof(k_parked[0]); i++) {
        output(k_parked[i].pin, k_parked[i].level);
        gpio_hold_en(k_parked[i].pin);
    }
    /* GPIO39-48 are digital pads: their holds need this as well. */
    gpio_deep_sleep_hold_en();

    uint64_t wake = released_buttons();
    if (wake) {
        esp_sleep_enable_ext1_wakeup_io(wake, ESP_EXT1_WAKEUP_ANY_LOW);
    }
    if (timer_us) {
        esp_sleep_enable_timer_wakeup(timer_us);
    }
    esp_deep_sleep_disable_rom_logging();
    esp_deep_sleep_start();
}
