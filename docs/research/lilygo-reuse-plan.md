# LilyGO reuse plan

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive: seven parallel source sweeps of Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5, adversarial verification of the 24 top claims, then a synthesis agent that wrote this file)

Status: synthesis (a porting plan, not a fact list); it builds on the verified claims in lilygo-verification.md

Notes: the settled decisions (docs/adr/) use 1.ntp.vnix.vn and pool.ntp.org as NTP servers (P7 lists vn.pool.ntp.org) and never take servers from DHCP; the press that wakes the device is consumed, so P8's DOWN start state must not produce an action; local time is computed in core/ from the fixed UTC+7 offset, so P7's setenv/tzset step is not used.

Superseded by decision: THIRD_PARTY_NOTICES.md carries only the two LilyGO MIT notices, because the button state machine (P8) is Gogadget's own and copies no OneButton code (ADR-0008); there is no `licenses` console command, and the SSID masking (P9) goes into the decided console commands (`wifi.list`, ADR-0016), not a `wifi status` command.

**Pinned sources:**
- T-Display-S3 (TDS3) @1cc41eb1c76b2f3215130a3dd64e7138ce00b482.
- LilyGo-Display-IDF (LDI) @b1a1cc54994bf1b417e3bb30c437bbe1036bff7f.

Nothing upstream builds on IDF 6.x, so everything below is ported by hand. Treat LilyGO code as a behavioural reference, not a drop-in. Tags are as in the KB: [V] verified, [I] inference.

## 1. Licences and attribution

| Source | Licence | Notice to keep |
|---|---|---|
| TDS3 (`factory.ino`, `Sleep.ino`, `GetBatteryVoltage.ino`, `zones.h`; no per-file headers) | MIT | "Copyright (c) 2022 Xinyuan-LilyGO" |
| LDI (`display_s3.c`, `power_driver.cpp`, …) | MIT | LICENSE: "Copyright (c) 2024 TTGO". File headers: "Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd" (Lewis He) |
| TDS3 `lib/OneButton` 2.0.4 | BSD | "Copyright (c) 2005-2014 by Matthias Hertel", plus conditions and disclaimer |
| posix_tz_db (via `zones.h`) | MIT | only one constant is used |
| ESP-IDF v6.1 examples and test apps (`i80_controller`, `test_i80_lcd_panel_sleep.c`) | CC0-1.0 | none required |
| esp_lvgl_port 2.9 | Apache-2.0 | dependency only; nothing copied |

- **Notices file:** add `THIRD_PARTY_NOTICES.md` with both MIT texts (both copyright lines) and the OneButton BSD text.
- **Binaries:** ship the notices with release binaries, for example in the release archive and through a `licenses` console command. MIT and BSD notices must accompany binaries too.
- **File headers:** put this on every derived file: `/* Portions derived from Xinyuan-LilyGO/<repo> <path> @<sha> (MIT). See THIRD_PARTY_NOTICES.md. */`
- **Never copy:**
  - `examples/ULP_ADC`, `examples/ULP_Count`, `examples/WIFI_Audio_I2S`: GPL-3.0 headers inside the MIT repo.
  - `examples/PCBClock`, `examples/PokerS3`: VolosR code with no licence.
  - `examples/factory/font_Alibaba.c`: the typeface licence is unknown.
  - `lib/ESP32-audioI2S` (GPL); arduino-nofrendo and DabbleESP32 (LGPL).
  - krupis/T-Display-S3-esp-idf: no licence.
  - llgok/lilygo_device_driver: GPL-3.0, and it only covers P4 boards.
- zef/t-display-s3-espidf-lvgl (MIT) is for reading only.

## 2. What to port

**P1. Vendor init table (CMD_1).**
- Source: LDI `main/display_s3.c:32-47`, with the send loop at `:139-143`. Identical to TDS3 `examples/factory/factory.ino:133-149`.
- Copy all 14 entries byte-for-byte, keeping `D0 A7`, `D6 A1` and `3A 05`. Store them as `static const struct { uint8_t cmd; uint8_t len; uint16_t delay_ms; uint8_t data[14]; }`, with an explicit delay instead of the `0x80` flag.
- Send each entry with `esp_lcd_panel_io_tx_param()` after `esp_lcd_panel_init()` and `esp_lcd_panel_invert_color(true)`.
- Keep the leading SLPOUT and its 120 ms delay. IDF sends its own SLPOUT 10 ms after reset, but DS Table 9 note 7 requires 120 ms [V].
- Comment each entry with its decode (KB §4).
- Add Kconfig `GOGADGET_LCD_VENDOR_INIT` (default y) for A/B tests on matte panels.
- Add a host unit test that each entry sends exactly `len` bytes and the right delay. This guards against the `j < len & 0x7f` bug.

**P2. i80 bus and panel bring-up.**
- Sources: LDI `main/display_s3.c:60-154`, TDS3 `factory.ino:252-331`, and IDF v6.1 `examples/peripherals/lcd/i80_controller` (CC0).
- Order:
  1. GPIO15 HIGH (on battery).
  2. GPIO9 (RD) as an output, HIGH.
  3. Create the bus, IO and panel: `esp_lcd_new_i80_bus`, `esp_lcd_new_panel_io_i80`, `esp_lcd_new_panel_st7789`.
  4. `esp_lcd_panel_reset`, then `esp_lcd_panel_init`.
  5. `invert_color(true)`, then `set_gap(0, 35)`.
  6. Send CMD_1.
  7. Register esp_lvgl_port (P3) and render the first frame.
  8. `disp_on_off(true)`.
  9. Turn on the backlight (P4).
- Bus config: `dc_gpio_num 7`, `wr_gpio_num 8`, `data_gpio_nums {39,40,41,42,45,46,47,48}`, `bus_width 8`, `clk_src LCD_CLK_SRC_DEFAULT`, `max_transfer_bytes` at least the draw-buffer size in bytes, `dma_burst_size` 32 or 64, `flags.allow_pd 0` (the S3 has no LCD_CAM sleep retention).
- IO config: `cs_gpio_num 6`, `pclk_hz` 10 MHz, `trans_queue_depth 10`, `dc_levels {0,0,0,1}`, `lcd_cmd_bits 8`, `lcd_param_bits 8`, `flags.swap_color_bytes 1`. Leave `on_color_trans_done` unset; esp_lvgl_port installs its own.
- Panel config: `reset_gpio_num 5`, `rgb_ele_order LCD_RGB_ELEMENT_ORDER_RGB`, `bits_per_pixel 16`.
- IDF 6.1 changes from the LilyGO code:
  - `.color_space` becomes `.rgb_ele_order`.
  - `.psram_trans_align` and `.sram_trans_align` become `.dma_burst_size`.
  - GPIO fields are `gpio_num_t`.
  - Drop the `esp_dma_utils.h` include.
- Check every `esp_err_t`: warnings are errors in 6.x.

**P3. esp_lvgl_port registration.** This is Gogadget glue; only the constants come from LDI.
- Init: `const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG(); lvgl_port_init(&port_cfg);` This gives task priority 4, a 7168 B stack and a 5 ms tick [V].
- `lvgl_port_display_cfg_t`:
  - `hres 320`, `vres 170`.
  - `buffer_size 320*40`, in pixels; keep it at most `max_transfer_bytes/2`.
  - `double_buffer true`, `color_format LV_COLOR_FORMAT_RGB565`.
  - `rotation {swap_xy 1, mirror_x 0, mirror_y 1}` for USB on the left; `LV_DISPLAY_ROTATION_180` then gives USB on the right.
  - `flags.buff_dma 1`, `flags.buff_spiram 0`, `flags.swap_bytes 0`.
- `lvgl_port_add_disp()` replaces the IO flush-done callback and re-applies swap_xy/mirror from `.rotation` [V source]. Do not call those yourself.
- First frame: inside `lvgl_port_lock(0)`, build the screen, call `lv_refr_now(disp)`, then `esp_lcd_panel_disp_on_off(panel, true)`. tx_param drains queued colour DMA first [V], so DISPON shows a complete frame. Unlock, then ramp the backlight.

**P4. AW9364 driver.**
- Source: TDS3 `factory.ino:223-248` (2d3c294) for the modular step maths; timing from the AW9364 V2.4 datasheet.
- Pulses: use `gpio_set_level` (or a dedicated GPIO) with `esp_rom_delay_us(1–2)` for each LOW and each HIGH phase. Emit at most 15 pulses inside `portENTER_CRITICAL` (under 50 µs in total), so no LOW phase can stretch past 500 µs.
- Enable: EN HIGH, wait at least 30 µs; the chip starts at level 16.
- Off: EN LOW for at least 3 ms.
- Invalidate the cached level after any EN LOW longer than 0.5 ms, or after V3V is lost (GPIO15 LOW on battery).
- API: steps 0–16, about 5 mA each. Use a dim default on battery.
- Sleep: no hold is needed because of the internal 150 kΩ pull-down; just drive EN LOW.

**P5. Sleep entry and wake.**
- Source, for the order only: TDS3 `factory.ino:395-404` (c9fd58a) and `examples/Sleep/Sleep.ino:77-117` (fc466c6).
- Gogadget sleep order:
  1. Backlight off (EN LOW for at least 3 ms).
  2. Optional DISPOFF.
  3. SLPIN via `esp_lcd_panel_disp_sleep(panel, true)`, which waits 100 ms. `tx_param(0x10)` followed by 5 ms is equivalent. Never send SLPIN less than 120 ms after an SLPOUT.
  4. GPIO15 LOW, pulls off, `gpio_hold_en(15)`.
  5. CS 6, RD 9, WR 8 and RST 5 HIGH, DC 7 LOW, each with `gpio_hold_en` (they are RTC pads).
  6. D0–D7 LOW with `gpio_hold_en`, plus `gpio_deep_sleep_hold_en()`.
  7. GPIO38 LOW. Leave GPIO16–18 untouched.
  8. `esp_wifi_stop()`.
  9. Wait until GPIO0 and GPIO14 have both read HIGH for at least 50 ms.
  10. `esp_sleep_enable_ext1_wakeup_io(BIT64(0) | BIT64(14), ESP_EXT1_WAKEUP_ANY_LOW)`, plus a timer if one is used.
  11. `esp_deep_sleep_disable_rom_logging()`, then `esp_deep_sleep_start()`.
- Wake: drive the same pins to the same levels first, then call `gpio_hold_dis` and `gpio_deep_sleep_hold_dis`. Read `esp_sleep_get_wakeup_causes()` and `esp_sleep_get_ext1_wakeup_status()`.
- On a matte unit, compare this parking against the #300 all-LOW parking. With RST LOW the panel is reset, so that variant needs a full init on wake.

**P6. Battery/VSYS and USB sensing.**
- Source: TDS3 `GetBatteryVoltage.ino:76-93`, for the idea only. It uses the legacy API, takes a single sample and prints a misleading message.
- ADC setup: `adc_oneshot_new_unit(ADC_UNIT_1)`, `ADC_CHANNEL_3`, `ADC_ATTEN_DB_12`, `adc_cali_create_scheme_curve_fitting`.
- Sampling: wait 3–5 ms after GPIO15 goes HIGH, and sample before the backlight or Wi-Fi load starts. Average 16–32 samples and multiply by 2. Optionally store a per-unit trim in NVS.
- USB test: GPIO15 LOW, wait 5 ms; a pin voltage above about 1 V means USB.
- VSYS above about 4.35 V (with hysteresis) means USB is present. It never means "no battery".

**P7. Time and SNTP.**
- Source: TDS3 `factory.ino:453-471` and `716-722` (c9b939f), plus `zones.h:251`.
- Setup:
```c
esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2, ESP_SNTP_SERVER_LIST("vn.pool.ntp.org", "pool.ntp.org"));
cfg.server_from_dhcp = false;
cfg.sync_cb = on_sync;
esp_netif_sntp_init(&cfg);
```
- `on_sync` sets an RTC-memory "time valid" flag and stores the last-sync epoch.
- Call `esp_netif_sntp_sync_wait()` with a bounded timeout, then `esp_netif_sntp_deinit()` and `esp_wifi_stop()`.
- Call `setenv("TZ", "<+07>-7", 1); tzset();` on every boot, deep-sleep wakes included. Unit-test this under PicolibC.
- Never block the glance waiting for sync.

**P8. Buttons.**
- Source: TDS3 `lib/OneButton/src/OneButton.cpp:217-330` and `OneButton.h:153-155` (2.0.4, BSD).
- Structure: one C struct per button with `tick(now_ms, level)`, called every 5–10 ms from an `esp_timer`. A GPIO ISR starts the timer; stop it when idle.
- Defaults: 50 / 400 / 800 ms. GPIO0 needs 20–30 ms of software debounce. GPIO14 needs at least 20 ms to settle.
- Start the button that caused the ext1 wake in the DOWN state.
- Post events to a FreeRTOS queue; never touch LVGL from callbacks.

**P9. Helpers.**
- SSID masking (`factory.ino:431-451`) for the console's `wifi status` output.
- Optional: `i2c_new_master_bus` on SDA 18 / SCL 17 (LDI `main/i2c_driver.c:33-77`, MIT), only if a sensor is ever added. The board pull-ups are already fitted.

**P10. Clock face (concept only).**
- Source: `factory_gui.cpp:23-80,137-149`. It has two 140×140 boxes, a 120 px 4 bpp digits-only font of about 25 KB, a blinking colon and a tileview.
- LVGL 9 replacements:
  - `lv_subject_t` and observers instead of `lv_msg`;
  - `lv_screen_active`;
  - `lv_obj_remove_flag`.
- Redraw only when the minute changes.
- Generate fonts with `lv_font_conv` using these ranges: U+0020–007E, 00B0, 00C0–00C3, 00C8–00CA, 00CC–00CD, 00D2–00D5, 00D9–00DA, 00DD, 00E0–00E3, 00E8–00EA, 00EC–00ED, 00F2–00F5, 00F9–00FA, 00FD, 0102–0103, 0110–0111, 0128–0129, 0168–0169, 01A0–01A1, 01AF–01B0, 1EA0–1EF9.

**P11. Espressif CC0 patterns.**
- `esp_lcd_i80_alloc_draw_buffer()` from the i80 example.
- `components/esp_lcd/test_apps/i80_lcd/main/test_i80_lcd_panel_sleep.c`: DISPOFF, then light sleep, then DISPON, with no re-init. Relevant only if USB mode ever uses light sleep.

**P12. Reference numbers only.** Use TFT_eSPI `ST7789_Rotation.h` (MADCTL per rotation, 35 px offsets) and `Setup206_LilyGo_T_Display_S3.h` as cross-checks.

## 3. What not to copy, and why

- **LDI LVGL glue and config:**
  - It uses the LVGL 8 API, which has no 9.6 mapping.
  - The LVGL task runs at idle priority, built with `-Og` and a 100 Hz tick; this is the likely cause of #8 "very slow".
  - It puts PSRAM draw buffers on a 10 MHz bus; Espressif pairs PSRAM buffers with 2 MHz.
  - `LV_COLOR_16_SWAP` and `LV_USE_USER_DATA` are gone in 9.6, and `LV_MEM_SIZE_KILOBYTES` is ignored.
  - It runs touch init on non-touch boards.
  - It turns the backlight fully on before the first frame.
  - Return codes are unchecked.
  - `setup.py` leaves the LVGL version floating.
  - The per-board sdkconfig is a template, not tuned settings.
- **TDS3 factory runtime:**
  - The 16 MHz pixel clock is out of spec, and the code uses fields removed in IDF 6.1.
  - Boot is blocking: a GIF, `tone()`, then 30 s + 120 s of Wi-Fi waits.
  - The ipapi.co lookup relies on a pinned certificate that expired on 2025-03-29.
  - It uses DHCP-supplied NTP servers and calls the blocking `getLocalTime()` every 100 ms.
  - It mounts storage with `format_if_mount_failed`.
- **Any LEDC/PWM backlight code** (the LovyanGFX MIDI examples' `Light_PWM` at 22 kHz, PCBClock, examples from before 2026): the AW9364 counts PWM low phases as dimming steps.
- **The CMD_1 loops in nes, usb_hid_pad, touch_test and GetBatteryVoltage:** the `j < len & 0x7f` precedence bug sends 128 bytes after SLPOUT.
- **ULP examples:** they are GPL, read GPIO1 instead of GPIO4, keep the CPU awake and assume an 8 MHz ULP clock. Write any ULP code fresh for IDF 6.1.
- **`esp_lcd_panel_set_brightness()`:** it writes WRDISBV, which this board ignores.
- **Other legacy material:**
  - `lib/lv_conf.h` and `lib/lvgl` (8.3.0-dev);
  - TFT_eSPI, Arduino_GFX and the LovyanGFX configs;
  - `examples/ota`, which loops on restart; later, use `esp_https_ota` with rollback;
  - `tools/Image2Lcd`, a 2005 Windows binary; use LVGL's image converter instead.

## 4. IDF 6.1 / LVGL 9.6 porting checklist

- **esp_lcd:**
  - Use `rgb_ele_order`/`data_endian`, `dma_burst_size` and `allow_pd = 0`.
  - Allocate buffers with `esp_lcd_i80_alloc_draw_buffer()`.
  - `tx_param` takes at most 32 parameter bytes and drains queued colour DMA.
  - DISPON is yours to send.
  - `disp_sleep` waits 100 ms.
- **LVGL 9.6:**
  - Buffer sizes are in bytes, and the flush callback takes `uint8_t *px_map`.
  - Set `CONFIG_LV_MEM_SIZE` in bytes, or use `CONFIG_LV_USE_CLIB_MALLOC`.
  - Include only `lvgl.h`.
  - Do not define `LV_DISABLE_API_MAPPING`: esp_lvgl_port 2.9 still calls `lv_draw_sw_*`.
  - Replace `lv_msg` with `lv_subject`.
- **esp_lvgl_port 2.9:** `lvgl_port_lock(0)` waits forever. Use `lvgl_port_stop()` and `lvgl_port_resume()` around any manual light sleep.
- **Sleep:** use `esp_sleep_enable_ext1_wakeup_io`, `esp_sleep_get_wakeup_causes` (a bitmap; the single-cause getter is deprecated) and `esp_sleep_get_ext1_wakeup_status`.
- **Other components:**
  - ADC: `esp_adc/adc_oneshot.h` and `adc_cali_scheme.h`; the legacy driver is removed.
  - Provisioning: `espressif/network_provisioning`.
  - I2S, only if an alarm is added: `driver/i2s_std.h`.
  - libc: PicolibC.
  - List `REQUIRES`/`PRIV_REQUIRES` explicitly.

## 5. Hardware acceptance tests for the port

- R/G/B bars and a 1-px frame in both orientations, to check colour order, the 35-row gap and inversion.
- Cold boot, deep-sleep wake and `esp_restart`: a stale frame or noise must never be visible.
- AW9364:
  - 16 levels that rise monotonically;
  - no flash at boot with USB connected;
  - off after 3 ms of EN LOW.
- Sleep current with SLPIN and the pin parking, compared with the #300 variant. Run an overnight soak on a matte unit.
- USB plug and unplug with Wi-Fi and the backlight on: log the reset reason and whether the time survives.
