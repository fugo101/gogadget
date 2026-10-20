/*
 * AW9364 backlight by pulse counting (ADR-0007). Step maths derived from
 * Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino setBrightness()
 * @1cc41eb (MIT, see THIRD_PARTY_NOTICES.md); pulse timing from the AW9364
 * V2.4 datasheet (docs/hardware/t-display-s3.md).
 */
#include "board/board.h"

#include "board/board_pins.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

#define BL_LEVELS 16
#define BL_START_US 30 /* EN high >= 20 us starts the chip at level 16 */
#define BL_OFF_US 3000 /* EN low > 2.5 ms shuts it down */
#define BL_PHASE_US 1  /* each high/low phase 0.5-500 us */

static uint8_t s_level; /* 0 = off */
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

esp_err_t board_backlight_init(void)
{
    const gpio_config_t cfg = {.pin_bit_mask = 1ULL << BOARD_PIN_BACKLIGHT,
                               .mode = GPIO_MODE_OUTPUT};
    gpio_set_level(BOARD_PIN_BACKLIGHT, 0);
    esp_err_t err = gpio_config(&cfg);
    gpio_set_level(BOARD_PIN_BACKLIGHT, 0);
    s_level = 0;
    return err;
}

void board_backlight_set(uint8_t level)
{
    if (level > BL_LEVELS) {
        level = BL_LEVELS;
    }
    if (level == s_level) {
        return;
    }
    if (level == 0) {
        gpio_set_level(BOARD_PIN_BACKLIGHT, 0);
        esp_rom_delay_us(BL_OFF_US);
        s_level = 0;
        return;
    }
    if (s_level == 0) {
        gpio_set_level(BOARD_PIN_BACKLIGHT, 1);
        esp_rom_delay_us(BL_START_US);
        s_level = BL_LEVELS;
    }

    /* Each rising edge steps one level down, wrapping from 1 back to 16. A low
     * phase stretched past 500 us by preemption would read as a shutdown, so the
     * train (at most 15 pulses, ~30 us) runs with interrupts off. */
    unsigned pulses = (BL_LEVELS + s_level - level) % BL_LEVELS;
    portENTER_CRITICAL(&s_mux);
    for (unsigned i = 0; i < pulses; i++) {
        gpio_set_level(BOARD_PIN_BACKLIGHT, 0);
        esp_rom_delay_us(BL_PHASE_US);
        gpio_set_level(BOARD_PIN_BACKLIGHT, 1);
        esp_rom_delay_us(BL_PHASE_US);
    }
    portEXIT_CRITICAL(&s_mux);
    s_level = level;
}
