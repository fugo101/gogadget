/*
 * Watchface v1, variant C "minimal" (docs/ui/design.md): huge seven-segment
 * HH:MM over faint "88:88" ghost segments, seconds as a thin progress line,
 * one compact date line, status icons in the top-right corner.
 */
#include <stdio.h>

#include "ui_priv.h"

#define TIME_Y_OFS (-10)
#define BAR_X 8
#define BAR_Y 122
#define BAR_W (UI_HOR_RES - 2 * BAR_X)
#define BAR_H 3

static struct {
    lv_obj_t *root; /* moved by the Always-on pixel shift */
    lv_obj_t *badge;
    lv_obj_t *status;
    lv_obj_t *time;
    lv_obj_t *progress;
    lv_obj_t *date;
} w;

static lv_obj_t *bar_create(lv_obj_t *parent, lv_color_t color)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, BAR_X, BAR_Y);
    lv_obj_set_size(bar, BAR_W, BAR_H);
    lv_obj_set_style_bg_color(bar, color, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    return bar;
}

lv_obj_t *ui_watchface_create(void)
{
    lv_obj_t *scr = ui_screen_create();

    w.root = lv_obj_create(scr);
    lv_obj_remove_style_all(w.root);
    lv_obj_set_size(w.root, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_scrollable(w.root, false);

    w.badge = ui_label_create(w.root, &gg_font_text_16, UI_COLOR_WARN);
    lv_obj_set_pos(w.badge, 10, 4);

    w.status = ui_label_create(w.root, &gg_font_text_16, UI_COLOR_TEXT_DIM);
    lv_obj_align(w.status, LV_ALIGN_TOP_RIGHT, -10, 4);

    /* Every DSEG7 glyph used here has the same advance, so the ghost and the
     * time line up exactly when both are centred. */
    lv_obj_t *ghost = ui_label_create(w.root, &gg_font_digits_88, UI_COLOR_GHOST);
    lv_label_set_text(ghost, "88:88");
    lv_obj_align(ghost, LV_ALIGN_CENTER, 0, TIME_Y_OFS);
    w.time = ui_label_create(w.root, &gg_font_digits_88, UI_COLOR_TEXT);
    lv_obj_align(w.time, LV_ALIGN_CENTER, 0, TIME_Y_OFS);

    bar_create(w.root, UI_COLOR_GHOST); /* idle track */
    w.progress = bar_create(w.root, UI_COLOR_ACCENT);

    w.date = ui_label_create(w.root, &gg_font_text_20, UI_COLOR_TEXT);
    lv_obj_align(w.date, LV_ALIGN_BOTTOM_MID, 0, -10);

    return scr;
}

static const char *power_icon(const gg_status_t *st)
{
    if (st->usb) {
        return LV_SYMBOL_USB;
    }
    if (!st->battery_known) {
        return LV_SYMBOL_BATTERY_EMPTY;
    }
    return st->battery_pct > 80   ? LV_SYMBOL_BATTERY_FULL
           : st->battery_pct > 55 ? LV_SYMBOL_BATTERY_3
           : st->battery_pct > 30 ? LV_SYMBOL_BATTERY_2
           : st->battery_pct > 10 ? LV_SYMBOL_BATTERY_1
                                  : LV_SYMBOL_BATTERY_EMPTY;
}

static void update_status(const gg_status_t *st)
{
    char buf[24];
    bool wifi = st->wifi != GG_WIFI_OFF;
    snprintf(buf, sizeof(buf), "%s%s%s", wifi ? LV_SYMBOL_WIFI : "", wifi ? "  " : "",
             power_icon(st));
    ui_label_set(w.status, buf);
    lv_obj_set_style_text_color(w.status,
                                st->wifi == GG_WIFI_FAILED ? UI_COLOR_WARN : UI_COLOR_TEXT_DIM, 0);
}

void ui_watchface_update(const gg_state_t *s)
{
    char buf[64];
    const gg_clock_t *c = &s->clock;

    lv_obj_set_pos(w.root, s->shift_x, s->shift_y);
    update_status(&s->status);
    ui_label_set(w.badge, s->status.unsynced || !c->valid ? lv_tr("badge.unsynced") : "");

    if (!c->valid) {
        ui_label_set(w.time, "--:--");
        lv_obj_set_hidden(w.progress, true);
        ui_label_set(w.date, "");
        return;
    }

    snprintf(buf, sizeof(buf), "%02u:%02u", c->now.hour, c->now.minute);
    ui_label_set(w.time, buf);

    lv_obj_set_hidden(w.progress, false);
    lv_obj_set_width(w.progress, BAR_W * c->now.second / 60);

    char tag[16];
    snprintf(tag, sizeof(tag), "weekday.%u", c->now.weekday);
    int n = snprintf(buf, sizeof(buf), "%s %u/%u", lv_tr(tag), c->now.day, c->now.month);
    if (c->lunar_valid && n > 0 && (size_t)n < sizeof(buf)) {
        /* Leap months get a short marker ("3/6N ÂL") so the line never overflows. */
        snprintf(buf + n, sizeof(buf) - (size_t)n, "  ·  %u/%u%s %s", c->lunar_day, c->lunar_month,
                 c->lunar_leap ? lv_tr("lunar.leap") : "", lv_tr("lunar.suffix"));
    }
    ui_label_set(w.date, buf);
}
