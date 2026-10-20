/* All user-visible strings (docs/ui/design.md). One column per language. */
#include "ui_priv.h"

static const char *const k_languages[] = {"vi", NULL};

static const char *const k_tags[] = {
    "weekday.0",
    "weekday.1",
    "weekday.2",
    "weekday.3",
    "weekday.4",
    "weekday.5",
    "weekday.6",
    "lunar.suffix",
    "lunar.leap",
    "badge.unsynced",
    "menu.title",
    "menu.sysinfo",
    "sysinfo.title",
    "sysinfo.version",
    "sysinfo.power",
    "sysinfo.power.usb",
    "sysinfo.power.battery",
    "sysinfo.vsys",
    "sysinfo.battery",
    NULL,
};

static const char *const k_translations[] = {
    "Chủ Nhật",
    "Thứ Hai",
    "Thứ Ba",
    "Thứ Tư",
    "Thứ Năm",
    "Thứ Sáu",
    "Thứ Bảy",
    "ÂL",
    "N",
    "Chưa đồng bộ",
    "Menu",
    "Thông tin hệ thống",
    "Thông tin hệ thống",
    "Phiên bản",
    "Nguồn",
    "USB",
    "Pin",
    "VSYS",
    "Pin còn",
};

_Static_assert(GG_ARRAY_LEN(k_translations) == GG_ARRAY_LEN(k_tags) - 1,
               "every tag needs one translation per language");

void ui_i18n_init(void)
{
    lv_translation_add_static(k_languages, k_tags, k_translations);
    lv_translation_set_language("vi");
}
