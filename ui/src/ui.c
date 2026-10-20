#include <string.h>

#include "ui_priv.h"

static lv_obj_t *s_screens[GG_SCREEN_COUNT];
static gg_screen_t s_active = GG_SCREEN_COUNT;

lv_obj_t *ui_screen_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_style_bg_color(scr, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    return scr;
}

lv_obj_t *ui_label_create(lv_obj_t *parent, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_label_set_text(label, "");
    return label;
}

void ui_label_set(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

void ui_init(lv_display_t *disp, const ui_config_t *cfg)
{
    lv_display_set_default(disp);
    ui_i18n_init();

    s_screens[GG_SCREEN_WATCHFACE] = ui_watchface_create();
    s_screens[GG_SCREEN_MENU] = ui_menu_create();
    s_screens[GG_SCREEN_SYSINFO] = ui_sysinfo_create(cfg);
    s_screens[GG_SCREEN_TESTPATTERN] = ui_testpattern_create();
    s_active = GG_SCREEN_COUNT;
}

void ui_render(const gg_state_t *s)
{
    switch (s->screen) {
    case GG_SCREEN_WATCHFACE:
        ui_watchface_update(s);
        break;
    case GG_SCREEN_MENU:
        ui_menu_update(s);
        break;
    case GG_SCREEN_SYSINFO:
        ui_sysinfo_update(s);
        break;
    case GG_SCREEN_TESTPATTERN:
        break; /* static */
    default:
        return;
    }
    if (s->screen != s_active) {
        s_active = s->screen;
        lv_screen_load(s_screens[s_active]);
    }
}
