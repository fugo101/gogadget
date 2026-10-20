/*
 * Board support for the T-Display S3: display, backlight, buttons, power
 * sensing and deep sleep. Nothing here knows about application state; the
 * composition root in firmware/main wires these into core and ui.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "gg/gg_types.h"
#include "lvgl.h"

/* ---- Power (board_power.c) ---- */

/* Configures GPIO15 and the VSYS ADC. Call first. Returns whether USB power is
 * present, measured with GPIO15 LOW (the clean test), then switches the V3V
 * rail on so the backlight can be used. */
esp_err_t board_power_init(bool *usb_present);

/* VSYS in millivolts (16-sample average, calibrated). Meaningful on battery;
 * on USB it reads VBUS minus a diode drop. */
uint16_t board_power_vsys_mv(void);

/* ---- Display (board_display.c, ADR-0006) ---- */

/* Brings up the i80 bus and the panel (vendor init table included) and
 * registers it with esp_lvgl_port. The panel stays dark: no DISPON yet. */
esp_err_t board_display_init(bool rotate_180, lv_display_t **out_disp);

/* Sends DISPON. Call after the first frame has been rendered and flushed. */
esp_err_t board_display_on(void);

/* Exclusive LVGL access from outside the LVGL task (wraps esp_lvgl_port, which
 * stays inside this component, ADR-0002). Code running in LVGL timers already
 * holds it. */
void board_display_lock(void);
void board_display_unlock(void);

/* Sends SLPIN (panel draws ~20 uA instead of ~6 mA and keeps its RAM). */
esp_err_t board_display_sleep(void);

/* ---- Backlight (board_backlight.c, ADR-0007) ---- */

esp_err_t board_backlight_init(void);

/* Brightness level 0 (off) to 16. */
void board_backlight_set(uint8_t level);

/* ---- Buttons (board_buttons.c, ADR-0008) ---- */

typedef void (*board_buttons_cb_t)(gg_nav_t nav, void *ctx);

/* Starts sampling both buttons and reports navigation actions from the
 * esp_timer task. `wake_mask` (from board_wake_buttons()) marks buttons whose
 * current press woke the device; that press is consumed. */
esp_err_t board_buttons_init(uint64_t wake_mask, board_buttons_cb_t cb, void *ctx);

/* ---- Sleep (board_sleep.c, ADR-0009) ---- */

/* Releases the pin holds of the previous deep sleep. Call early in app_main. */
void board_sleep_release_holds(void);

/* GPIO mask of the buttons that woke the device, or 0 for any other reset. */
uint64_t board_wake_buttons(void);

/* Backlight off, panel SLPIN, rail off, pins parked, button wake armed, plus a
 * timer wake when `timer_us` is non-zero; then deep sleep. Does not return. */
void board_sleep_enter(uint64_t timer_us) __attribute__((noreturn));
