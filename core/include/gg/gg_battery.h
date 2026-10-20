/*
 * Battery level from the VSYS voltage measured on GPIO4 (see
 * docs/hardware/t-display-s3.md, battery sensing). VSYS is the cell voltage
 * minus small path losses while on battery; on USB it is VBUS minus a diode
 * drop (~4.6 V) and says nothing about the cell.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Above this VSYS the board is on USB power (a full cell rests at 4.2 V). */
#define GG_BATTERY_USB_MV 4350

/* Below this VSYS a Wi-Fi start risks a brownout (500 mA LDO, ADR-0011). */
#define GG_BATTERY_WIFI_MIN_MV 3600

/* Remaining charge in percent (0-100) for a VSYS reading taken under light
 * load. Piecewise-linear over a typical LiPo discharge curve. */
uint8_t gg_battery_percent(uint16_t vsys_mv);

bool gg_battery_wifi_allowed(uint16_t vsys_mv);
