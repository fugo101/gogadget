/* Watchface v1 (docs/ui/design.md). Layout is provisional until a variant is chosen. */
#include <stdio.h>

#include "ui_priv.h"

static struct {
    lv_obj_t *root; /* moved by the Always-on pixel shift */
    lv_obj_t *power;
    lv_obj_t *wifi;
    lv_obj_t *badge;
    lv_obj_t *time;
    lv_obj_t *seconds;
    lv_obj_t *date;
    lv_obj_t *lunar;
} w;

/* A transparent flex row spanning the width; children keep their content size. */
static lv_obj_t *row_create(lv_obj_t *parent, lv_flex_align_t main, lv_flex_align_t cross)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, main, cross, cross);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_scrollable(row, false);
    return row;
}

lv_obj_t *ui_watchface_create(void)
{
    lv_obj_t *scr = ui_screen_create();

    w.root = lv_obj_create(scr);
    lv_obj_remove_style_all(w.root);
    lv_obj_set_size(w.root, UI_HOR_RES, UI_VER_RES);
    lv_obj_set_scrollable(w.root, false);

    /* Status bar: power, Wi-Fi, then the badge pushed to the right edge. */
    lv_obj_t *status = row_create(w.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, 10, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 4);
    w.power = ui_label_create(status, &gg_font_text_16, UI_COLOR_TEXT);
    w.wifi = ui_label_create(status, &gg_font_text_16, UI_COLOR_TEXT_DIM);
    w.badge = ui_label_create(status, &gg_font_text_16, UI_COLOR_WARN);
    lv_obj_set_flex_grow(w.badge, 1);
    lv_obj_set_style_text_align(w.badge, LV_TEXT_ALIGN_RIGHT, 0);

    /* Time: HH:MM with the seconds sitting on the same bottom line. */
    lv_obj_t *clock = row_create(w.root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(clock, 8, 0);
    lv_obj_align(clock, LV_ALIGN_CENTER, 0, -4);
    w.time = ui_label_create(clock, &gg_font_digits_70, UI_COLOR_TEXT);
    w.seconds = ui_label_create(clock, &gg_font_digits_28, UI_COLOR_TEXT_DIM);

    /* Dates: solar on the left, lunar on the right. */
    lv_obj_t *dates = row_create(w.root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END);
    lv_obj_align(dates, LV_ALIGN_BOTTOM_MID, 0, -8);
    w.date = ui_label_create(dates, &gg_font_text_20, UI_COLOR_TEXT);
    w.lunar = ui_label_create(dates, &gg_font_text_16, UI_COLOR_TEXT_DIM);

    return scr;
}

static const char *power_text(const gg_status_t *st, char *buf, size_t len)
{
    if (st->usb) {
        return LV_SYMBOL_USB;
    }
    if (!st->battery_known) {
        return LV_SYMBOL_BATTERY_EMPTY;
    }
    const char *icon = st->battery_pct > 80   ? LV_SYMBOL_BATTERY_FULL
                       : st->battery_pct > 55 ? LV_SYMBOL_BATTERY_3
                       : st->battery_pct > 30 ? LV_SYMBOL_BATTERY_2
                       : st->battery_pct > 10 ? LV_SYMBOL_BATTERY_1
                                              : LV_SYMBOL_BATTERY_EMPTY;
    snprintf(buf, len, "%s %u%%", icon, st->battery_pct);
    return buf;
}

void ui_watchface_update(const gg_state_t *s)
{
    char buf[48];

    lv_obj_set_pos(w.root, s->shift_x, s->shift_y);

    ui_label_set(w.power, power_text(&s->status, buf, sizeof(buf)));

    switch (s->status.wifi) {
    case GG_WIFI_CONNECTED:
        lv_obj_set_style_text_color(w.wifi, UI_COLOR_TEXT, 0);
        ui_label_set(w.wifi, LV_SYMBOL_WIFI);
        break;
    case GG_WIFI_CONNECTING:
        lv_obj_set_style_text_color(w.wifi, UI_COLOR_TEXT_DIM, 0);
        ui_label_set(w.wifi, LV_SYMBOL_WIFI);
        break;
    case GG_WIFI_FAILED:
        lv_obj_set_style_text_color(w.wifi, UI_COLOR_WARN, 0);
        ui_label_set(w.wifi, LV_SYMBOL_WIFI);
        break;
    default:
        ui_label_set(w.wifi, "");
        break;
    }

    ui_label_set(w.badge, s->status.unsynced || !s->clock.valid ? lv_tr("badge.unsynced") : "");

    const gg_clock_t *c = &s->clock;
    if (!c->valid) {
        ui_label_set(w.time, "--:--");
        ui_label_set(w.seconds, "--");
        ui_label_set(w.date, "");
        ui_label_set(w.lunar, "");
        return;
    }

    snprintf(buf, sizeof(buf), "%02u:%02u", c->now.hour, c->now.minute);
    ui_label_set(w.time, buf);
    snprintf(buf, sizeof(buf), "%02u", c->now.second);
    ui_label_set(w.seconds, buf);

    char tag[16];
    snprintf(tag, sizeof(tag), "weekday.%u", c->now.weekday);
    snprintf(buf, sizeof(buf), "%s, %02u/%02u/%04d", lv_tr(tag), c->now.day, c->now.month,
             c->now.year);
    ui_label_set(w.date, buf);

    if (c->lunar_valid) {
        /* Leap months get a short marker ("3/6N ÂL") so the row never overflows. */
        snprintf(buf, sizeof(buf), "%u/%u%s %s", c->lunar_day, c->lunar_month,
                 c->lunar_leap ? lv_tr("lunar.leap") : "", lv_tr("lunar.suffix"));
        ui_label_set(w.lunar, buf);
    } else {
        ui_label_set(w.lunar, "");
    }
}
