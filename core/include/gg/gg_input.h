/*
 * Two-button input state machine (ADR-0008).
 *
 * The firmware samples both buttons every few milliseconds while either is
 * active and feeds the levels in; this module debounces them and turns them
 * into navigation actions:
 *   KEY  press -> GG_NAV_NEXT, KEY hold -> GG_NAV_BACK
 *   BOOT press -> GG_NAV_ENTER (BOOT has no hold action: GPIO0 is a strapping pin)
 * A button marked with gg_input_consume_wake_press() produces nothing until it
 * has been released once, so the press that woke the device triggers no action.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gg/gg_types.h"

typedef enum {
    GG_BTN_KEY = 0, /* GPIO14 */
    GG_BTN_BOOT,    /* GPIO0 */
    GG_BTN_COUNT,
} gg_button_t;

typedef struct {
    uint16_t debounce_ms; /* level must be stable this long before it counts */
    uint16_t hold_ms;     /* KEY held this long becomes Back instead of Next */
} gg_input_config_t;

#define GG_INPUT_CONFIG_DEFAULT {.debounce_ms = 30, .hold_ms = 600}

typedef struct {
    bool raw;            /* last sampled level (true = pressed) */
    bool stable;         /* debounced level */
    bool suppressed;     /* ignore until the next release */
    bool hold_fired;     /* Back already emitted for this press */
    uint32_t raw_since;  /* when raw last changed */
    uint32_t down_since; /* when the debounced press started */
} gg_input_button_t;

typedef struct {
    gg_input_config_t cfg;
    gg_input_button_t btn[GG_BTN_COUNT];
} gg_input_t;

void gg_input_init(gg_input_t *in, const gg_input_config_t *cfg);

/* Ignore the current press of `b` (the wake press); it only becomes active
 * again after it has been seen released. */
void gg_input_consume_wake_press(gg_input_t *in, gg_button_t b, uint32_t now_ms);

/* Feed one sample. Returns the action produced by this sample, or GG_NAV_NONE. */
gg_nav_t gg_input_sample(gg_input_t *in, gg_button_t b, bool pressed, uint32_t now_ms);

/* True while any button is pressed or still settling: keep sampling. */
bool gg_input_busy(const gg_input_t *in);
