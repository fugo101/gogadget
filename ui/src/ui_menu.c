#include "ui_priv.h"

static const char *const k_item_tags[GG_MENU_COUNT] = {
    [GG_MENU_SYSINFO] = "menu.sysinfo",
};

static struct {
    lv_obj_t *items[GG_MENU_COUNT];
} m;

lv_obj_t *ui_menu_create(void)
{
    lv_obj_t *scr = ui_screen_create();

    lv_obj_t *title = ui_label_create(scr, &gg_font_text_20, UI_COLOR_TEXT_DIM);
    lv_label_set_text(title, lv_tr("menu.title"));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

    for (int i = 0; i < GG_MENU_COUNT; i++) {
        lv_obj_t *row = ui_label_create(scr, &gg_font_text_20, UI_COLOR_TEXT);
        lv_label_set_text(row, lv_tr(k_item_tags[i]));
        lv_obj_set_width(row, UI_HOR_RES - 24);
        lv_obj_set_style_pad_hor(row, 8, 0);
        lv_obj_set_style_pad_ver(row, 4, 0);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_align(row, LV_ALIGN_TOP_LEFT, 12, 40 + i * 34);
        m.items[i] = row;
    }
    return scr;
}

void ui_menu_update(const gg_state_t *s)
{
    for (int i = 0; i < GG_MENU_COUNT; i++) {
        bool focused = i == s->menu_focus;
        lv_obj_set_style_bg_color(m.items[i], UI_COLOR_ACCENT, 0);
        lv_obj_set_style_bg_opa(m.items[i], focused ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(m.items[i], focused ? UI_COLOR_BG : UI_COLOR_TEXT, 0);
    }
}
