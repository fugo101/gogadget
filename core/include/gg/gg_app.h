/*
 * Application state and its reducer (ADR-0004).
 *
 * Every change to what the device shows or does goes through gg_app_handle():
 * shells (firmware, simulator, tests) post messages, the reducer updates the
 * state and returns effects for the shell to carry out. ui/ renders the state.
 * Nothing here touches hardware, LVGL or the clock: time arrives in messages.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gg/gg_civil.h"
#include "gg/gg_types.h"

typedef enum {
    GG_SCREEN_WATCHFACE = 0,
    GG_SCREEN_MENU,
    GG_SCREEN_SYSINFO,
    GG_SCREEN_TESTPATTERN, /* hardware acceptance test (docs/hardware/bring-up.md DSP-1) */
    GG_SCREEN_COUNT,
} gg_screen_t;

/* Entries of the Menu, in display order. */
typedef enum {
    GG_MENU_SYSINFO = 0,
    GG_MENU_COUNT,
} gg_menu_item_t;

typedef enum {
    GG_MODE_GLANCE = 0, /* on battery: lit, returns to Standby after the glance time */
    GG_MODE_ALWAYS_ON,  /* on USB: lit, never enters Standby on its own */
} gg_mode_t;

typedef enum {
    GG_WIFI_OFF = 0,
    GG_WIFI_CONNECTING,
    GG_WIFI_CONNECTED,
    GG_WIFI_FAILED,
} gg_wifi_t;

typedef struct {
    bool valid;     /* false until the first Sync (or after a lost time) */
    gg_civil_t now; /* local civil time */
    /* The Lunar date of .now (gg_lunar.h), recomputed when the date changes; not valid
     * while .valid is false or outside the calendar's range. */
    bool lunar_valid;
    uint8_t lunar_day;
    uint8_t lunar_month;
    bool lunar_leap; /* a day of the leap month that repeats .lunar_month */
} gg_clock_t;

typedef struct {
    bool usb;            /* USB power present */
    bool battery_known;  /* a VSYS reading on battery exists */
    uint8_t battery_pct; /* valid when battery_known && !usb */
    uint16_t vsys_mv;    /* last reading, for System info */
    gg_wifi_t wifi;
    bool unsynced; /* show the Unsynced badge */
} gg_status_t;

typedef struct {
    uint32_t glance_ms;         /* Glance length, extended by every button action */
    uint8_t brightness_battery; /* Brightness level in Glance */
    uint8_t brightness_usb;     /* Brightness level in Always-on */
    uint32_t shift_period_ms;   /* Always-on pixel shift period */
} gg_app_config_t;

#define GG_APP_CONFIG_DEFAULT                                                                      \
    {.glance_ms = 10000, .brightness_battery = 6, .brightness_usb = 10, .shift_period_ms = 180000}

typedef struct {
    gg_app_config_t cfg;
    gg_mode_t mode;
    gg_screen_t screen;
    uint8_t menu_focus;
    gg_clock_t clock;
    gg_status_t status;
    int8_t shift_x; /* Always-on pixel shift applied to the Watchface */
    int8_t shift_y;
    uint8_t shift_step;
    uint32_t last_activity_ms;
    uint32_t last_shift_ms;
} gg_state_t;

typedef enum {
    GG_MSG_NAV = 0, /* .nav: a button action */
    GG_MSG_TICK,    /* periodic, carries only .at_ms */
    GG_MSG_TIME,    /* .time: current UTC epoch, or valid = false */
    GG_MSG_POWER,   /* .power: USB presence and VSYS */
    GG_MSG_NET,     /* .net: Wi-Fi and Sync state */
    GG_MSG_GOTO,    /* .screen: jump to a Screen (Dev console) */
} gg_msg_kind_t;

typedef struct {
    gg_msg_kind_t kind;
    uint32_t at_ms; /* monotonic milliseconds when the message was produced */
    union {
        gg_nav_t nav;
        struct {
            int64_t epoch_s;
            bool valid;
        } time;
        struct {
            bool usb;
            uint16_t vsys_mv;
        } power;
        struct {
            gg_wifi_t wifi;
            bool unsynced;
        } net;
        gg_screen_t screen;
    };
} gg_msg_t;

typedef struct {
    bool render;        /* something visible changed: call ui_render() */
    bool enter_standby; /* backlight off, park the panel, deep sleep */
    int8_t brightness;  /* -1 = unchanged, otherwise the Brightness level to set */
} gg_effects_t;

void gg_app_init(gg_state_t *s, const gg_app_config_t *cfg, bool usb, uint32_t now_ms);

gg_effects_t gg_app_handle(gg_state_t *s, const gg_msg_t *m);

/* The Brightness level the current mode wants. */
uint8_t gg_app_brightness(const gg_state_t *s);
