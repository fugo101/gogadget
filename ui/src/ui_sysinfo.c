#include <stdio.h>

#include "ui_priv.h"

static struct {
    lv_obj_t *version;
    lv_obj_t *power;
    lv_obj_t *vsys;
} si;

lv_obj_t *ui_sysinfo_create(const ui_config_t *cfg)
{
    lv_obj_t *scr = ui_screen_create();

    lv_obj_t *title = ui_label_create(scr, &gg_font_text_20, UI_COLOR_TEXT_DIM);
    lv_label_set_text(title, lv_tr("sysinfo.title"));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 12, 8);

    si.version = ui_label_create(scr, &gg_font_text_16, UI_COLOR_TEXT);
    lv_label_set_text_fmt(si.version, "%s: %s", lv_tr("sysinfo.version"), cfg->version);
    lv_obj_align(si.version, LV_ALIGN_TOP_LEFT, 12, 44);

    si.power = ui_label_create(scr, &gg_font_text_16, UI_COLOR_TEXT);
    lv_obj_align(si.power, LV_ALIGN_TOP_LEFT, 12, 72);

    si.vsys = ui_label_create(scr, &gg_font_text_16, UI_COLOR_TEXT);
    lv_obj_align(si.vsys, LV_ALIGN_TOP_LEFT, 12, 100);

    return scr;
}

void ui_sysinfo_update(const gg_state_t *s)
{
    char buf[64];
    const gg_status_t *st = &s->status;

    if (st->usb) {
        snprintf(buf, sizeof(buf), "%s: %s", lv_tr("sysinfo.power"), lv_tr("sysinfo.power.usb"));
    } else if (st->battery_known) {
        snprintf(buf, sizeof(buf), "%s: %s, %s %u%%", lv_tr("sysinfo.power"),
                 lv_tr("sysinfo.power.battery"), lv_tr("sysinfo.battery"), st->battery_pct);
    } else {
        snprintf(buf, sizeof(buf), "%s: %s", lv_tr("sysinfo.power"),
                 lv_tr("sysinfo.power.battery"));
    }
    ui_label_set(si.power, buf);

    snprintf(buf, sizeof(buf), "%s: %u mV", lv_tr("sysinfo.vsys"), st->vsys_mv);
    ui_label_set(si.vsys, buf);
}
