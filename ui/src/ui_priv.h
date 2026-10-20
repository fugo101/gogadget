/* Internal to ui/: theme, fonts and the per-Screen modules. */
#pragma once

#include "gg/gg_app.h"
#include "lvgl.h"
#include "ui/ui.h"

#include "gg_fonts.h"

/* Provisional palette (docs/ui/design.md); approve on the real panel. */
#define UI_COLOR_BG lv_color_hex(0x000000)
#define UI_COLOR_TEXT lv_color_hex(0xFFFFFF)
#define UI_COLOR_TEXT_DIM lv_color_hex(0xA0A0A0)
#define UI_COLOR_ACCENT lv_color_hex(0x3FA9F5)
#define UI_COLOR_GHOST lv_color_hex(0x262626) /* unlit seven-segment ghosts, idle track */
#define UI_COLOR_WARN lv_color_hex(0xFFB000)
#define UI_COLOR_ERROR lv_color_hex(0xFF4040)

void ui_i18n_init(void);

/* A plain black Screen with no scrolling or padding. */
lv_obj_t *ui_screen_create(void);
lv_obj_t *ui_label_create(lv_obj_t *parent, const lv_font_t *font, lv_color_t color);
/* lv_label_set_text() only when the text differs, so unchanged frames stay clean. */
void ui_label_set(lv_obj_t *label, const char *text);

lv_obj_t *ui_watchface_create(void);
void ui_watchface_update(const gg_state_t *s);

lv_obj_t *ui_menu_create(void);
void ui_menu_update(const gg_state_t *s);

lv_obj_t *ui_sysinfo_create(const ui_config_t *cfg);
void ui_sysinfo_update(const gg_state_t *s);

lv_obj_t *ui_testpattern_create(void);
