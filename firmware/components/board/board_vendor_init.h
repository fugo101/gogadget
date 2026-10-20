/*
 * Portions derived from Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino
 * @1cc41eb (LCD_MODULE_CMD_1, MIT). See THIRD_PARTY_NOTICES.md.
 *
 * LilyGO's vendor init table for the ST7789V panel, sent byte-for-byte after
 * esp_lcd_panel_init() on every unit (ADR-0006). The maintainer: without it
 * "the pixels will be blurred" (#229). Decoded against the ST7789V v1.4
 * datasheet in docs/hardware/t-display-s3.md; do not "clean up" these values.
 */
#pragma once

#include <stdint.h>

typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint16_t delay_ms; /* wait after sending */
    uint8_t data[14];
} board_lcd_init_cmd_t;

static const board_lcd_init_cmd_t k_vendor_init[] = {
    /* SLPOUT. esp_lcd sends one only 10 ms after reset, while SLPOUT needs
     * >= 120 ms after reset release (datasheet Table 9 note 7): keep this one. */
    {0x11, 0, 120, {0}},
    /* COLMOD 05: 16 bpp on the MCU interface (vendor value; esp_lcd sent 55). */
    {0x3A, 1, 0, {0x05}},
    /* PORCTRL: back/front porch 11/11 lines (default 12/12). */
    {0xB2, 5, 0, {0x0B, 0x0B, 0x00, 0x33, 0x33}},
    /* GCTRL: VGH 14.97 V, VGL -10.43 V. */
    {0xB7, 1, 0, {0x75}},
    /* VCOMS: VCOM 1.10 V (default 0.90 V). */
    {0xBB, 1, 0, {0x28}},
    /* LCMCTRL: power-on default, keeps RGB order. */
    {0xC0, 1, 0, {0x2C}},
    /* VDVVRHEN: take VDV/VRH from the registers. */
    {0xC2, 1, 0, {0x01}},
    /* VRHS: GVDD 5.10 V. */
    {0xC3, 1, 0, {0x1F}},
    /* FRCTRL2: dot inversion, ~53 Hz frame rate. */
    {0xC6, 1, 0, {0x13}},
    /* PWCTRL1 with an undocumented first byte (the key is A4h); vendor value. */
    {0xD0, 1, 0, {0xA7}},
    /* PWCTRL1: AVDD 6.8 V, AVCL -4.8 V, VDS 2.3 V. */
    {0xD0, 2, 0, {0xA4, 0xA1}},
    /* D6h: not in the v1.4 datasheet; vendor value. */
    {0xD6, 1, 0, {0xA1}},
    /* PVGAMCTRL: positive gamma. */
    {0xE0,
     14,
     0,
     {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32}},
    /* NVGAMCTRL: negative gamma. */
    {0xE1,
     14,
     0,
     {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37}},
};
