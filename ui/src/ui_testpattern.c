/*
 * Hardware acceptance pattern (docs/hardware/bring-up.md, DSP-1/DSP-2): a 1-px
 * frame on every edge, pure R/G/B/white/grey bars and an asymmetric "F" near
 * the top-left corner. It checks the 35-row gap, colour order and byte swap,
 * inversion and orientation on the real panel.
 */
#include "ui_priv.h"

static void rect(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h, lv_color_t c)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, c, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

lv_obj_t *ui_testpattern_create(void)
{
    lv_obj_t *scr = ui_screen_create();
    const lv_color_t white = lv_color_hex(0xFFFFFF);

    rect(scr, 0, 0, UI_HOR_RES, 1, white);
    rect(scr, 0, UI_VER_RES - 1, UI_HOR_RES, 1, white);
    rect(scr, 0, 0, 1, UI_VER_RES, white);
    rect(scr, UI_HOR_RES - 1, 0, 1, UI_VER_RES, white);

    static const uint32_t k_bars[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF, 0x808080};
    const int32_t bar_w = (UI_HOR_RES - 40) / 5;
    for (int i = 0; i < 5; i++) {
        rect(scr, 20 + i * bar_w, 50, bar_w, 90, lv_color_hex(k_bars[i]));
    }

    lv_obj_t *marker = ui_label_create(scr, &gg_font_text_20, white);
    lv_label_set_text(marker, "F");
    lv_obj_set_pos(marker, 10, 10);

    lv_obj_t *hint = ui_label_create(scr, &gg_font_text_16, UI_COLOR_TEXT_DIM);
    lv_label_set_text(hint, "R G B W 50%");
    lv_obj_set_pos(hint, 40, 14);
    return scr;
}
