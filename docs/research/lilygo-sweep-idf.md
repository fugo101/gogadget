# LilyGO sweep: LilyGo-Display-IDF

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 4 of 29 items were verified in lilygo-verification.md (3 confirmed / 1 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): 16 MHz is above the ST7789V write-cycle limit (about 15 MHz), so it is not a fallback (K12); local time is computed in core/ from the fixed UTC+7 offset, so no TZ string needs testing under PicolibC (K23).

Superseded by decision: CMD_1's leading SLPOUT + 120 ms is kept (ADR-0006), so the K04 experiment and reusable item 1's note on removing it are not adopted; the sleep entry (K06) sends SLPIN without DISPOFF and parks every LCD pin ([power-budget.md](../hardware/power-budget.md#deep-sleep-entry-and-exit)); the toolchain pins sit in the component manifests under `ui/` and `firmware/` plus `firmware/dependencies.lock`, not `main/idf_component.yml` (K02).

Scope: LilyGO's own ESP-IDF example repo as a reference for IDF 6.1 + LVGL 9.6. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

### LilyGo-Display-IDF as an IDF reference for T-Display S3: verdict

This is the ESP-IDF example repo LilyGo itself points to (T-Display-S3 README §5, added 2026-07-17). Its T-Display-S3 path, though, has not changed since **2024-01-31** (built for IDF 5.3-dev, LVGL `^8.3.11` with no version pin). Later commits only touched other boards and setup.py. Until PR #18 (Nov 2025), setup.py silently built the AMOLED config when you picked option 7; the maintainer replied "I didn't find this problem in my tests". The code **does not compile on IDF 6.x or LVGL 9.x**. What it is good for: board facts and the panel tuning table, not its code structure. **Nothing in it contradicts the known list.** Pins, GPIO15 being set before LCD init, the 10 MHz clock, the CMD_1 table being sent every time, and the USB-Serial-JTAG console all check out.

### What it does well

- Uses native esp_lcd i80 with the built-in ST7789 driver, async DMA flush through `on_color_trans_done`, and two draw buffers.
- Holds RD (GPIO9) high. Orientation is `swap_xy(true)` plus `mirror(false,true)` with gap `(0,35)`. The gap is the same on both sides, so USB-left and USB-right both work.
- **Sends DISPON explicitly.** This has been required since IDF 5.0, when the ST7789 init stopped doing it. LilyGo only fixed this in the Arduino factory example on 2026-07-28.
- Uses the new I2C master driver and the USB-Serial-JTAG console.

### Outdated, or not worth copying

- **IDF 6.0 removed fields:** `psram_trans_align`/`sram_trans_align` become `dma_burst_size`. `color_space` becomes `rgb_ele_order` (with new `data_endian`). GPIO fields are now `gpio_num_t`. `esp_dma_utils.h` moved to esp_driver_dma. Default warnings now fail the build.
- **LVGL 8 code:** the LVGL 8 calls (`lv_disp_drv_t`, `lv_disp_draw_buf_init`, sized in pixels) have no LVGL 9.6 equivalent mapped. `lv_display_set_buffers` takes **bytes**.
- **LVGL settings:** `LV_COLOR_16_SWAP` and `LV_USE_USER_DATA` no longer exist. `LV_MEM_SIZE_KILOBYTES=48` is overridden by `LV_MEM_SIZE`, so the pool becomes 64 KB.
- **Draw buffers in PSRAM at 10 MHz** with the default 40 MHz PSRAM. Espressif's own IDF 6.1 i80 example lowers the pixel clock to **2 MHz** when buffers are in PSRAM on S3. So Gogadget's internal DMA buffers at 10 MHz is the pairing Espressif defaults to.
- **LVGL task setup:** idle priority, 4 KB stack, 2 ms tick, `-Og` build, 100 Hz FreeRTOS tick (so `pdMS_TO_TICKS(1)` is 0). This may explain issue #8 ("very slow"), which nobody from LilyGo answered.
- **sdkconfig and fonts:** the sdkconfig is a template, identical to the S3-Pro file except one line. The 19 Montserrat sizes are ASCII-only, with no Vietnamese letters.

### Porting facts (IDF 6.1 + esp_lvgl_port 2.9 + LVGL 9.6)

- `lvgl_port_add_disp()` replaces your `on_color_trans_done`. It also re-applies swap/mirror from `.rotation`, so set `{swap_xy=1, mirror_x=0, mirror_y=1}`.
- **Byte order:** use the i80 flag `swap_color_bytes=1`. The LCD peripheral swaps colour bytes in hardware, at no CPU cost. Avoid `swap_bytes`, which swaps on the CPU every flush (`lv_draw_sw_rgb565_swap`, renamed in 9.6 but kept as an alias). This keeps LVGL buffers in plain RGB565, so simulator screenshots match what the device renders.
- `esp_lcd_panel_disp_on_off()` waits for queued colour DMA to finish. Calling it right after `lv_refr_now()` therefore turns the display on with a complete first frame. Then ramp up the AW9364.
- A cold init spends about 240 ms in fixed waits. `esp_lcd_panel_disp_sleep()` (SLPIN) adds another 100 ms.

### Wake latency

Issue #4's boot log comes from a sibling board with the same ESP32-S3R8 and 8 MB octal PSRAM:
- **App image load:** about 180 ms with flash in DIO mode.
- **PSRAM memory test:** about 735 ms.
- **app_main reached:** at 1.1 s.

To cut this: `SPIRAM_MEMTEST=n` (or no PSRAM at all), QIO flash (LilyGo's own board file uses QIO at 80 MHz), and optionally `BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP`.

### IDF 6 APIs the clock needs

- **Button wake:** `esp_sleep_enable_ext1_wakeup_io(BIT64(0)|BIT64(14), ESP_EXT1_WAKEUP_ANY_LOW)`.
- **Wake reason:** `esp_sleep_get_wakeup_causes()`; the old single-cause getter is deprecated.
- **Battery ADC:** the legacy ADC driver is removed.
- **libc:** PicolibC is now the default.
- **USB always-on with auto light sleep:** set `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION`.
- **Simulator tests:** LVGL 9.6 includes `LV_USE_TEST_SCREENSHOT_COMPARE`.

### Reusable code

- The CMD_1 table (MIT).
- A rewritten display bring-up (MIT plus CC0).
- esp_lvgl_port registration with the first-frame and sleep sequence.
- Espressif's light-sleep LCD pattern (CC0).

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| K01 | Repo role and freshness | high | refines | partially |
| K02 | setup.py behaviour and reproducibility | medium | new |  |
| K03 | Pins and board init order in the IDF repo | high | confirms |  |
| K04 | CMD_1 init table: content and how it is sent | high | refines | confirmed |
| K05 | DISPON is no longer part of esp_lcd ST7789 init | high | new |  |
| K06 | Fixed delays and the SLPIN API in IDF 6.1 | high | refines | confirmed |
| K07 | IDF 6.x LCD API breaks in display_s3.c (and LilyGo's Arduino code) | high | new |  |
| K08 | LVGL 8 -> 9.6 API and Kconfig breaks | high | new |  |
| K09 | esp_lvgl_port 2.9 behaviours that change the port | high | new |  |
| K10 | RGB565 byte-order options on IDF 6.1 | high | new |  |
| K11 | PSRAM draw buffers vs pixel clock | high | refines | confirmed |
| K12 | Pixel-clock arithmetic and vendor-used clocks | medium | new |  |
| K13 | i80 driver vs power management, light sleep, DMA alignment | medium | new |  |
| K14 | sdkconfig.defaults.t-display-s3 decoded for IDF 6.1 / LVGL 9.6 | high | new |  |
| K15 | Boot-time budget from a LilyGo-Display-IDF boot log | high | new |  |
| K16 | LVGL task loop in main.cpp and issue #8 | medium | new |  |
| K17 | USB-Serial-JTAG console and power management | medium | refines |  |
| K18 | Backlight handling and power-up order in the IDF repo | medium | new |  |
| K19 | Orientation and gap | medium | new |  |
| K20 | Touch handling (not needed for the non-touch board) | low | new |  |
| K21 | I2C master driver usage | low | new |  |
| K22 | IDF 6 sleep/wake APIs Gogadget needs (absent from the LilyGo repo) | high | new |  |
| K23 | Other IDF 6.0/6.1 platform changes relevant to the port | medium | new |  |
| K24 | Fonts: built-in Montserrat cannot render Vietnamese | high | new |  |
| K25 | LVGL 9.6 built-in screenshot-test utilities | medium | new |  |
| K26 | The per-board sdkconfig files are boilerplate | low | new |  |
| K27 | Espressif's IDF 6.1 i80_controller example as a modern template | medium | new |  |
| K28 | RTC slow-clock choices without a 32 kHz crystal | medium | refines |  |
| K29 | Bundled components | low | new |  |

### K01: Repo role and freshness

Relevance: high. Novelty: refines. Verification: partially (see lilygo-verification.md, `idf:K01`).

**Fact:** LilyGo-Display-IDF (MIT, 15 commits 2024-01-31..2025-11-03, 70 stars, 19 forks, 9 open issues) is the IDF reference LilyGo points T-Display-S3 users to, but every T-Display-S3 file (display_s3.c, main.cpp, tft_driver.h, i2c_driver.c, touch_driver.cpp, sdkconfig.defaults.t-display-s3, the S3 block of product_pins.h) is unchanged since the first commit; later commits only added T-Watch S3/2019, a T4-S3 offset, a T-RGB touch fix, XPowersLib bumps and setup.py fixes. README targets IDF v5.3.0 (then the master branch, per #5) on Ubuntu 22.04; LVGL comes from add-dependency lvgl/lvgl^8.3.11. Maintainer stance: "I can't do more, all I can do is demonstrate how to use IDF" (#3); the Component Registry BSP request (#16) is unanswered. The ESP Component Registry has no T-Display-S3 BSP; the only LilyGo-engineer component (llgok = 'LILYGO_L', 24 commits in T-Display-S3), llgok/lilygo_device_driver 2.1.0, is GPL-3.0, needs IDF>=5.5.3 and only enables T-Display-P4.

**Evidence:** git log per file -> only c465092ee2 (2024-01-31) @b1a1cc5; T-Display-S3 README.md:122-124 @1cc41eb (added 6fe4d51, 2026-07-17); LilyGo-Display-IDF README.md:57-59 and setup.py:182 @b1a1cc5; issue #3 comment lewisxhe 2024-02-20; issue #5 comments lewisxhe 2024-03-16/20; issue #16; gh api repos/Xinyuan-LilyGO/LilyGo-Display-IDF; components.espressif.com/api/components/llgok/lilygo_device_driver (2.1.0, license GPL-3.0, deps only if LILYGO_DEVICE_DRIVER_T_DISPLAY_P4); gh api users/Llgok

**Action:** Treat it as a 2024 LVGL-8/IDF-5.3 snapshot: harvest board facts and the panel table, write Gogadget's display layer fresh against IDF 6.1; do not depend on lilygo_device_driver (GPL, wrong board).

### K02: setup.py behaviour and reproducibility

Relevance: medium. Novelty: new.

**Fact:** setup.py: git submodule init/update -> deletes main/idf_component.yml, sdkconfig and build/ -> checks idf.py -> menu 1-16 (7 = T-Display-S3, shared by touch and non-touch) -> copies sdkconfig.defaults.t-display-s3 to sdkconfig.defaults -> idf.py set-target esp32s3 -> idf.py add-dependency "lvgl/lvgl^8.3.11" -> reconfigure -> build. Until 98132dd (PR #18, merged 2025-11-03) set-target ran before the copy, so option 7 built CONFIG_LILYGO_T_DISPLAY_S3_AMOLED (the Kconfig default); maintainer: "Okay, I didn't find this problem in my tests." sdkconfig.defaults, idf_component.yml and dependencies.lock are .gitignored, so the LVGL version floats (^8.3.11 now resolves to 8.4.0). Community workaround (#15): idf.py set-target esp32s3, then idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults.<board>" build.

**Evidence:** setup.py:10-27,116-121,182-186 @b1a1cc5; commit 98132dd0e7 and PR #18 (artisdom build log 2025-10-31; lewisxhe reply 2025-11-03); main/Kconfig.projbuild:2-4; .gitignore:8-14; issue #15 comment sukesh-ak 2025-04-06

**Action:** Commit sdkconfig.defaults, main/idf_component.yml (lvgl/lvgl 9.6.0 - registry revision 9.6.0~1 of 2026-09-17 - and espressif/esp_lvgl_port 2.9.0) and dependencies.lock; no generator script; set the target once.

### K03: Pins and board init order in the IDF repo

Relevance: high. Novelty: confirms.

**Fact:** product_pins.h T-Display-S3 block matches the known pins (POWERON 15, BL 38, D0-D7 39/40/41/42/45/46/47/48, RST 5, CS 6, DC 7, WR 8, RD 9, I2C SCL 17 / SDA 18) and adds TOUCH_IRQ 16 / TOUCH_RST 21; the 170x320 panel is run as 320x170 (hor_res = AMOLED_HEIGHT). README lists both T-Display-S3 variants as ESP32-S3R8, 16 MB flash, 8 MB OPI PSRAM. RD is set up as a plain GPIO output and held high before the bus is created (the i80 peripheral drives only WR/DC/CS/D0-7). GPIO15 is driven high by power_driver_init() (non-PMU #else branch) before display_init(). Buttons and battery ADC are not defined (README: only display and touch are supported).

**Evidence:** main/product_pins.h:202-231; main/display_s3.c:64-69; main/power_driver.cpp:172-195; main/main.cpp:131-143,180-182; README.md:9,19-20 @b1a1cc5

**Action:** Keep 'RD high' and 'GPIO15 high before panel init' in Gogadget's board init; nothing to borrow for buttons/ADC.

### K04: CMD_1 init table: content and how it is sent

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `idf:K04`).

**Fact:** display_s3.c lcd_st7789v[] is byte-identical to T-Display-S3 factory.ino and lv_demos.ino: 0x11 SLPOUT (+120 ms), 3A=05, B2=0B 0B 00 33 33 (porch), B7=75, BB=28 (VCOM), C0=2C, C2=01, C3=1F, C6=13 (frame rate), D0=A7, D0=A4 A1, D6=A1, E0/E1 14-byte gamma. Bit7 of len means wait 120 ms. The IDF repo sends it unconditionally (no LCD_MODULE_CMD_1 switch) with esp_lcd_panel_io_tx_param after reset -> init -> invert(true) -> swap_xy(true) -> mirror(false,true) -> set_gap(0,35), then esp_lcd_panel_disp_on_off(true). The table repeats SLPOUT and rewrites COLMOD to 0x05 after the driver's SLPOUT(+100 ms)/COLMOD 0x55, and never touches MADCTL 0x36 or RAMCTRL 0xB0, so driver-managed orientation and endianness survive. On IDF 6.1, tx_param is synchronous, first drains in-flight colour DMA, and is limited to 32 parameter bytes.

**Evidence:** main/display_s3.c:32-47,121-145 @b1a1cc5; T-Display-S3 examples/factory/factory.ino:133-149 and examples/lv_demos/lv_demos.ino:74-90 @1cc41eb; IDF v6.1 components/esp_lcd/src/esp_lcd_panel_st7789.c:182-201; components/esp_lcd/i80/esp_lcd_panel_io_i80.c:487-559; components/esp_lcd/priv_include/esp_lcd_common.h:23

**Action:** Port the table verbatim (keep MIT attribution) and send it after init+invert; leave MADCTL to esp_lcd/esp_lvgl_port and RAMCTRL to the driver; as an experiment, drop the duplicate leading 0x11 + 120 ms.

### K05: DISPON is no longer part of esp_lcd ST7789 init

Relevance: high. Novelty: new.

**Fact:** The built-in ST7789 esp_lcd_panel_init() sent DISPON in IDF v4.4.7 but not since v5.0 (v6.1 sends only SLPOUT, 100 ms, MADCTL, COLMOD, RAMCTRL). That is the 'display issues' LilyGo fixed in the T-Display-S3 factory example for Arduino core 3.3.11 (c9b939f, 2026-07-28) by adding esp_lcd_panel_disp_on_off(panel_handle, true). LilyGo-Display-IDF already calls disp_on_off(true).

**Evidence:** IDF v4.4.7 components/esp_lcd/src/esp_lcd_panel_st7789.c:162-163 ('// turn on display' + DISPON); IDF v5.0 same file (no DISPON in init, only in disp_off at :267); IDF v6.1 esp_lcd_panel_st7789.c:182-201; T-Display-S3 commit c9b939f95676 factory.ino:+324; LilyGo-Display-IDF main/display_s3.c:145 @b1a1cc5

**Action:** Send DISPON explicitly, after the first frame is in GRAM. Because tx_param drains queued colour DMA (esp_lcd_panel_io_i80.c:504-510), calling esp_lcd_panel_disp_on_off(panel,true) right after lv_refr_now() (holding the LVGL lock) shows a complete frame; then ramp the AW9364.

### K06: Fixed delays and the SLPIN API in IDF 6.1

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `idf:K06`).

**Fact:** Cold bring-up waits: esp_lcd_panel_reset() 10 ms low + 10 ms high; esp_lcd_panel_init() SLPOUT + 100 ms; CMD_1's SLPOUT + 120 ms. That is about 240 ms before the first pixel. esp_lcd_panel_disp_sleep(panel,true) sends SLPIN and always waits 100 ms (false: SLPOUT + 100 ms). esp_lcd_panel_disp_on_off(false) sends DISPOFF. Without a reset GPIO the driver uses SWRESET + 20 ms. LilyGo-Display-IDF has no sleep code at all.

**Evidence:** IDF v6.1 components/esp_lcd/src/esp_lcd_panel_st7789.c:162-180,186-189,304-333; LilyGo-Display-IDF main/display_s3.c:33,121-143 @b1a1cc5

**Action:** Before deep sleep: backlight off -> disp_on_off(false) -> disp_sleep(true) -> GPIO15 low (the #289 263 uA state). Budget about 250 ms of panel init per wake; see the open question on skipping re-init.

### K07: IDF 6.x LCD API breaks in display_s3.c (and LilyGo's Arduino code)

Relevance: high. Novelty: new.

**Fact:** Fails to compile on IDF 6.x: (1) esp_lcd_i80_bus_config_t .psram_trans_align=64/.sram_trans_align=4 were removed; replace with .dma_burst_size (v6.1 struct: dc/wr_gpio_num, clk_src, data_gpio_nums[16], bus_width, max_transfer_bytes, dma_burst_size, flags.allow_pd). (2) esp_lcd_panel_dev_config_t .color_space=ESP_LCD_COLOR_SPACE_RGB was removed together with esp_lcd_color_space_t; replace with .rgb_ele_order=LCD_RGB_ELEMENT_ORDER_RGB (new .data_endian next to it). (3) LCD GPIO fields are gpio_num_t (GPIO_NUM_x). (4) esp_dma_utils.h (included, unused) moved to the esp_driver_dma component. (5) esp_lcd_new_panel_st7789 is declared in esp_lcd_panel_st7789.h (pulled in by esp_lcd_panel_vendor.h); NT35510 left IDF. (6) Default warnings are now errors. Still valid: LCD_CLK_SRC_DEFAULT, dc_levels incl. dc_idle_level, trans_queue_depth, lcd_cmd/param_bits, on_color_trans_done/user_ctx, tx_param, invert/swap_xy/mirror/set_gap, disp_on_off. vTaskDelay is still visible via FreeRTOS.h -> idf_additions.h. The same removals hit LilyGo's Arduino factory.ino even after the 2026-07-28 core 3.3.11 fix (.psram_trans_align/.sram_trans_align at :277-278, .color_space at :302), and lv_demos.ino has #error for IDF>=5; no LilyGo T-Display-S3 display code builds on an IDF-6 toolchain.

**Evidence:** main/display_s3.c:14,73-92,113-119 @b1a1cc5; IDF v6.1 docs/en/migration-guides/release-6.x/6.0/peripherals.rst:276-285,313-328; build-system.rst:86; components/esp_lcd/include/esp_lcd_io_i80.h:25-38,67-90; esp_lcd_panel_dev.h:20-29; esp_lcd_panel_vendor.h:9-11; components/esp_lcd/hints.yml:1-3; components/freertos/FreeRTOS-Kernel/include/freertos/FreeRTOS.h:1531; T-Display-S3 examples/factory/factory.ino:277-302 and examples/lv_demos/lv_demos.ino:3-5 @1cc41eb

**Action:** Apply the replacements (see reusable_code bring-up); compile warning-clean.

### K08: LVGL 8 -> 9.6 API and Kconfig breaks

Relevance: high. Novelty: new.

**Fact:** main.cpp's LVGL 8 API has no mapping in LVGL 9.6's lv_api_map_v8.h: lv_disp_draw_buf_t, lv_disp_draw_buf_init, lv_disp_drv_t/init/register, lv_indev_drv_t, and flush_cb(lv_disp_drv_t*, area, lv_color_t*). Only names such as lv_disp_t, lv_disp_flush_ready and lv_disp_get_hor_res survive. LVGL 9: lv_display_create(320,170); lv_display_set_buffers(disp,b1,b2,size_in_BYTES,LV_DISPLAY_RENDER_MODE_PARTIAL) (LVGL 8 took pixels); flush_cb(lv_display_t*, const lv_area_t*, uint8_t *px_map); lv_display_flush_ready(); lv_display_set_user_data(). Kconfig: CONFIG_LV_COLOR_16_SWAP and CONFIG_LV_USE_USER_DATA no longer exist. CONFIG_LV_MEM_SIZE_KILOBYTES is deprecated, and because Kconfig always defines CONFIG_LV_MEM_SIZE (default 65536) it is ignored with #warning; LilyGo's 48 KB becomes 64 KB. 9.6 renamed lv_draw_sw_rgb565_swap -> lv_draw_rgb565_swap and lv_draw_sw_rotate -> lv_draw_rotate, with compat macros that stay active unless LV_DISABLE_API_MAPPING is defined. Public headers moved to include/lvgl/..., with #warning shims left in src/.

**Evidence:** main/main.cpp:42-63,150-208 @b1a1cc5; LVGL v9.6.0 include/lvgl/api_map/lv_api_map_v8.h:56,118,134,143-144; include/lvgl/display/lv_display.h:96,259-270,316; Kconfig:106-120,310; include/lvgl/config/lv_conf_kconfig.h:222-232; include/lvgl/api_map/lv_api_map_v9_5.h (#define lv_draw_sw_rgb565_swap lv_draw_rgb565_swap); include/lvgl/lvgl.h:211-221; src/lv_conf_kconfig.h (#warning shim)

**Action:** Let esp_lvgl_port create the display. Set CONFIG_LV_MEM_SIZE in bytes (or CONFIG_LV_USE_CLIB_MALLOC=y). Include only "lvgl.h". Do not define LV_DISABLE_API_MAPPING: esp_lvgl_port 2.9 still calls the lv_draw_sw_* names.

### K09: esp_lvgl_port 2.9 behaviours that change the port

Relevance: high. Novelty: new.

**Fact:** lvgl_port_add_disp(): (a) registers its own on_color_trans_done via esp_lcd_panel_io_register_event_callbacks, so any io_config callback (LilyGo's display_notify_lvgl_flush_ready) is replaced; (b) immediately re-applies esp_lcd_panel_swap_xy()/esp_lcd_panel_mirror() from disp_cfg.rotation (ROTATION_0: swap_xy(cfg), mirror(cfg)), so LilyGo's swap_xy(true)+mirror(false,true) must be given as .rotation={1,0,1} or the panel ends up unrotated; (c) flags.swap_bytes means a software lv_draw_sw_rgb565_swap() over every flushed area; (d) with buff_dma it allocates heap_caps_aligned_alloc(CONFIG_LV_DRAW_BUF_ALIGN, px*2, MALLOC_CAP_DMA); (e) the tick is an esp_timer calling lv_tick_inc every timer_period_ms (default 5), which lvgl_port_stop()/lvgl_port_resume() stop and restart. Defaults: task prio 4, stack 7168, max sleep 500 ms; lvgl_port_lock(0) blocks forever. Registry: esp_lvgl_port 2.9.0 (2026-08-07, idf>=5.2, lvgl >=8,<10). lvgl 9.6.0~1 pulls FreeType/libpng/libjpeg-turbo/LZ4 only when enabled.

**Evidence:** espressif/esp-bsp@058d2aaa1a components/esp_lvgl_port/src/lvgl9/esp_lvgl_port_disp.c:110-135,346-360,383-390,739-742,767-800; src/lvgl9/esp_lvgl_port.c:108-130,289-306; include/esp_lvgl_port.h:52-73,100-114; idf_component.yml:1-8; components.espressif.com API (espressif/esp_lvgl_port versions; lvgl/lvgl 9.6.0~1 dependency rules)

**Action:** When porting: drop io_config.on_color_trans_done, put the orientation in .rotation, keep swap_bytes=0 if the hardware swap is used, and call lvgl_port_stop()/resume() around manual light sleep in USB mode.

### K10: RGB565 byte-order options on IDF 6.1

Relevance: high. Novelty: new.

**Fact:** LilyGo relied on LVGL 8 CONFIG_LV_COLOR_16_SWAP. IDF 6.1 offers three mutually exclusive replacements: (1) esp_lvgl_port flags.swap_bytes, done on the CPU each flush; (2) esp_lcd_panel_io_i80_config_t.flags.swap_color_bytes=1, which turns on the LCD_CAM swizzle (lcd_user.lcd_8bits_order, AB->BA) for colour transactions only (command/param phases use the swizzle only when param_bits > bus_width), with zero CPU cost; (3) esp_lcd_panel_dev_config_t.data_endian=LCD_RGB_DATA_ENDIAN_LITTLE, which makes the driver write ST7789 RAMCTRL 0xB0 = 00 F8 instead of 00 F0. LilyGo's CMD_1 never overwrites 0xB0. Espressif's own i80 example still swaps in software (lv_draw_sw_rgb565_swap in flush_cb).

**Evidence:** sdkconfig.defaults.t-display-s3:3 @b1a1cc5; IDF v6.1 components/esp_lcd/i80/esp_lcd_panel_io_i80.c:371,517-519,884-886; components/esp_hal_lcd/esp32s3/include/hal/lcd_ll.h:48-52,566-577; components/esp_lcd/src/esp_lcd_panel_st7789.c:29-30,112-117,196-198; components/esp_lcd/include/esp_lcd_io_i80.h:86; examples/peripherals/lcd/i80_controller/main/i80_controller_example_main.c:101-102

**Action:** Use option (2), or (3) after a colour check on hardware. This keeps LVGL buffers native RGB565, so desktop-simulator screenshots equal device buffers. Never combine two swaps.

### K11: PSRAM draw buffers vs pixel clock

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `idf:K11`).

**Fact:** LilyGo-Display-IDF puts both LVGL buffers (2 x 17,000 px = 2 x 34,000 B) in PSRAM and clocks the bus at 10 MHz with the default 40 MHz PSRAM. Espressif's IDF 6.1 i80_controller example defaults PCLK to 2 MHz when buffers are in PSRAM on ESP32-S3 (10 MHz otherwise). Its README warns: "Can't get a stable UI when EXAMPLE_LCD_I80_COLOR_IN_PSRAM is enabled... because of the limited PSRAM bandwidth". The suggested fixes are a lower PCLK, faster SPIRAM_SPEED, SPIRAM_XIP_FROM_PSRAM and dma_burst_size 64. LilyGo's Arduino lv_demos uses an internal DMA buffer (MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL).

**Evidence:** main/main.cpp:152-170, main/product_pins.h:229, main/display_s3.c:23,89-91 @b1a1cc5; issue #4 log 'esp_psram: Speed: 40MHz'; IDF v6.1 examples/peripherals/lcd/i80_controller/main/Kconfig.projbuild:3-15, README.md:128-130, main/i80_controller_example_main.c:79,323-330, sdkconfig.defaults.esp32s3:1-7; T-Display-S3 examples/lv_demos/lv_demos.ino:225 @1cc41eb

**Action:** Confirms Gogadget's 10 MHz + internal DMA buffers (Espressif's default pairing); do not copy LilyGo's PSRAM buffers.

### K12: Pixel-clock arithmetic and vendor-used clocks

Relevance: medium. Novelty: new.

**Fact:** On IDF 6.1/S3: LCD_CLK_SRC_DEFAULT = PLL_F160M with a fixed pre-scale of 2, giving an 80 MHz resolution and an integer divider. 10 MHz = /8 exactly. LilyGo's Arduino clocks: factory 16 MHz (/5, exact) and lv_demos 6.528 MHz, which actually runs at 6.67 MHz (/12). So LilyGo has used 6.5, 10 and 16 MHz on this panel.

**Evidence:** IDF v6.1 components/soc/esp32s3/include/soc/clk_tree_defs.h:187-190; components/esp_lcd/priv_include/esp_lcd_common.h:28; components/esp_lcd/i80/esp_lcd_panel_io_i80.c:343-364,645-660; T-Display-S3 examples/factory/pin_config.h:25 (adc038e, 2023-03-03) and examples/lv_demos/pin_config.h:14 (ed3610b, 2022-11-16) @1cc41eb

**Action:** Keep 10 MHz (exact). 16 MHz is a vendor-used ceiling if faster fills are ever needed; avoid values that are not exact divisors of 80 MHz.

### K13: i80 driver vs power management, light sleep, DMA alignment

Relevance: medium. Novelty: new.

**Fact:** With CONFIG_PM_ENABLE the i80 bus holds an ESP_PM_NO_LIGHT_SLEEP lock per transaction. flags.allow_pd must stay 0 on ESP32-S3 (no LCDCAM sleep retention; non-zero returns ESP_ERR_NOT_SUPPORTED). IDF's own 2026 i80 sleep test (ST7789, S3 pins) shows disp_on_off(false) -> esp_light_sleep_start() -> disp_on_off(true) -> drawing works without re-init. On the S3, GDMA alignment is 1 byte for TX unless CONFIG_GDMA_ENABLE_WEIGHTED_ARBITRATION is set, so odd-sized LVGL partial areas pass the tx_color alignment checks. tx_color rejects transfers larger than max_transfer_bytes.

**Evidence:** IDF v6.1 components/esp_lcd/i80/esp_lcd_panel_io_i80.c:131-136,544-557,562-577,662-669; components/esp_lcd/test_apps/i80_lcd/main/test_i80_lcd_panel_sleep.c:24-128 (CC0-1.0) and test_i80_board.h:15-28; components/esp_driver_dma/src/gdma.c:437-472; components/soc/esp32s3/include/soc/soc_caps.h (no SOC_LCDCAM_LCD_SUPPORT_SLEEP_RETENTION)

**Action:** In USB always-on mode, light sleep between frames is safe. Leave allow_pd=0, don't enable weighted arbitration, and size max_transfer_bytes to buffer_px*2.

### K14: sdkconfig.defaults.t-display-s3 decoded for IDF 6.1 / LVGL 9.6

Relevance: high. Novelty: new.

**Fact:** L1 CONFIG_LILYGO_T_DISPLAY_S3=y: project-local board choice, N/A. L2 CONFIG_LV_USE_USER_DATA=y: LVGL 8 only, absent in 9.6, drop. L3 CONFIG_LV_COLOR_16_SWAP=y: LVGL 8 only, absent in 9.6, replace with the hardware swap (K10). L4 CONFIG_LV_COLOR_DEPTH_16=y: valid, keep. L5-6 CONFIG_IDF_TARGET="esp32s3"/_ESP32S3=y: valid. L7-9 CONFIG_SPIRAM=y, CONFIG_SPIRAM_MODE_OCT=y (required for the S3R8's octal PSRAM), CONFIG_SPIRAM_TYPE_AUTO=y (default): valid in 6.1; speed stays at the 40 MHz default, SPIRAM_USE defaults to MALLOC, and the memtest stays on. L10 CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y: valid; in 6.x it lives in the esp_stdio component (esp_vfs_console was renamed). L11-12 CPU 240 MHz: valid; the 6.1 S3 default is 160 MHz. L13 CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y: valid, but flash mode stays at the IDF default DIO although LilyGo's board JSON/README use QIO 80 MHz (80M is the S3 default speed). L14 CONFIG_LV_MEM_SIZE_KILOBYTES=48: deprecated and overridden, giving 64 KB. L15-18 LV_USE_DEMO_WIDGETS/BENCHMARK/STRESS/MUSIC: valid, flash bloat. L19-37 LV_FONT_MONTSERRAT_12..48 (19 sizes): valid but ASCII-only. L38 LV_FONT_DEFAULT_MONTSERRAT_14: valid. Not set, so IDF defaults apply: FREERTOS_HZ=100, optimisation Debug (-Og), no PM, default partition table (1 MB factory app per the #4 log), LV_OS_NONE, bootloader image validation on.

**Evidence:** sdkconfig.defaults.t-display-s3:1-38 and main/Kconfig.projbuild:30-31 @b1a1cc5; LVGL v9.6.0 Kconfig:106-120,168-170,310,1677-1787,3819-3887 (no LV_COLOR_16_SWAP/LV_USE_USER_DATA symbols); IDF v6.1 components/esp_psram/esp32s3/Kconfig.spiram:18-27,83-106; components/esp_psram/Kconfig.spiram.common:62-64,89-95; components/esp_stdio/Kconfig:2-33; docs/en/migration-guides/release-6.x/6.0/storage.rst:28; components/esp_system/port/soc/esp32s3/Kconfig.cpu:1-4; components/esptool_py/Kconfig.projbuild:39-62; components/spi_flash/esp32s3/Kconfig.flash_freq (default 80M); components/freertos/Kconfig:33-37; Kconfig:409-437; T-Display-S3 boards/lilygo-t-display-s3.json:5,13-14 and README.md:104,109 @1cc41eb

**Action:** Starting point for Gogadget (measure each): CONFIG_IDF_TARGET="esp32s3", CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y, CONFIG_ESPTOOLPY_FLASHMODE_QIO=y, CONFIG_PARTITION_TABLE_CUSTOM=y, CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160=y, CONFIG_COMPILER_OPTIMIZATION_PERF=y (or _SIZE). For PSRAM, either none, or CONFIG_SPIRAM=y + CONFIG_SPIRAM_MODE_OCT=y + CONFIG_SPIRAM_MEMTEST=n. LVGL: CONFIG_LV_COLOR_DEPTH_16=y, CONFIG_LV_MEM_SIZE=<bytes> (or CONFIG_LV_USE_CLIB_MALLOC=y), no demos, only generated fonts. Drop LV_COLOR_16_SWAP, LV_USE_USER_DATA and LV_MEM_SIZE_KILOBYTES.

### K15: Boot-time budget from a LilyGo-Display-IDF boot log

Relevance: high. Novelty: new.

**Fact:** Issue #4's log is a LilyGo-Display-IDF build on a T-Display-S3 AMOLED Touch (I2C on GPIO3/2, CST816 at 0x15), which uses the same ESP32-S3R8 with 8 MB AP octal PSRAM. Bootloader in SPI mode DIO at 80 MHz; image segments load/verify from I(92) to 'Loaded app' at I(271), about 180 ms. Octal PSRAM init runs 276-341 ms. 'SPI SRAM memory test OK' arrives at I(1076), so the memtest alone takes about 735 ms. app_main starts at I(1102). The default partition table has a 1 MB factory app, and the log shows 'spi_flash: flash io: dio'.

**Evidence:** Xinyuan-LilyGO/LilyGo-Display-IDF issue #4 body (2024-02-24, ESP-IDF v5.3-dev-2087); main/product_pins.h:88-116 @b1a1cc5

**Action:** For the 10 s glance after a deep-sleep wake: CONFIG_SPIRAM_MEMTEST=n (Kconfig help: 'Disable this for slightly faster startup') or no PSRAM, QIO flash, consider CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP=y (IDF v6.1 components/bootloader/Kconfig.projbuild:322-336; trades away image integrity checks), quieter boot logs. Measure wake-to-first-pixel on hardware.

### K16: LVGL task loop in main.cpp and issue #8

Relevance: medium. Novelty: new.

**Fact:** main.cpp: esp_timer tick every 2 ms; LVGL task created at tskIDLE_PRIORITY (the priority-2 line is commented out) with a 4 KB stack; delay clamped to 1..500 ms via pdMS_TO_TICKS. With the unset default FREERTOS_HZ=100, pdMS_TO_TICKS(1)==0, so the lower clamp becomes a bare yield. A recursive mutex guards LVGL. lv_demo_music() is hard-wired (the Kconfig 'GUI Demo' choice is ignored). IDF's default -Og is unchanged. Issue #8 ('Runs very slow compared to pio Arduino version', 2024-07-10) has no maintainer reply; a community member only suggested 240 MHz.

**Evidence:** main/main.cpp:34-38,86-123,188-199,210-233 and main/Kconfig.projbuild:62-77 @b1a1cc5; IDF v6.1 components/freertos/Kconfig:33-37; IDF v6.1 Kconfig:409-437; issue #8 and comment by mehdok 2024-08-05

**Action:** Don't copy this loop. Use esp_lvgl_port (prio 4, 7168 B stack, event-group wakeups) and build with -O2/-Os. Treat 'idle priority + -Og + PSRAM buffers' as only a hypothesis for #8.

### K17: USB-Serial-JTAG console and power management

Relevance: medium. Novelty: refines.

**Fact:** The T-Display-S3's USB-C goes to the S3's native USB (LilyGo's Arduino settings: USB Mode 'CDC and JTAG'), so USJ as primary console is correct. In IDF 6.1, USB-Serial-JTAG does not work under light sleep or after waking from it. CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION (needs PM_ENABLE) holds a NO_LIGHT_SLEEP lock while the port is connected, but does not block a manual esp_light_sleep_start(). CONFIG_USJ_ENABLE_USB_SERIAL_JTAG=n would gate the module clock but removes the console.

**Evidence:** sdkconfig.defaults.t-display-s3:10 @b1a1cc5; T-Display-S3 README.md:112 @1cc41eb; IDF v6.1 components/esp_driver_usb_serial_jtag/Kconfig:4-35

**Action:** Keep the USJ console for the Wi-Fi-credential command. If USB always-on mode uses PM auto light sleep, set CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION=y. Expect the USB device to drop and re-enumerate around deep-sleep cycles.

### K18: Backlight handling and power-up order in the IDF repo

Relevance: medium. Novelty: new.

**Fact:** display_init() ends by making GPIO38 an output and driving it high: no AW9364 pulse dimming at all. main.cpp calls display_init() before lv_init(), so the backlight comes on at full level before LVGL has written a frame, and the panel shows whatever is in GRAM.

**Evidence:** main/display_s3.c:147-153; main/main.cpp:142-147 @b1a1cc5

**Action:** Gogadget order: GPIO15 high -> panel init (display off) -> build UI -> lv_refr_now() -> esp_lcd_panel_disp_on_off(true) -> own AW9364 pulse driver to the target level. Reverse the order before deep sleep.

### K19: Orientation and gap

Relevance: medium. Novelty: new.

**Fact:** swap_xy(true) + mirror(false,true) means 'screen faces you, USB on the left'; the commented alternative mirror(true,false) is USB on the right. Both use set_gap(0,35). Since 35 = (240-170)/2, the 170-row window is centred in the ST7789's 240-row GRAM and the offset is the same either way. In esp_lvgl_port, LV_DISPLAY_ROTATION_180 with rotation {1,0,1} issues swap_xy(true) + mirror(true,false), which is exactly LilyGo's USB-right variant.

**Evidence:** main/display_s3.c:127-137 @b1a1cc5; espressif/esp-bsp@058d2aaa1a components/esp_lvgl_port/src/lvgl9/esp_lvgl_port_disp.c:778-797

**Action:** Offer a 'flip 180°' setting via lv_display_set_rotation() without touching the gap; verify once on hardware.

### K20: Touch handling (not needed for the non-touch board)

Relevance: low. Novelty: new.

**Fact:** Touch and non-touch T-Display-S3 share one build with BOARD_HAS_TOUCH=1. touch_init() calls SensorLib TouchClassCST816 setPins(RST=21, IRQ=16) (driving GPIO21), begin() at the CST816 address, swapXY, mirror(true,false), disableAutoSleep(). On a non-touch board it logs 'Touch init failed!' but the LVGL pointer input is still registered. CST328, which T-Display-S3's Arduino lv_demos tries first, is not supported. Maintainer on CST auto-sleep: "This is the touch screen entering sleep state and will not respond to host polling"; the fix is disableAutoSleep (#17; same fix in 6dc65b4 for #6).

**Evidence:** main/product_pins.h:222-227; main/touch_driver.cpp:25-29,95-118; main/main.cpp:201-208 @b1a1cc5; T-Display-S3 examples/lv_demos/lv_demos.ino:259-280 @1cc41eb; issue #17 comment lewisxhe 2025-05-27; commit 6dc65b4

**Action:** Gogadget should not initialise I2C/touch or drive GPIO16/21. If a touch variant is ever supported: probe CST328, then CST816, and disable CST auto-sleep.

### K21: I2C master driver usage

Relevance: low. Novelty: new.

**Fact:** i2c_driver.c uses the new master driver (driver/i2c_master.h): I2C_NUM_0, SCL 17/SDA 18, I2C_CLK_SRC_DEFAULT, glitch_ignore_cnt 7, internal pull-ups left disabled (memset 0). I2C_MASTER_FREQ_HZ 400000 is defined but unused (speed is set per device). The scanner prints 'UU' on ESP_ERR_TIMEOUT. On IDF 6 this is the supported API (the legacy driver is EOL, removal in v7.0), and transmit/receive now return ESP_ERR_INVALID_RESPONSE on NACK.

**Evidence:** main/i2c_driver.c:21-77 @b1a1cc5; IDF v6.1 docs/en/migration-guides/release-6.x/6.0/peripherals.rst:198-240; issue #6 log 'Please check pull-up resistances...' (T-RGB)

**Action:** Only matters if a sensor is added on SDA18/SCL17: reuse with the new API and check external pull-ups.

### K22: IDF 6 sleep/wake APIs Gogadget needs (absent from the LilyGo repo)

Relevance: high. Novelty: new.

**Fact:** In IDF 6.1, esp_sleep_get_wakeup_cause() is deprecated in favour of the bitmap esp_sleep_get_wakeup_causes(). esp_sleep_enable_ext1_wakeup() is superseded by esp_sleep_enable_ext1_wakeup_io(mask, mode). ESP_EXT1_WAKEUP_ALL_LOW is deprecated after the ESP32 (use ESP_EXT1_WAKEUP_ANY_LOW). esp_sleep_get_ext1_wakeup_status() tells which pin fired. gpio_deep_sleep_wakeup_enable()/esp_deep_sleep_enable_gpio_wakeup() were removed; they were for chips without RTC IO, and GPIO0/14 are RTC IOs on the S3 (22 RTC IO pins). LilyGo-Display-IDF has no sleep code.

**Evidence:** IDF v6.1 components/esp_hw_support/include/esp_sleep.h:31-39,333-355,406,466,619,777-785; docs/en/migration-guides/release-6.x/6.0/system.rst:83-129; components/soc/esp32s3/include/soc/soc_caps.h:282-288

**Action:** esp_sleep_enable_ext1_wakeup_io(BIT64(GPIO_NUM_0) | BIT64(GPIO_NUM_14), ESP_EXT1_WAKEUP_ANY_LOW); on wake use esp_sleep_get_wakeup_causes() and esp_sleep_get_ext1_wakeup_status() to tell which button.

### K23: Other IDF 6.0/6.1 platform changes relevant to the port

Relevance: medium. Novelty: new.

**Fact:** PicolibC replaces Newlib as the default libc. The legacy ADC driver (driver/adc.h) is removed; use esp_adc/adc_oneshot.h + adc_cali. The umbrella 'driver' component is deprecated, so depend on esp_driver_* explicitly. FreeRTOS functions now live in flash unless CONFIG_FREERTOS_IN_IRAM. Default compiler warnings are errors. esp_event.h no longer pulls in queue.h/semphr.h. The legacy I2C driver is EOL. Python >= 3.10 and CMake >= 3.22.1 are required. IDF 6.1 'idf.py flash' does fast incremental reflash ('-a' forces a full flash).

**Evidence:** IDF v6.1 docs/en/migration-guides/release-6.x/6.0/system.rst:6-25,322,363; peripherals.rst:9-31,55-63,198-240; build-system.rst:86; 6.0/tools.rst:34-37,51; 6.1/tools.rst:6-9

**Action:** Read the battery on GPIO4 via adc_oneshot + calibration. Add a test for TZ handling ('ICT-7', strftime) under PicolibC. List REQUIRES/PRIV_REQUIRES explicitly. Keep builds warning-clean.

### K24: Fonts: built-in Montserrat cannot render Vietnamese

Relevance: high. Novelty: new.

**Fact:** The 19 Montserrat sizes LilyGo enables are generated with ranges 0x20-0x7F, 0xB0 and 0x2022 plus FontAwesome symbols only, so they have no Vietnamese letters (Ă/ă, Đ/đ, Ĩ/ĩ, Ũ/ũ, Ơ/ơ, Ư/ư, U+1EA0-1EF9). LVGL 9.6 from the registry pulls espressif/freetype only if CONFIG_LV_USE_FREETYPE.

**Evidence:** sdkconfig.defaults.t-display-s3:19-38 @b1a1cc5; LVGL v9.6.0 src/font/lv_font_montserrat_14.c:1-5 ('Opts: ... -r 0x20-0x7F,0xB0,0x2022 ...'); components.espressif.com lvgl/lvgl 9.6.0~1 dependency rules

**Action:** Generate only the sizes you need (lv_font_conv / LVGL converter) with U+0020-007E plus U+00C0-00C3, 00C8-00CA, 00CC-00CD, 00D2-00D5, 00D9-00DA, 00DD, 00E0-00E3, 00E8-00EA, 00EC-00ED, 00F2-00F5, 00F9-00FA, 00FD, 0102-0103, 0110-0111, 0128-0129, 0168-0169, 01A0-01A1, 01AF-01B0, 1EA0-1EF9 (+ U+00B0 for degrees). Drop the Montserrat set; prefer bitmap fonts over runtime FreeType for battery.

### K25: LVGL 9.6 built-in screenshot-test utilities

Relevance: medium. Novelty: new.

**Fact:** LVGL 9.6 has a 'Test Utilities' menu: LV_USE_TEST (emulated input devices, time emulation, screenshot compares); LV_USE_TEST_SCREENSHOT_COMPARE (selects LodePNG, 'Requires a few MB of extra RAM', provides lv_test_screenshot_compare()); LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE (default y: missing reference images are created instead of failing).

**Evidence:** LVGL v9.6.0 Kconfig:3419-3440; include/lvgl/debugging/test/lv_test_screenshot_compare.h

**Action:** Enable only in the desktop-simulator build. In CI turn CREATE_REFERENCE_IMAGE off so a missing baseline fails. Keep the RGB565 byte swap out of LVGL (K10) so device and simulator renders are identical.

### K26: The per-board sdkconfig files are boilerplate

Relevance: low. Novelty: new.

**Fact:** sdkconfig.defaults.t-display-s3 differs from sdkconfig.defaults.t-display-s3pro only in the board symbol. sdkconfig.defaults.t-display, for an ESP32 board, even sets CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y, which the ESP32 doesn't have. The files are copies of one template, not tuned configurations.

**Evidence:** diff sdkconfig.defaults.t-display-s3 sdkconfig.defaults.t-display-s3pro (only line 1 differs); sdkconfig.defaults.t-display:7; README.md:16 (T-Display = ESP32) @b1a1cc5

**Action:** Don't read LilyGo's IDF options (240 MHz, PSRAM on, 19 fonts) as validated tuning for this board.

### K27: Espressif's IDF 6.1 i80_controller example as a modern template

Relevance: medium. Novelty: new.

**Fact:** ESP-IDF v6.1 examples/peripherals/lcd/i80_controller (CC0-1.0, ST7789 by default) is the current hand-written LVGL 9 + i80 template. Manifest pins lvgl 9.2.2. sdkconfig.defaults sets CONFIG_LV_CONF_SKIP=y, CONFIG_LV_USE_CLIB_MALLOC=y, CONFIG_FREERTOS_HZ=1000 (+ sysmon/perf monitor). Buffers come from esp_lcd_i80_alloc_draw_buffer(), which applies the bus DMA alignment: 100-line double buffers, dma_burst_size 64 ('Higher burst size can improve the performance when the DMA buffer comes from PSRAM'). It uses lv_display_set_buffers(..., PARTIAL), lv_draw_sw_rgb565_swap() in flush, esp_lcd_panel_io_register_event_callbacks() for flush-ready, an LVGL task at prio 2 with 8 KB stack, and a 2 ms esp_timer tick. main CMakeLists uses PRIV_REQUIRES esp_lcd esp_psram.

**Evidence:** IDF v6.1 examples/peripherals/lcd/i80_controller/main/i80_controller_example_main.c:1-5,72-79,93-125,193-212,316-362; main/idf_component.yml; sdkconfig.defaults; main/CMakeLists.txt; components/esp_lcd/i80/esp_lcd_panel_io_i80.c:710-723

**Action:** Use it (CC0, copy freely) for anything esp_lvgl_port does not cover. Note its LVGL pin (9.2.2) is older than Gogadget's 9.6.

### K28: RTC slow-clock choices without a 32 kHz crystal

Relevance: medium. Novelty: refines.

**Fact:** With no 32 kHz crystal possible (known), deep-sleep timekeeping on the S3 uses either RTC_CLK_SRC_INT_RC (136 kHz, the default, calibrated at boot with RTC_CLK_CAL_CYCLES=1024) or RTC_CLK_SRC_INT_8MD256 (17.5 MHz/256). IDF 6.0 removed the RC32K option ('unstable at extreme temperatures'). LilyGo-Display-IDF doesn't touch this.

**Evidence:** IDF v6.1 components/esp_hw_support/port/esp32s3/Kconfig.rtc:1-22; docs/en/migration-guides/release-6.x/6.0/system.rst:180

**Action:** Measure drift over a day of deep sleep with INT_RC vs INT_8MD256, check 8MD256's sleep-current cost, then set the NTP resync interval; consider a higher RTC_CLK_CAL_CYCLES.

### K29: Bundled components

Relevance: low. Novelty: new.

**Fact:** components/ holds: XPowersLib (submodule 5a751d8, MIT; PMU code, unused on T-Display-S3, which takes the BOARD_POWERON branch); SensorLib (submodule b50ac6f, MIT; CST816 touch only); esp_lcd_st7735 (Apache-2.0 copy of an Espressif driver, used by the ESP32 T-Display/T-Dongle path). None is needed for a non-touch T-Display S3. The submodules are not checked out in the local clone.

**Evidence:** git ls-tree HEAD components/ and .gitmodules:1-6 @b1a1cc5; components/esp_lcd_st7735/esp_lcd_st7735.c:1-4; main/display_esp32.c:21,98; main/power_driver.cpp:20,40,153,172

**Action:** Import nothing from components/.

## Reusable code

### 1. ST7789 'LCD_MODULE_CMD_1' panel-tuning table (porch, gate, VCOM, power, frame rate, gamma) and sender loop, rewritten as const C:

```c
typedef struct { uint8_t cmd; uint8_t data[14]; uint8_t len; } lcd_init_cmd_t; /* len bit7 => wait 120 ms */
static const lcd_init_cmd_t k_st7789_cmd1[] = {
    {0x11, {0}, 0x80},
    {0x3A, {0x05}, 1},
    {0xB2, {0x0B, 0x0B, 0x00, 0x33, 0x33}, 5},
    {0xB7, {0x75}, 1},
    {0xBB, {0x28}, 1},
    {0xC0, {0x2C}, 1},
    {0xC2, {0x01}, 1},
    {0xC3, {0x1F}, 1},
    {0xC6, {0x13}, 1},
    {0xD0, {0xA7}, 1},
    {0xD0, {0xA4, 0xA1}, 2},
    {0xD6, {0xA1}, 1},
    {0xE0, {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32}, 14},
    {0xE1, {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37}, 14},
};

esp_err_t st7789_send_cmd1(esp_lcd_panel_io_handle_t io)
{
    for (size_t i = 0; i < sizeof(k_st7789_cmd1) / sizeof(k_st7789_cmd1[0]); i++) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, k_st7789_cmd1[i].cmd, k_st7789_cmd1[i].data,
                                                      k_st7789_cmd1[i].len & 0x7F), TAG, "cmd1");
        if (k_st7789_cmd1[i].len & 0x80) {
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }
    return ESP_OK;
}
```

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:32-47 (table) and :139-143 (loop) @b1a1cc54994bf1b417e3bb30c437bbe1036bff7f; byte-identical to Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:133-149 @1cc41eb

**Licence:** MIT. Repo LICENSE says 'Copyright (c) 2024 TTGO'; the file header says '@license MIT, Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd' (author Lewis He). Keep the notice in Gogadget's third-party notices.

**Port notes:** Send after esp_lcd_panel_reset() + esp_lcd_panel_init() + esp_lcd_panel_invert_color(true). The table never writes MADCTL or RAMCTRL, so orientation (esp_lvgl_port .rotation) and endianness (driver data_endian) stay intact. tx_param is synchronous and limited to 32 bytes; the largest entry here is 14. The leading 0x11 duplicates the driver's SLPOUT (the driver already waited 100 ms); removing it saves 120 ms per wake but must be verified visually. DISPON is not in the table: call esp_lcd_panel_disp_on_off(panel, true) after the first frame.

### 2. Display bring-up rewritten for ESP-IDF 6.1 (replaces LilyGo display_init):

```c
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"   /* esp_lcd_new_panel_st7789 */

#define LCD_H_RES      320
#define LCD_V_RES      170
#define LCD_DRAW_LINES 40
static const char *TAG = "lcd";

esp_err_t board_lcd_init(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel)
{
    const gpio_config_t rd_cfg = { .pin_bit_mask = 1ULL << GPIO_NUM_9, .mode = GPIO_MODE_OUTPUT };
    ESP_RETURN_ON_ERROR(gpio_config(&rd_cfg), TAG, "RD");
    gpio_set_level(GPIO_NUM_9, 1);                       /* RD idle high: LCD_CAM does not drive it */

    esp_lcd_i80_bus_handle_t bus = NULL;
    const esp_lcd_i80_bus_config_t bus_cfg = {
        .dc_gpio_num = GPIO_NUM_7,
        .wr_gpio_num = GPIO_NUM_8,
        .clk_src = LCD_CLK_SRC_DEFAULT,                  /* PLL_F160M: 80 MHz / 8 = 10 MHz exactly */
        .data_gpio_nums = { GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42,
                            GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48 },
        .bus_width = 8,
        .max_transfer_bytes = LCD_H_RES * LCD_DRAW_LINES * sizeof(uint16_t),
        .dma_burst_size = 32,                            /* was .psram_trans_align/.sram_trans_align */
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_cfg, &bus), TAG, "bus");

    const esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = GPIO_NUM_6,
        .pclk_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = { .dc_idle_level = 0, .dc_cmd_level = 0, .dc_dummy_level = 0, .dc_data_level = 1 },
        .flags = { .swap_color_bytes = 1 },              /* replaces CONFIG_LV_COLOR_16_SWAP; colour data only */
    };                                                   /* no .on_color_trans_done: esp_lvgl_port installs its own */
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(bus, &io_cfg, io), TAG, "io");

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = GPIO_NUM_5,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,      /* was .color_space = ESP_LCD_COLOR_SPACE_RGB */
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(*io, &panel_cfg, panel), TAG, "st7789");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(*panel), TAG, "reset");      /* 10 + 10 ms */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(*panel), TAG, "init");        /* SLPOUT+100 ms, MADCTL, COLMOD, RAMCTRL; no DISPON */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(*panel, true), TAG, "invert");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(*panel, 0, 35), TAG, "gap");  /* (240-170)/2 */
    ESP_RETURN_ON_ERROR(st7789_send_cmd1(*io), TAG, "cmd1");
    return ESP_OK;  /* swap_xy/mirror come from esp_lvgl_port .rotation; DISPON after first frame */
}
```

**Source:** Derived from Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:60-154 @b1a1cc5 and ESP-IDF v6.1 examples/peripherals/lcd/i80_controller/main/i80_controller_example_main.c:193-266. Field names checked against IDF v6.1 components/esp_lcd/include/esp_lcd_io_i80.h:25-90 and esp_lcd_panel_dev.h:20-29.

**Licence:** MIT (LilyGo-derived values and sequence; keep the LilyGo notice) combined with CC0-1.0 (Espressif example structure). The result can be shipped under Gogadget's own licence with the MIT notice.

**Port notes:** GPIO15 must already be high (LilyGo power_driver.cpp:176-183). Set max_transfer_bytes to at least the esp_lvgl_port buffer_size*2, or tx_color fails with 'color bytes too long'. allow_pd must stay 0 on S3. If colours look wrong, remove swap_color_bytes and set .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE instead, or fall back to esp_lvgl_port .flags.swap_bytes = 1. Use exactly one of the three. ESP_RETURN_ON_ERROR comes from esp_check.h.

### 3. esp_lvgl_port 2.9 registration with LilyGo's orientation, first frame drawn before DISPON, and the panel sleep sequence:

```c
#include "esp_lvgl_port.h"

const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();   /* prio 4, 7168 B stack, 5 ms tick */
ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));
const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle = io,
    .panel_handle = panel,
    .buffer_size = 320 * 40,                 /* pixels; <= max_transfer_bytes / 2 */
    .double_buffer = true,
    .hres = 320,
    .vres = 170,
    .rotation = { .swap_xy = true, .mirror_x = false, .mirror_y = true },   /* USB on the left */
    .color_format = LV_COLOR_FORMAT_RGB565,
    .flags = { .buff_dma = true, .swap_bytes = false },  /* swap done by i80 swap_color_bytes */
};
lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);

if (lvgl_port_lock(0)) {                     /* 0 = wait forever */
    clock_ui_create(disp);
    lv_refr_now(disp);                       /* render and queue colour DMA */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true)); /* tx_param drains queued DMA, then DISPON */
    lvgl_port_unlock();
}
backlight_set(level);                        /* own AW9364 pulse-count driver */

/* before esp_deep_sleep_start(): */
backlight_off();
if (lvgl_port_lock(0)) {
    esp_lcd_panel_disp_on_off(panel, false); /* DISPOFF */
    esp_lcd_panel_disp_sleep(panel, true);   /* SLPIN; driver waits 100 ms */
    lvgl_port_unlock();
}
```

**Source:** esp_lvgl_port 2.9.0 API (espressif/esp-bsp@058d2aaa1a components/esp_lvgl_port/include/esp_lvgl_port.h:52-73,86-114 and include/esp_lvgl_port_disp.h:28-77,105). Orientation from LilyGo-Display-IDF main/display_s3.c:127-130 @b1a1cc5. Sleep calls from IDF v6.1 esp_lcd_panel_st7789.c:304-333.

**Licence:** esp_lvgl_port is Apache-2.0 (used as a dependency, not copied). Orientation constants come from MIT code. The glue itself is Gogadget's own.

**Port notes:** lvgl_port_add_disp() overwrites any on_color_trans_done and re-applies swap_xy/mirror from .rotation, so don't call them yourself. Keep the panel calls inside lvgl_port_lock so they don't interleave with an LVGL flush. In USB always-on mode, call lvgl_port_stop() before and lvgl_port_resume() after a manual light sleep to stop the 5 ms esp_timer tick. clock_ui_create, backlight_set and backlight_off are placeholders.

### 4. Light-sleep pattern for an i80 ST7789 without re-initialising the panel (from Espressif's own test):

```c
esp_lcd_panel_disp_on_off(panel, false);            /* blank */
ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(2 * 1000 * 1000));
ESP_ERROR_CHECK(esp_light_sleep_start());
esp_lcd_panel_disp_on_off(panel, true);             /* i80 bus and panel keep working, no re-init */
/* esp_lcd_panel_draw_bitmap(...) works again */
```

**Source:** ESP-IDF v6.1 components/esp_lcd/test_apps/i80_lcd/main/test_i80_lcd_panel_sleep.c:101-128 (ST7789, ESP32-S3 pins in test_i80_board.h:15-28)

**Licence:** CC0-1.0 (file header SPDX)

**Port notes:** On the S3, bus flags.allow_pd must stay 0 (no LCDCAM retention). With CONFIG_PM_ENABLE the i80 driver holds a NO_LIGHT_SLEEP lock during transfers, so auto light sleep only happens between frames. USB-Serial-JTAG does not survive light sleep; use CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION while USB is attached. Only relevant to the USB always-on mode; the battery mode uses deep sleep.

### 5. I2C master bus init and address scan on the SDA18/SCL17 header (new IDF driver):

```c
i2c_master_bus_handle_t bus;
const i2c_master_bus_config_t cfg = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = GPIO_NUM_17,
    .sda_io_num = GPIO_NUM_18,
    .glitch_ignore_cnt = 7,
    /* LilyGo leaves .flags.enable_internal_pullup = 0 (external pull-ups assumed) */
};
ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    if (i2c_master_probe(bus, addr, 50) == ESP_OK) {
        ESP_LOGI(TAG, "found 0x%02x", addr);
    }
}
```

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/i2c_driver.c:33-77 @b1a1cc5

**Licence:** MIT (Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd)

**Port notes:** Low priority: only if Gogadget adds a sensor. The LilyGo scanner uses a 1000 ms probe timeout per address; shorten it. On IDF 6 the legacy driver/i2c.h is EOL, and NACK on transmit/receive returns ESP_ERR_INVALID_RESPONSE. Check whether the non-touch board has pull-ups on 17/18.

## Open questions

- Does this panel accept ST7789 RAMCTRL little-endian (esp_lcd data_endian=LITTLE) on the 8-bit i80 bus? Otherwise the LCD_CAM swizzle (swap_color_bytes) is the only zero-CPU byte-order option. A one-minute colour test on hardware settles it.
- Is the CMD_1 table's leading SLPOUT + 120 ms (on top of the driver's SLPOUT + 100 ms) actually needed? Dropping it saves 120 ms per wake; LilyGo never tested that.
- After ESP32 deep sleep, can the panel be woken from SLPIN with just SLPOUT/DISPON, skipping hardware reset and CMD_1? The panel supply stays on, but it depends on RST(GPIO5), CS(GPIO6), WR/DC/RD having board pull-ups or needing gpio_hold_en during deep sleep. Check the schematic and measure current and image.
- Does GPIO14 (button 2) have an external pull-up on v1.2, so ext1 ANY_LOW wake works without RTC pull-ups (which keep RTC_PERIPH powered)? Does GPIO15's enable line have a pull-down, so backlight/LED/divider stay off in deep sleep without gpio_hold?
- Is PSRAM needed at all for Wi-Fi + later BLE provisioning + LVGL with internal DMA buffers? Measure free internal heap, and compare wake time with PSRAM disabled vs CONFIG_SPIRAM_MEMTEST=n.
- Actual wake-to-first-frame time on IDF 6.1 with QIO flash, no PSRAM memtest and BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP is unknown. Issue #4's numbers come from IDF 5.3-dev on the AMOLED sibling board (same SoC/PSRAM).
- The root cause of LilyGo-Display-IDF issue #8 ('very slow' vs Arduino) is unconfirmed: no maintainer reply. 'Idle-priority LVGL task + -Og + PSRAM buffers' is only a hypothesis.
- Deep-sleep RTC drift with RTC_CLK_SRC_INT_RC vs RTC_CLK_SRC_INT_8MD256 on this board, and the extra sleep current of INT_8MD256, need a 24 h measurement before choosing the NTP resync interval.
- How PicolibC (IDF 6 default libc) handles setenv("TZ","ICT-7")/tzset()/strftime has not been verified; needs a unit test.
- Whether LilyGo will update LilyGo-Display-IDF for IDF 6 / LVGL 9 is unknown: BSP request #16 has been open and unanswered since 2025-04-06.
- The SensorLib (b50ac6f) and XPowersLib (5a751d8) submodules are not checked out in the clone, so SensorLib's CST816 reset/IRQ behaviour on a non-touch board was not inspected. It only matters if touch support is ever added.
