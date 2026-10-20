/* LilyGO T-Display S3 v1.2, non-touch. Facts: docs/hardware/t-display-s3.md. */
#pragma once

/* ST7789V on the 8-bit i80 bus */
#define BOARD_PIN_LCD_D0 39
#define BOARD_PIN_LCD_D1 40
#define BOARD_PIN_LCD_D2 41
#define BOARD_PIN_LCD_D3 42
#define BOARD_PIN_LCD_D4 45 /* strapping pin: never hold HIGH */
#define BOARD_PIN_LCD_D5 46 /* strapping pin: never hold HIGH */
#define BOARD_PIN_LCD_D6 47
#define BOARD_PIN_LCD_D7 48
#define BOARD_PIN_LCD_WR 8
#define BOARD_PIN_LCD_RD 9 /* not driven by esp_lcd; must be HIGH */
#define BOARD_PIN_LCD_DC 7
#define BOARD_PIN_LCD_CS 6
#define BOARD_PIN_LCD_RST 5

/* AW9364 EN (pulse-counting backlight, internal pull-down) */
#define BOARD_PIN_BACKLIGHT 38

/* Switches V3V (backlight supply, green LED) and the GPIO4 divider on battery.
 * VBUS also switches them, so on USB this pin has no effect. Never enable a
 * pull-up on it (gpio_reset_pin() does): that alone turns V3V on. */
#define BOARD_PIN_POWER_EN 15

/* VSYS / 2 through 100k/100k + 10 nF, ADC1 channel 3 */
#define BOARD_PIN_VSYS_ADC 4

/* Buttons: active LOW, external pull-ups to the always-on 3.3 V rail */
#define BOARD_PIN_BUTTON_KEY 14 /* 100k + 100 nF */
#define BOARD_PIN_BUTTON_BOOT 0 /* 10k, strapping pin */

#define BOARD_LCD_H_RES 320 /* landscape */
#define BOARD_LCD_V_RES 170
