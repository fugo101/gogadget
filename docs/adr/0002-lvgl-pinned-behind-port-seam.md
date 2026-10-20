# Pin LVGL 9.6.0 exactly and keep esp_lvgl_port behind a thin seam

The UI uses LVGL 9.6.0, pinned to the exact version in both the firmware manifest and the simulator, because golden screenshots (ADR-0005) only stay meaningful when device and host render with identical code; esp_lvgl_port accepts any LVGL from 8 up to 10, so without a pin a routine dependency refresh would silently change pixels. `espressif/esp_lvgl_port` 2.9.0 provides the display/flush/tick glue on the device, but only `firmware/` touches it: it is build-verified on IDF 6.1 with LVGL 9.6 but not hardware-tested by Espressif on i80 panels, and it has known gaps (a white frame before the first app frame, `trans_size` ignored under LVGL 9, a 3-button-only navigation helper), so it must stay replaceable by a few hundred lines of our own glue.

## Consequences

- One Kconfig fragment, `config/lvgl.defconfig`, configures LVGL for both builds (firmware SDKCONFIG_DEFAULTS and the simulator's `LV_BUILD_USE_KCONFIG`), so the two cannot drift; CI checks that the simulator's LVGL tag matches `firmware/dependencies.lock`.
- LVGL 9.6 is the last 9.x release and deprecates several widgets (`lv_list`, `lv_win`, `lv_menu`); new UI code uses flex layouts and plain objects instead.
- `LV_DISABLE_API_MAPPING` stays off because esp_lvgl_port 2.9 still calls the old `lv_draw_sw_*` names.
