/*
 * The UI renders gg_state_t and nothing else (ADR-0004). Call both functions
 * only from the task that owns LVGL.
 */
#pragma once

#include "gg/gg_app.h"
#include "lvgl.h"

#define UI_HOR_RES 320
#define UI_VER_RES 170

typedef struct {
    const char *version; /* firmware version shown in System info */
} ui_config_t;

/* Builds every Screen for `disp` (320x170, landscape). */
void ui_init(lv_display_t *disp, const ui_config_t *cfg);

/* Brings the widgets in line with `s`; cheap when nothing visible changed. */
void ui_render(const gg_state_t *s);
