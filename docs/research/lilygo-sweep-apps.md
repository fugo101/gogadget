# LilyGO sweep: other examples, libraries and tooling

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 2 of 48 items were verified in lilygo-verification.md (2 confirmed / 0 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): the pixel clock stays at 10 MHz with a hard ceiling of about 15 MHz (K08 suggests trying 16 MHz); PSRAM stays enabled (octal, memtest off); the press that wakes the device is consumed and does not act (the OneButton item says the waking press should count); local time is computed in core/ from the fixed UTC+7 offset, not through a TZ string.

Superseded by decision: DISPON is sent after the first frame, not before CMD_1 (K06 and reusable item 6; ADR-0006); the NTP servers are 1.ntp.vnix.vn and pool.ntp.org, not time.google.com (K12); the coredump partition is 256 KB, not 64 KB (K19; ADR-0013); the draw buffers are two 320x40 buffers (K09 suggests 320x34); the USB test is GPIO15 LOW then GPIO4, with no extra timer wake (K04); the button state machine is Gogadget's own, not a OneButton port (K24; ADR-0008); console commands follow ADR-0016 (`wifi.list`, not `wifi status`, reusable item 5).

Scope: the non-display examples, the vendored lib/ folder, PlatformIO and CI config, and the factory images. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**Scope.** I read all 32 example folders, going deep on the 20 that are not about display or power. I also covered lib/ (14 libraries, 3 diffed against upstream tags), platformio.ini, boards/, the CI workflows, tools/, firmware/ (parsed the headers, partition tables and app descriptors of all 7 binaries), README/README_CN and LICENSE. Evidence also comes from all 350 issues/PRs and their 1,693 comments. Repo HEAD is 1cc41eb (2026-10-13).

**Most useful for Gogadget**
1. **Board revision:** LilyGO now sells a v1.3 with a re-matched onboard antenna. v1.2 boards have weaker Wi-Fi (#340), so NTP needs retries and timeouts.
2. **IDF 6.1 display bring-up (from the factory code):** drive LCD_RD (GPIO9) high and call `esp_lcd_panel_disp_on_off(true)`. IDF 6.1 renamed two config fields the factory uses (`color_space`→`rgb_ele_order`, `*_trans_align`→`dma_burst_size`). RGB565 byte swap can be done in hardware (LCD_CAM `swap_color_bytes` or ST7789 little-endian mode) instead of in LVGL.
3. **Time:** the main crystal is 10 ppm (#262), but deep sleep runs on the RC clock, so resync NTP periodically. Use TZ `"<+07>-7"` (from zones.h). Stop Wi-Fi once SNTP syncs. Don't take NTP servers from DHCP (#297). Never block waiting for valid time.
4. **Energy:** about 148 mA awake (#289) and 150 mA or more with Wi-Fi (#295), so one 10 s glance costs about 0.42 mAh. 20 glances, 1 sync and 263 µA sleep come to about 15 mAh/day, roughly 2 weeks on 250 mAh.
5. **USB detection works only through the ADC** (#292), so plugging in USB cannot wake the board. Check for USB at every wake.
6. **Buttons:** the OneButton state machine (50/400/800 ms, BSD-3) is the best base for the two-button driver. No official example wakes on GPIO0 or on both buttons; they all use ext0 on GPIO14.
7. **Flash/PSRAM:** Winbond 16 MB flash at QIO 80 MHz plus octal PSRAM. Choosing the wrong PSRAM mode causes a boot loop.
8. **firmware/:** the touch and no-touch factory images are byte-identical 8 MB Arduino builds (ESP-IDF 4.4.4) with NVS at 0x9000. Run `idf.py erase-flash` once before the first Gogadget flash. The "ScreenDetection" image only prints compile-time pin settings; it does not probe the panel.
9. **Don't copy:** the community MIDI examples (not built by CI, wrong mapping.h pins, PWM backlight, one broken include), PCBClock (blocking Wi-Fi, 2.4 MB of frames, no licence), and the files with GPL headers.

**Examples, one line each**
- **factory:** esp_lcd + LVGL 8 clock with NTP, POSIX TZ, a Wi-Fi fallback chain, SmartConfig, OneButton and SLPIN + deep sleep. Reuse its init and NTP patterns; avoid its multi-second boot.
- **PCBClock:** 7-segment clock with fixed UTC offsets, Wi-Fi always on, full redraw every loop. An anti-pattern.
- **ota:** ArduinoOTA push over espota and a reboot loop when Wi-Fi fails. Use esp_https_ota with rollback instead.
- **SerialExample:** UART pin remapping. The JST-SH connector (GPIO43/44) is a ready-made debug UART.
- **I2CScan / MPR121TouchSensor:** I2C on 18/17 (nothing but pull-ups on the non-touch board) / MIDI-shield touch chips.
- **sd:** SD shield on 11/12/13, which formats the card if mounting fails. Never auto-format.
- **usb_hid_pad:** TinyUSB HID mouse. It needs USB-OTG mode, which removes the USB-Serial-JTAG console.
- **touch_test / CapacitiveTouch:** touch variant only; shows holding a pin across deep sleep with gpio_hold.
- **nes:** direct-to-panel palette drawing and line dropping. Not needed.
- **PokerS3:** two-button click/double-click mapping. Seed the RNG with esp_random instead.
- **WIFI_Audio_I2S / T-Display-S3-MIDI:** audio and MIDI-shield bring-up. Not applicable.
- **Queue / Piano / Piano-Debug / Gingoduino:** MIDI visualisers that redraw only on change, cap at 30 fps, and keep an on-screen log.
- **BLE Sender/Receiver:** the 31-byte advertising limit and BLE scan/connect in separate tasks. Hints for BLE provisioning later.
- **USB-Device:** USB host and device are mutually exclusive; this example has a broken include.
- **Music-Explorer:** a 7-page two-button UI (one button for page, one for item). Its audio uses the I2S driver that IDF 6 removed.

**lib/:**
- TFT_eSPI 2.5.43: LilyGO changed only two things.
- LVGL 8.3.0-dev: a 2022 snapshot, never updated.
- OneButton and Arduino_GFX: identical to upstream.
- ESP32-audioI2S: GPL and patched by LilyGO.
- The rest: well behind current upstream releases.

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| K01 | Board revision / Wi-Fi antenna | high | new |  |
| K02 | Energy budget numbers | high | refines | confirmed |
| K03 | Timekeeping accuracy | high | new |  |
| K04 | USB presence detection | high | refines | confirmed |
| K05 | LCD RD pin must be high | high | new |  |
| K06 | esp_lcd init order and IDF 6.1 API changes | high | new |  |
| K07 | Landscape orientation | medium | new |  |
| K08 | Pixel clock history | medium | refines |  |
| K09 | Factory draw buffer / DMA sizing | medium | new |  |
| K10 | RGB565 byte order | medium | new |  |
| K11 | Time zone handling | high | new |  |
| K12 | SNTP lifecycle | high | new |  |
| K13 | DHCP-provided NTP pitfall | medium | new |  |
| K14 | Never block on time validity | medium | new |  |
| K15 | Wi-Fi credential flow in the factory | medium | new |  |
| K16 | Stale NVS from factory firmware | medium | new |  |
| K17 | Prebuilt firmware facts | medium | new |  |
| K18 | Flash / PSRAM configuration | high | new |  |
| K19 | Partition layout references | medium | new |  |
| K20 | OTA example | medium | new |  |
| K21 | PCBClock (what not to do) | medium | new |  |
| K22 | Factory clock UI patterns (LVGL 8 to 9) | medium | new |  |
| K23 | Factory boot sequence is a glance anti-pattern | medium | new |  |
| K24 | Buttons in official code (OneButton) | high | new |  |
| K25 | Other button schemes in the repo | low | new |  |
| K26 | Peripherals stealing GPIO0 | low | new |  |
| K27 | USB modes: Serial-JTAG vs TinyUSB | medium | new |  |
| K28 | Console, flashing and debugging workflow | medium | refines |  |
| K29 | JST-SH connector and spare GPIOs | medium | new |  |
| K30 | UART pin labelling inconsistency | low | contradicts |  |
| K31 | BLE facts for later provisioning | medium | new |  |
| K32 | Cross-task event plumbing pattern | medium | new |  |
| K33 | Community MIDI examples: quality and contradictions | medium | contradicts |  |
| K34 | CMD_1 sender bug in TFT_eSPI-style examples | low | new |  |
| K35 | Code that won't port to IDF 6.1 | low | new |  |
| K36 | nes example throughput tricks | low | new |  |
| K37 | lib/TFT_eSPI patch status | low | new |  |
| K38 | lib/lvgl and lv_conf | low | new |  |
| K39 | lib/ inventory (remaining libraries) | low | new |  |
| K40 | Licensing of repo content | medium | new |  |
| K41 | Version-support statements | low | refines |  |
| K42 | README power wording vs maintainer statements | medium | refines |  |
| K43 | Electrical limits and enclosure | low | new |  |
| K44 | tools/ image converter | low | new |  |
| K45 | Repo maintenance signals | low | new |  |
| K46 | Minor examples, one line each | low | new |  |
| K47 | Wi-Fi association quirk | low | new |  |
| K48 | AW9364 timing in the factory driver | medium | refines |  |

### K01: Board revision / Wi-Fi antenna

Relevance: high. Novelty: new.

**Fact:** LilyGO states there is a newer v1.3 T-Display-S3 whose onboard antenna was 'specially matched'; boards silkscreened 'T-Display S3 22-8-2 v1.2 XX2' (older) have inferior onboard-antenna performance (a customer with ~100 boards reported consistently weak RSSI). LilyGO advises ordering 'the latest v1.3 version' from lilygo.cc. External antenna = rotate a 0201 resistor to the u.FL position (README section added 2026-08-10). Wi-Fi is 2.4 GHz only (#250).

**Evidence:** issue #340 (lewisxhe 2026-04-14: 'The latest version ... the antenna has been specially matched. The image you sent is of an older version, and its antenna performance is somewhat inferior'; 2026-05-26: 'specify that you need the latest v1.3 version'; 2026-05-19 resistor photo); README.md:258-262@1cc41eb (commit 3c048f7); issue #123; #250

**Action:** Assume weak RSSI on the v1.2 Gogadget board: NTP with 2-3 attempts and ~8-10 s connect timeout per attempt, log RSSI at each sync, test provisioning at realistic distance; keep the u.FL resistor mod as a fallback; buy v1.3 for future units.

### K02: Energy budget numbers

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `apps:K02`).

**Fact:** Reported currents: 147.7 mA awake with screen on (user, ESP-IDF code, same thread as the known 6.42 mA/263 uA sleep numbers); maintainer: average current with Wi-Fi on is about 150 mA+ and the heat centre should be the ESP32-S3.

**Evidence:** issue #289 body ('draws 6.42mA asleep and 147.7mA awake'); issue #295 (lewisxhe 2024-12-19: 'When WiFi is turned on normally, the average current is about 150mA+')

**Action:** Model ~150 mA for both the 10 s glance (~0.42 mAh) and NTP sessions (~0.25-0.4 mAh); with 20 glances/day + 1 sync + 263 uA sleep (~6.3 mAh/day) expect ~15 mAh/day, i.e. roughly two weeks on 250 mAh. Validate with a power profiler.

### K03: Timekeeping accuracy

Relevance: high. Novelty: new.

**Fact:** The 40 MHz main crystal is 10 ppm (maintainer), i.e. <=~0.9 s/day while awake/on USB; a user doing NTP once and then running offline saw 1-2 min/month drift. In deep sleep the RTC runs from the internal RC slow clock (no 32 kHz crystal possible, known), which is far less accurate than 10 ppm; nothing in the repo quantifies it.

**Evidence:** issue #262 (lewisxhe 2024-07-08: 'The default is 10ppm crystal.'); issue #253 body

**Action:** Schedule NTP resyncs (e.g. every 6-12 h via timer wake, or opportunistically while on USB); measure the RC drift over 24 h of deep sleep on the real board and set the resync interval from it; keep SNTP periodic sync running in always-on USB mode.

### K04: USB presence detection

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `apps:K04`).

**Fact:** There is no digital VBUS-detect line. Maintainer: detecting USB cable connect/disconnect without the ADC 'is not possible with current hardware'; USB is inferred only from the battery ADC reading above 4.2 V (charging state cannot be read; only the red LED shows it).

**Evidence:** issue #292 (lewisxhe 2024-12-04); issue #254 (lewisxhe 2024-06-03: 'If it is greater than 4.2V, it can be determined that the USB is connected, but it cannot be determined whether it is charging'); issue #260

**Action:** Plugging USB cannot wake Gogadget from deep sleep. At every wake (button/timer) enable GPIO15, read GPIO4; if >~4.3 V enter always-on mode. Optionally add a periodic timer wake so 'plugged in -> always-on' happens without a button press; usb_serial_jtag_is_connected() can additionally tell a PC host from a charger.

### K05: LCD RD pin must be high

Relevance: high. Novelty: new.

**Fact:** The i80 driver does not manage LCD_RD (GPIO9); LilyGO's code drives it HIGH before creating the bus (Arduino factory and LilyGo-Display-IDF). An ESP-IDF v5 user had a black/garbage screen until RD was pulled up; two others confirmed the fix.

**Evidence:** examples/factory/factory.ino:257-258@1cc41eb; LilyGo-Display-IDF main/display_s3.c:66-69@b1a1cc5; issue #103 (hoffmajs 2023-05-02, confirmed by simonchatts and KamranAghlami)

**Action:** In the Gogadget display driver configure GPIO9 as output HIGH (or input with pull-up) before esp_lcd_new_i80_bus(); keep it high in light sleep; include it in the board pin header.

### K06: esp_lcd init order and IDF 6.1 API changes

Relevance: high. Novelty: new.

**Fact:** LilyGO's 2026-07-28 fix for Arduino core 3.3.11 added esp_lcd_panel_disp_on_off(panel, true) after set_gap and before the CMD_1 table, because newer esp_lcd ST7789 init no longer turns the display on. In IDF v6.1 panel_st7789_init sends only SLPOUT(+100 ms), MADCTL, COLMOD, RAMCTRL; the i80 bus config has dma_burst_size (no psram_trans_align/sram_trans_align) and the panel config uses rgb_ele_order/data_endian (no color_space), so the factory's structs do not compile on 6.1. Factory sequence: reset, init, invert(true), swap_xy(true), mirror, set_gap(0,35), disp_on, CMD_1 table (120 ms after SLPOUT).

**Evidence:** commit c9b939f diff (+esp_lcd_panel_disp_on_off); examples/factory/factory.ino:259-331@1cc41eb; esp-idf v6.1 components/esp_lcd/src/esp_lcd_panel_st7789.c (panel_st7789_init), components/esp_lcd/include/esp_lcd_io_i80.h:28-32, components/esp_lcd/include/esp_lcd_panel_dev.h:21-27

**Action:** Gogadget init: RD high, then i80 bus (LCD_CLK_SRC_DEFAULT, dma_burst_size e.g. 64, max_transfer_bytes >= buffer), panel IO 10 MHz, st7789 panel (rgb_ele_order RGB, 16 bpp), reset, init, invert(true), swap_xy(true), mirror per orientation, set_gap(0,35), disp_on_off(true), CMD_1 table, then backlight. Unit-test the table sender.

### K07: Landscape orientation

Relevance: medium. Novelty: new.

**Fact:** For 320x170 landscape the factory uses invert_color(true) + swap_xy(true) + mirror(false,true) for 'screen facing you, USB on the left'; mirror(true,false) for USB on the right; set_gap(0,35).

**Evidence:** examples/factory/factory.ino:310-322@1cc41eb

**Action:** Expose both orientations (Kconfig or NVS setting); keep the LVGL logical resolution 320x170 in firmware and in the desktop simulator so screenshot tests match.

### K08: Pixel clock history

Relevance: medium. Novelty: refines.

**Fact:** Factory pixel clock went 6.528 MHz (2022-07) -> 20 MHz (2022-12-08, 58bd641) -> 16 MHz (2023-03-03, adc038e, with the comment 'Too low or too high pixel clock may cause screen mosaic'). lv_demos still uses 6.528 MHz; LilyGo-Display-IDF uses 10 MHz (known).

**Evidence:** examples/factory/pin_config.h:24-25@1cc41eb; examples/lv_demos/pin_config.h:14@1cc41eb; git show adc038e, 58bd641

**Action:** Keep 10 MHz (inside LilyGO's 6.5-20 MHz range); if artefacts ('mosaic') appear, try the factory's 16 MHz before going lower.

### K09: Factory draw buffer / DMA sizing

Relevance: medium. Novelty: new.

**Fact:** The factory allocates one full-frame LVGL buffer (320x170x2 = 108,800 B) with MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL, sets max_transfer_bytes to the full frame and trans_queue_depth=20, and runs Wi-Fi + SD_MMC concurrently on Arduino 2.x.

**Evidence:** examples/factory/factory.ino:276,285,333-335@1cc41eb; examples/factory/pin_config.h:29@1cc41eb

**Action:** With esp_lvgl_port use two partial internal-DMA buffers (e.g. 320x34 lines, ~21.8 KB each) to keep internal RAM free for Wi-Fi now and NimBLE later; allocate with esp_lcd_i80_alloc_draw_buffer() (IDF 6.1) for DMA alignment.

### K10: RGB565 byte order

Relevance: medium. Novelty: new.

**Fact:** LilyGO swaps colour bytes in software (lib/lv_conf.h LV_COLOR_16_SWAP 1; LilyGo-Display-IDF CONFIG_LV_COLOR_16_SWAP=y). IDF v6.1 offers zero-CPU alternatives: esp_lcd_panel_io_i80_config_t.flags.swap_color_bytes (LCD_CAM hardware swap) or esp_lcd_panel_dev_config_t.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE (ST7789 RAMCTRL endian bit). The CMD_1 table never writes RAMCTRL (0xB0), so it would not undo that setting.

**Evidence:** lib/lv_conf.h:30@1cc41eb; LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:3@b1a1cc5; esp-idf v6.1 esp_lcd_io_i80.h (flags.swap_color_bytes) and esp_lcd_panel_st7789.c (ramctl_val_2 |= ST7789_DATA_LITTLE_ENDIAN_BIT); examples/factory/factory.ino:133-149@1cc41eb

**Action:** Turn off esp_lvgl_port swap_bytes and try i80 flags.swap_color_bytes=1 (or panel little-endian) and check colours on hardware; this saves CPU on every flush.

### K11: Time zone handling

Relevance: high. Novelty: new.

**Fact:** Factory sets time with configTime(0,0,'pool.ntp.org','time.nist.gov') and applies a POSIX TZ via setenv('TZ')/tzset. CUSTOM_TIMEZONE 'CST-8' is hard-wired by default since c9b939f because the IP-geolocation path (https://ipapi.co/timezone/ with a pinned leaf certificate valid only to 2025-03-29) broke (#297). zones.h has 461 IANA-to-POSIX pairs taken from nayarsystems/posix_tz_db (MIT): 'Asia/Ho_Chi_Minh' -> '<+07>-7'.

**Evidence:** examples/factory/factory.ino:40-116,724-768@1cc41eb; examples/factory/pin_config.h:13-21@1cc41eb; examples/factory/zones.h:251@1cc41eb (added 0d31b85); issue #297 (lewisxhe 2025-01-13: 'it is obvious that the SSL certificate has expired'); issue #80; PR #93; github.com/nayarsystems/posix_tz_db (MIT)

**Action:** Hard-code TZ '<+07>-7' (Vietnam: UTC+7, no DST) and call setenv/tzset on every boot including deep-sleep wakes (env vars are not retained); don't use IP geolocation; if HTTPS is ever needed use esp_crt_bundle, never pinned leaf certificates.

### K12: SNTP lifecycle

Relevance: high. Novelty: new.

**Fact:** Factory registers sntp_set_time_sync_notification_cb and in the callback calls WiFi.disconnect() (radio stays on in Arduino). The 2026-07-28 fix moved SNTP init after esp_netif_init()/WiFi.mode(STA) (previously configTime() was called at the top of setup()) and uses esp_sntp_servermode_dhcp on core 3.x.

**Evidence:** examples/factory/factory.ino:453-471,489-491,716-722@1cc41eb; commit c9b939f diff

**Action:** In IDF: esp_netif_init -> event loop -> Wi-Fi start/connect -> esp_netif_sntp_init(...MULTIPLE(2,'pool.ntp.org','time.google.com')) -> esp_netif_sntp_sync_wait(timeout) -> esp_netif_sntp_deinit + esp_wifi_stop() (not just disconnect) -> set an RTC-memory 'time_valid' flag.

### K13: DHCP-provided NTP pitfall

Relevance: medium. Novelty: new.

**Fact:** Factory enables DHCP-supplied NTP servers (sntp_servermode_dhcp(1) under LWIP_DHCP_GET_NTP_SRV). In #297 a user's board never synced on DHCP but synced with a static IP. Root cause not confirmed; it is consistent with a DHCP-advertised NTP server replacing the pool.

**Evidence:** examples/factory/factory.ino:462-468@1cc41eb; issue #297 (roblatour 2025-01-14: 'if I assign a static ip address to the esp32 device then NTP works just fine')

**Action:** Keep esp_sntp_config_t.server_from_dhcp=false (the default), or set index_of_first_server so the configured pool server is never overwritten; log which server responded.

### K14: Never block on time validity

Relevance: medium. Novelty: new.

**Fact:** Factory loop() calls getLocalTime() every 100 ms. In Arduino it busy-waits up to 5000 ms until year>2016, so the LVGL loop stalls until the first sync (maintainer: 'otherwise it will block there').

**Evidence:** examples/factory/factory.ino:417-423@1cc41eb; arduino-esp32 2.0.14 cores/esp32/esp32-hal-time.c:83-96 and Arduino.h:210 (ms = 5000); issue #297 (lewisxhe 2025-01-14)

**Action:** Use a non-blocking validity check (RTC-memory flag or time() > build epoch) and render '--:--' or the last known time instead of waiting; never block the LVGL task.

### K15: Wi-Fi credential flow in the factory

Relevance: medium. Novelty: new.

**Fact:** Factory flow: scan and show SSIDs masked on screen -> compile-time credentials via WiFiMulti (30 s) -> credentials the Wi-Fi driver already stored (esp_wifi_get_config(WIFI_IF_STA)) -> SmartConfig/EspTouch for 120 s -> continue offline. Users mistook 'EspTouch' for the touch screen (#212). No BLE provisioning example exists in the repo.

**Evidence:** examples/factory/factory.ino:431-451,473-704@1cc41eb (rewritten in c9b939f); examples/factory/pin_config.h:3-11@1cc41eb; issues #212, #88

**Action:** Gogadget: console-provisioned credentials in its own NVS namespace, then (optionally) driver-stored config, then offline with a later retry; shorten the timeouts on battery; reuse the SSID masking idea in console/UI output. SmartConfig is still in IDF 6.1 (esp_smartconfig.h) as a no-app fallback.

### K16: Stale NVS from factory firmware

Relevance: medium. Novelty: new.

**Fact:** The shipped factory image uses nvs at 0x9000 (size 0x5000), the same offset as typical IDF layouts, and stores SmartConfig/Wi-Fi driver configuration there. `idf.py flash` does not erase NVS.

**Evidence:** firmware/t-display-s3-no-touch-20231013_0x0.bin partition table at 0x8000 (parsed: nvs 0x9000/0x5000); examples/factory/factory.ino:620-672@1cc41eb

**Action:** Run `idf.py erase-flash` once when converting a factory board, or call esp_wifi_set_storage(WIFI_STORAGE_RAM) and always pass credentials explicitly, so old SSIDs are not reused silently.

### K17: Prebuilt firmware facts

Relevance: medium. Novelty: new.

**Fact:** All 7 firmware/*.bin are merged images for offset 0x0, built with Arduino (app project 'arduino-lib-builder', ESP-IDF v4.4.4-v4.4.6), bootloader header DIO 80 MHz. The no-touch (20231013) and touch (20230417) factory images are byte-identical (SHA-256 f288081645ee...) and are built for 8 MB with the default_8MB layout (2x3.19 MB OTA, 1.5 MB SPIFFS, coredump at 0x7F0000). The other images use default_16MB. 'ScreenDetection' only prints compile-time TFT_eSPI pin settings ('PWR:%d - should be 15' ...); it does not probe the panel. Flash with esptool --chip esp32s3 write_flash --flash_mode dio --flash_freq 80m 0x0, or with LILYGO Spark or the web flasher.

**Evidence:** firmware/README.MD:11-24,26-37,85-92@1cc41eb; shasum of firmware/t-display-s3-no-touch-20231013_0x0.bin == firmware/t-display-s3-touch-20230417_0x0.bin; parsed partition tables/app descriptors; strings in firmware/t-display-s3-screen-detect-20230315_0x0.bin; firmware/images/log.png

**Action:** Keep the no-touch factory image as a hardware sanity/restore image. Distribute Gogadget the same way (`idf.py merge-bin` -> single image at 0x0, esptool-js for users) but with Gogadget's own QIO bootloader header. There is no software panel-variant detection: always send CMD_1.

### K18: Flash / PSRAM configuration

Relevance: high. Novelty: new.

**Fact:** The board is an ESP32-S3R8 (8 MB octal PSRAM in package) with a 16 MB Winbond W25Q128 ('25Q128' silkscreen; IDF boot log 'Enabling QIO for flash chip WinBond', QIO 80 MHz). LilyGO's board JSON: flash_mode qio, f_flash 80 MHz, memory_type qio_opi, -DBOARD_HAS_PSRAM, 16 MB, default_16MB.csv, USB VID:PID 303A:1001, maximum_ram_size 327680. A wrong PSRAM mode in IDF gave the boot loop 'failed to init external RAM' (#103); a wrong memory_type in PlatformIO (qspi_opi) caused a boot loop (#8).

**Evidence:** boards/lilygo-t-display-s3.json:2-23,36-42@1cc41eb; README.md:16-19@1cc41eb; issue #41 (mmMicky: check the silkscreen reads 25Q128); issue #103 (boot log; krupis 2023-05-23); issue #8; LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:7-13@b1a1cc5

**Action:** sdkconfig.defaults: CONFIG_ESPTOOLPY_FLASHSIZE_16MB, CONFIG_ESPTOOLPY_FLASHMODE_QIO, CONFIG_ESPTOOLPY_FLASHFREQ_80M. If PSRAM is enabled: CONFIG_SPIRAM=y + CONFIG_SPIRAM_MODE_OCT=y, and consider CONFIG_SPIRAM_MEMTEST=n (or no PSRAM at all) to shorten wake-to-first-frame for the glance clock.

### K19: Partition layout references

Relevance: medium. Novelty: new.

**Fact:** PlatformIO board uses default_16MB.csv (nvs 20 KB, otadata, 2x6.25 MB OTA, 3.375 MB SPIFFS, 64 KB coredump). The Arduino 'LilyGo T-Display-S3' board defaults to app3M_fat9M_16MB (2x3 MB OTA + 9.9 MB FAT + coredump). README says that board's partition is 'fixed', but current arduino-esp32 boards.txt also offers RainMaker and TinyUF2 schemes. Maintainer: custom CSVs need PlatformIO/ESP-IDF, not the Arduino IDE.

**Evidence:** boards/lilygo-t-display-s3.json:6@1cc41eb; README.md:108,119@1cc41eb; espressif/arduino-esp32 tools/partitions/default_16MB.csv and app3M_fat9M_16MB.csv; arduino-esp32 master boards.txt lilygo_t_display_s3.build.partitions and menu.PartitionScheme; issue #99 (mmMicky 2023-02-07)

**Action:** Write Gogadget's own partitions.csv: nvs, otadata, phy_init, ota_0/ota_1 (~4 MB each), a storage partition (LittleFS/FAT for fonts/assets), coredump 64 KB. Enable CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE once OTA exists.

### K20: OTA example

Relevance: medium. Novelty: new.

**Fact:** ota is ArduinoOTA push (espota UDP port 3232, optional password or MD5 hash; PlatformIO upload_protocol=espota + device IP) with a TFT progress bar. On Wi-Fi failure it calls ESP.restart() every 5 s (boot loop). No rollback/validation and no HTTPS-pull OTA anywhere in the repo. Issue #231 ('app returns to the old version when pressing reset') was closed without diagnosis.

**Evidence:** examples/ota/ota.ino:41-48,50-61,63-99,110@1cc41eb; platformio.ini:205-208@1cc41eb; issue #231

**Action:** Use esp_https_ota (or a USB-console update command) with app rollback and a mark-valid step after the first successful glance; show progress with an LVGL bar; never restart-loop on Wi-Fi failure while on battery.

### K21: PCBClock (what not to do)

Relevance: medium. Novelty: new.

**Fact:** PCBClock (copied from VolosR) blocks in setup() until Wi-Fi connects, uses configTime with hard-coded gmtOffset/daylightOffset (3600/3600), keeps Wi-Fi on, and pushes a full 320x170 sprite every loop from 22 full-screen RGB565 frames (22x108,800 B, about 2.4 MB flash; users needed bigger partitions, #256). Fonts are DSEG7. Its brightness button's LEDC call is commented out (02f0e9d). The upstream VolosR/PCBDesignClock repo has no licence.

**Evidence:** examples/PCBClock/PCBClock.ino:13-18,32,63-66,69-96,115-137@1cc41eb; examples/PCBClock/background.h:1-2 (framesNumber=22, frames[][54400]); issue #256; GitHub API VolosR/PCBDesignClock license=null

**Action:** Use it only as a counter-example: async Wi-Fi with timeouts, POSIX TZ, Wi-Fi off after sync, LVGL partial invalidation, compact assets. Do not reuse its code or graphics (unlicensed).

### K22: Factory clock UI patterns (LVGL 8 to 9)

Relevance: medium. Novelty: new.

**Fact:** Factory UI: lv_tileview with 3 tiles switched by GPIO14 with animation; hour and minute in two 140x140 boxes using a 120 px 4 bpp font limited to '0-9:' (11 glyphs, ~25 KB bitmap); colon blink via a 500 ms lv_timer; values pushed with lv_msg (LVGL 8.3 lv_msg_subsribe_obj) from loop() every 100 ms; an info page (chip, PSRAM, flash, Arduino version, SD, voltage).

**Evidence:** examples/factory/factory_gui.cpp:16-21,26-73,93-137,140-149@1cc41eb; examples/factory/factory_gui.h:3-11@1cc41eb; examples/factory/factory.ino:417-426@1cc41eb; examples/factory/font_Alibaba.c:2-3,3228,3280-3302@1cc41eb

**Action:** In LVGL 9.6 replace lv_msg with lv_subject_t (hour/minute/battery int subjects + label bindings/observers); generate a digits-only big font plus a Vietnamese text font with lv_font_conv using explicit ranges; update only on minute change during the 10 s glance; keep a diagnostics page (IDF/app version, battery mV, wake cause, reset reason).

### K23: Factory boot sequence is a glance anti-pattern

Relevance: medium. Novelty: new.

**Fact:** Before showing the clock the factory plays a 423 KB GIF splash, ramps brightness in 17 steps of 50 ms, waits LV_DELAY(800), plays tone(10,1000,5000)+delay(5000) on GPIO10 (left over from #278 testing), then scans Wi-Fi and may wait 30 s (connect) plus 120 s (SmartConfig).

**Evidence:** examples/factory/factory.ino:366-393,473-704@1cc41eb; examples/factory/lilygo2_gif.c:32557-32563 (data_size 422993); issue #278; examples/factory/pin_config.h:11

**Action:** The glance path must draw the time first (target a few hundred ms from wake) and push Wi-Fi/NTP to timer wakes or background; keep splash assets and long waits out of the wake path.

### K24: Buttons in official code (OneButton)

Relevance: high. Novelty: new.

**Fact:** factory, PokerS3, touch_test and usb_hid_pad use OneButton 2.0.4 (BSD-3-Clause; byte-identical to upstream tag 2.0.4 apart from CRLF) with defaults debounce 50 ms, click 400 ms, long press 800 ms. Factory: GPIO0 click sends SLPIN (0x10), sets GPIO15 and BL low, arms ext0 wake on GPIO14 (low) and deep-sleeps; GPIO14 click switches page. No LilyGO example wakes on GPIO0 or on both buttons (factory and Sleep both use ext0 on GPIO14; nothing uses ext1).

**Evidence:** lib/OneButton/src/OneButton.h:153-155@1cc41eb; lib/OneButton/src/OneButton.cpp:217-330@1cc41eb; examples/factory/factory.ino:124-125,395-408@1cc41eb; examples/Sleep/Sleep.ino:84@1cc41eb; diff vs github.com/mathertel/OneButton tag 2.0.4

**Action:** Port the OneButton state machine to C for the two-button driver (with attribution), ticking every 5-10 ms from esp_timer or after a GPIO interrupt. At boot, read the wake cause / ext1 status and start the waking button's state machine in the DOWN state so the waking press counts. Validate ext1 ANY_LOW on GPIO0+GPIO14 yourself; LilyGO never did.

### K25: Other button schemes in the repo

Relevance: low. Novelty: new.

**Fact:** Music-Explorer: level-triggered with a 250 ms lockout (auto-repeat while held); GPIO0 = next page, GPIO14 = next item / play-stop. Queue: digitalRead + delay(200). PokerS3: OneButton click/double-click on GPIO0 plus a raw edge flag on GPIO14; it seeds the RNG with analogRead(0) (GPIO0 is not an ADC pin).

**Evidence:** examples/T-Display-S3-Music-Explorer/T-Display-S3-Music-Explorer.ino:174,1564-1645@1cc41eb; examples/T-Display-S3-Queue/T-Display-S3-Queue.ino:36-39@1cc41eb; examples/PokerS3/PokerS3.ino:208-243@1cc41eb

**Action:** Prefer an event-based state machine over lockouts or delays; the 'left = page, right = item' mapping suits Gogadget's menus; seed any randomness with esp_random().

### K26: Peripherals stealing GPIO0

Relevance: low. Novelty: new.

**Fact:** A user's BOOT button (GPIO0) read permanently low because an audio library defaulted I2S MCLK to GPIO0.

**Evidence:** issue #101 (pdurys 2023-02-14, pointing to ESP32-audioI2S issue 408)

**Action:** When adding any peripheral (I2S, LEDC, RMT, etc.) set unused pins to -1 / I2S_GPIO_UNUSED explicitly so nothing grabs GPIO0 or GPIO14.

### K27: USB modes: Serial-JTAG vs TinyUSB

Relevance: medium. Novelty: new.

**Fact:** The HID/MIDI-device examples require ARDUINO_USB_MODE=0 (TinyUSB OTG), which detaches the USB-Serial-JTAG (console and auto-download); usb_hid_pad compiles to an empty sketch otherwise. USB host and USB device cannot coexist (single OTG port on GPIO19/20). Default envs use USB_MODE=1 (HW CDC+JTAG) and CDC_ON_BOOT=1.

**Evidence:** examples/usb_hid_pad/usb_hid_pad.ino:12-16,25-27,121-131@1cc41eb; platformio.ini:51-61,195-196,226-232,376-382@1cc41eb; lib/ESP32_Host_MIDI/src/USBDeviceConnection.h:14-33@1cc41eb

**Action:** Keep the USB-Serial-JTAG console now. If HID/MSC is wanted later, plan a TinyUSB build with a CDC-ACM console (esp_tinyusb) plus a 'reboot to download mode' console command, since auto-reset flashing is lost.

### K28: Console, flashing and debugging workflow

Relevance: medium. Novelty: refines.

**Fact:** Manual download mode: hold BOOT, press and release RST, release BOOT, flash, then press RST to run (an old README tip says code flashed in manual mode won't start until reset/replug). With USB CDC enabled, Arduino builds wait for USB when powered externally. ESP-IDF's default console is UART0 (GPIO43); one user had no monitor output until switching to USB-Serial-JTAG/CDC (#266). The board JSON enables on-chip USB-JTAG debugging (openocd esp32s3-builtin.cfg, debug_tool esp-builtin).

**Evidence:** README.md:215-232@1cc41eb; README.md@eed5e46:63-79 (Tips); firmware/README.MD:41-49@1cc41eb; issue #266 (BullT 2024-07-26); boards/lilygo-t-display-s3.json:28-30@1cc41eb; platformio.ini:43-49@1cc41eb; commit e53beab (#210)

**Action:** Document BOOT+RST recovery (needed because the device deep-sleeps after 10 s on battery and its USB port disappears); use `idf.py openocd`/gdb over the same cable; on battery, check that USB-Serial-JTAG logging with no host attached doesn't add wake latency and lower the log level in the glance path.

### K29: JST-SH connector and spare GPIOs

Relevance: medium. Novelty: new.

**Fact:** Non-touch pinout: left header 3V,1,2,3,10,11,12,13,NC,NC,G,5V; right header G,G,43,44,18,17,21,16,NC,G,G,3V; the 4-pin JST-SH 1.0 mm connector is G,3V,43,44 (UART0 TX/RX by default; the community uses it as Qwiic I2C with SDA=43/SCL=44). DS18B20 1-Wire on GPIO43 failed and the maintainer never found why. GPIO16 and 17 (and, per a follow-up photo, 18) have on-board pull-ups that can be removed on the non-touch board; GPIO21 and 17 are free on the non-touch board. The 3V header is limited to 100 mA and is the unswitched LDO output.

**Evidence:** image/T-DISPLAY-S3.jpg@1cc41eb; README.md:131-160,264-275@1cc41eb; issues #181, #225, #127 (teastainGit: Wire.begin(43,44)); #117 and #161 (lewisxhe: 'I didn't get to the bottom of why'); #313 (lewisxhe 2025-07-29: 'Pull-up resistors are already added to the 16 and 17'); #346 (lewisxhe 2026-08-07); #352

**Action:** Reserve the JST-SH as a 3.3 V debug UART or Qwiic port. Anything powered from the 3V pins drains the battery in deep sleep (add a high-side switch as in #352 if needed). In deep sleep leave GPIO16/17/18 as inputs; driving them low sinks current through the pull-ups.

### K30: UART pin labelling inconsistency

Relevance: low. Novelty: contradicts.

**Fact:** SerialExample uses Serial1 RX=17/TX=18 and Serial2 RX=21/TX=16, while the pinout image labels 18 as U1RXD and 17 as U1TXD, and the maintainer told a user to use Serial1.begin(9600, SERIAL_8N1, 18, 17). Both work because UART pins are routed through the GPIO matrix.

**Evidence:** examples/SerialExample/SerialExample.ino:13-31@1cc41eb; image/T-DISPLAY-S3.jpg; issue #164 (lewisxhe 2023-06-18); issue #70

**Action:** Always pass explicit pins to uart_set_pin() and document the chosen direction.

### K31: BLE facts for later provisioning

Relevance: medium. Novelty: new.

**Fact:** The ESP32-S3 supports BLE only (no Classic/BluetoothSerial). The repo's BLE MIDI server advertises a 128-bit service UUID with setScanResponse(false), so the device name may not fit in the 31-byte advertisement; the sender therefore matches by name OR UUID and runs scan/connect in separate FreeRTOS tasks pinned to core 0. The repo has no BLE provisioning example. In ESP-IDF 6.x, wifi_provisioning was removed and replaced by the managed component espressif/network_provisioning.

**Evidence:** issue #200 (lewisxhe 2023-11-28); lib/ESP32_Host_MIDI/src/BLEConnection.cpp:92-93@1cc41eb; examples/T-Display-S3-BLE-Sender/T-Display-S3-BLE-Sender.ino:80-86,101-117,121-162@1cc41eb; esp-idf v6.1 docs/en/migration-guides/release-6.x/6.0/provisioning.rst

**Action:** Plan BLE provisioning on espressif/network_provisioning (^1.1) with NimBLE; keep the advertised name short or in the scan response; run provisioning outside the LVGL task and post results back via subjects/queues.

### K32: Cross-task event plumbing pattern

Relevance: medium. Novelty: new.

**Fact:** ESP32_Host_MIDI (MIT) hands data from the BLE stack task to the main loop through a fixed 64-entry ring buffer guarded by a portMUX spinlock, dropping new data when full rather than blocking the BLE task; the send path takes a mutex with a 100 ms timeout. Examples redraw at most ~30 fps and only when state changed (memcmp).

**Evidence:** lib/ESP32_Host_MIDI/src/BLEConnection.cpp:67-83,112-135,167@1cc41eb; lib/ESP32_Host_MIDI/src/BLEConnection.h:47-58@1cc41eb; examples/T-Display-S3-BLE-Receiver/T-Display-S3-BLE-Receiver.ino:82-116@1cc41eb

**Action:** Same rule for Gogadget: button ISRs and Wi-Fi/SNTP/BLE callbacks only enqueue (FreeRTOS queue/ringbuf) or update subjects under lvgl_port_lock; never touch LVGL objects from other tasks; never block stack callbacks.

### K33: Community MIDI examples: quality and contradictions

Relevance: medium. Novelty: contradicts.

**Fact:** The 8 sauloverissimo examples (PRs #328-#336, merged Feb-Mar 2026) are not in either CI matrix (CI builds 19 of 32 example folders), even though the maintainer says 'All examples in the repository have passed the GitHub Arduino CI' (#351). Their mapping.h files carry wrong or unused pins (USB_DP 19 / USB_DN 18, TFT_DC 16, 'I2C 43/44 required when USB Host is active on pins 18/19'). T-Display-S3-USB-Device includes a non-existent '../../src/USBDeviceConnection.h' (the repo has no src/). All of them drive the AW9364 with LovyanGFX Light_PWM (LEDC 22 kHz, ch 7), the method LilyGO removed elsewhere as invalid; commit 02f0e9d did not touch these files.

**Evidence:** .github/workflows/arduino_ci.yml:19-38 and .github/workflows/platformio.yml:18-37@1cc41eb; issue #351 (lewisxhe 2026-09-30); examples/T-Display-S3-Queue/mapping.h:6-29@1cc41eb; examples/T-Display-S3-USB-Device/T-Display-S3-USB-Device.ino:26@1cc41eb; examples/T-Display-S3-Queue/ST7789_Handler.h:71-79@1cc41eb; examples/T-Display-S3-Piano-Debug/T-Display-S3-Piano-Debug.ino:29-50@1cc41eb; git show 02f0e9d --stat

**Action:** Use these examples for ideas only; take pin facts from examples/factory/pin_config.h and never copy PWM-backlight code.

### K34: CMD_1 sender bug in TFT_eSPI-style examples

Relevance: low. Novelty: new.

**Fact:** touch_test, usb_hid_pad, nes and GetBatteryVoltage send CMD_1 parameters with `for (j = 0; j < len & 0x7f; j++)`, which parses as `(j < len) & 0x7f`. For the SLPOUT entry (len 0x80) this sends 128 garbage parameter bytes read past data[14]. The esp_lcd variants (factory, lv_demos) pass `len & 0x7f` correctly.

**Evidence:** examples/touch_test/touch_test.ino:83, examples/usb_hid_pad/usb_hid_pad.ino:94, examples/nes/nes.ino:80, examples/GetBatteryVoltage/GetBatteryVoltage.ino:57 vs examples/factory/factory.ino:327@1cc41eb

**Action:** Port the CMD_1 sender from the esp_lcd variant only and unit-test that each entry sends exactly len & 0x7f bytes, with a 120 ms delay when bit 7 is set.

### K35: Code that won't port to IDF 6.1

Relevance: low. Novelty: new.

**Fact:** nes/osd.c and Music-Explorer use the legacy I2S driver (driver/i2s.h, i2s_driver_install/i2s_write), which is absent from the IDF v6.1 tree (only esp_driver_i2s i2s_std.h remains). SynthEngine.h (Piano, BLE-Receiver) already has an i2s_std code path for IDF 5+. The factory's esp_lcd structs also use fields removed in 6.1 (see K06).

**Evidence:** examples/nes/osd.c (includes driver/i2s.h)@1cc41eb; examples/T-Display-S3-Music-Explorer/T-Display-S3-Music-Explorer.ino:1228,1296@1cc41eb; examples/T-Display-S3-Piano/SynthEngine.h (ESP_ARDUINO_VERSION_MAJOR>=3 branch)@1cc41eb; esp-idf v6.1 tree (components/esp_driver_i2s/include/driver/i2s_std.h; no legacy driver/i2s.h)

**Action:** If audio (alarm tone through a DAC) is ever added, start from the i2s_std path; treat any Arduino example code as needing an API audit for IDF 6.1.

### K36: nes example throughput tricks

Relevance: low. Novelty: new.

**Fact:** nes writes frames straight into a panel window (CASET/RASET/RAMWR), converting 8-bit palette indices to RGB565 on the fly; it shrinks 240 lines to fit 170 by dropping 2 of every 7 lines; it de-initialises Wi-Fi and removes core-0 IDLE from the task watchdog; large buffers prefer PSRAM.

**Evidence:** examples/nes/nes.ino:67-71,146-157@1cc41eb; examples/nes/osd.c:25-31@1cc41eb

**Action:** Not needed for a clock (LVGL partial refresh already minimises bus traffic); do not disable watchdogs in Gogadget.

### K37: lib/TFT_eSPI patch status

Relevance: low. Novelty: new.

**Fact:** lib/TFT_eSPI is v2.5.43 from the PlatformIO registry (.piopm), updated 2026-03-02 (f15652b, #326). LilyGO changed only 2 things: User_Setup_Select.h enables Setup206_LilyGo_T_Display_S3.h, and Setup206 adds '#define USE_HSPI_PORT'. Other differences from the GitHub tag V2.5.43 are registry-vs-tag drift (fontHeight(int16_t), font>8 checks, Sprite pushImage bpp, STM32 swap). Setup206 uses INIT_SEQUENCE_3, CGRAM_OFFSET, TFT_INVERSION_ON, RGB order.

**Evidence:** diff lib/TFT_eSPI vs github.com/Bodmer/TFT_eSPI tag V2.5.43; lib/TFT_eSPI/User_Setup_Select.h:27,133@1cc41eb; lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h:5-13@1cc41eb; issue #326 (lewisxhe 2026-03-02: 'I only changed two parts')

**Action:** Nothing to port (Gogadget doesn't use TFT_eSPI); Setup206 is only a cross-check for inversion/offset settings.

### K38: lib/lvgl and lv_conf

Relevance: low. Novelty: new.

**Fact:** lib/lvgl is an 8.3.0-dev snapshot imported 2022-07-01 (758b2a3) and never updated: 25 src files differ from v8.3.0 final, and LilyGO only re-indented lv_conf.h (fafcb0b). The active lib/lv_conf.h: 16-bit colour, LV_COLOR_16_SWAP=1, malloc-based memory, tick = millis(), refresh period 16 ms, input read 30 ms, GIF and lv_msg enabled, Montserrat 14 only.

**Evidence:** lib/lvgl/lvgl.h:16-19@1cc41eb; lib/lv_conf.h:27-30,49-66,81-91,639,692@1cc41eb; diff vs lvgl tag v8.3.0

**Action:** Nothing reusable for LVGL 9.6 except the intent; on battery use a >=30 ms refresh/task period in esp_lvgl_port (16 ms gains nothing on a clock).

### K39: lib/ inventory (remaining libraries)

Relevance: low. Novelty: new.

**Fact:** Bundled vs current upstream (licence): GFX Library for Arduino 1.4.2 = upstream v1.4.2 (BSD; licence file missing in the copy; upstream v1.6.9). OneButton 2.0.4 (BSD-3; library.json still says 2.0.3; upstream 2.6.2). ESP32-audioI2S folder '3.0.6' but library.properties 2.0.0 (GPL-3.0; LilyGO-patched for the MIDI shield left/right channel error, 6ce938b/80a73fc 2023-08-18; upstream 4.0.0). arduino-nofrendo 1.2 (LGPL-2; LilyGO edits 76ca887, 02f0e9d). DabbleESP32 1.5.1 (LGPL-3.0; upstream 1.6.0). Adafruit MPR121 1.0.2 (BSD; upstream 1.2.1). PCA95x5 0.1.1 (MIT; upstream 0.1.3). PCF8575 1.1.0 (MIT; upstream 2.0.1). SensorLib 0.2.0 (MIT, lewisxhe; upstream 0.5.0). TouchLib 0.0.1 (MIT, Micky; LilyGO's own). ESP32_Host_MIDI 5.0.0 (MIT; upstream 7.3.0). Gingoduino 0.3.0 (MIT; upstream 0.6.0).

**Evidence:** lib/*/library.properties and LICENSE files @1cc41eb; git log -- lib/<name>; diff of OneButton and Arduino_GFX vs upstream tags; GitHub API tags/releases of each upstream (queried 2026-10-20)

**Action:** Don't vendor any of these into Gogadget; if a pattern is borrowed, keep GPL/LGPL code out of Gogadget's C sources.

### K40: Licensing of repo content

Relevance: medium. Novelty: new.

**Fact:** LICENSE is MIT (Copyright 2022 Xinyuan-LilyGO), but the content is mixed: WIFI_Audio_I2S.cpp, ULP_ADC.ino and ULP_Count.ino declare 'GPL 3.0' in their headers; PCBClock and PokerS3 are copied from VolosR repos that have no licence; zones.h derives from MIT posix_tz_db; fonts are DSEG7 and Noto Sans (OFL) plus an 'Alibaba' 120 px digit font with no stated licence.

**Evidence:** LICENSE:1-3@1cc41eb; examples/WIFI_Audio_I2S/WIFI_Audio_I2S.cpp:9; examples/ULP_ADC/ULP_ADC.ino:9; examples/ULP_Count/ULP_Count.ino:9; examples/PCBClock/PCBClock.ino:1; examples/PokerS3/PokerS3.ino:1; GitHub API VolosR/PCBDesignClock and VolosR/PokerS3 (license null); examples/PCBClock/fonts.h:1-2

**Action:** Port only MIT LilyGO code (factory, SerialExample, I2CScan, etc.) with attribution; build Vietnamese and digit fonts from OFL sources with full Vietnamese coverage instead of reusing font_Alibaba.

### K41: Version-support statements

Relevance: low. Novelty: refines.

**Fact:** README: examples were tested on Arduino-ESP32 2.0.14, newer versions 'may not work', TFT_eSPI broken above 2.0.14 as of 2024-08-02, ULP examples need 3.0.0-rc3. PlatformIO is pinned to espressif32@6.5.0 (packages: Arduino 2.0.14, ESP-IDF 5.1.2). Arduino CI went 2.0.14 -> unpinned latest (2026-03-09) -> pinned 2.0.17 (2026-07-16) with FQBN esp32:esp32:lilygo_t_display_s3. The '#error below 3.0' guards were removed from 14 examples on 2026-03-09. Maintainer on 2026-09-30: 'The repository example is based on version 2.0.14'. ESP-IDF is only covered by a link to LilyGo-Display-IDF; there is no IDF 6 example anywhere.

**Evidence:** README.md:57-58,86,122-124@1cc41eb; platformio.ini:39@1cc41eb; platform-espressif32 v6.5.0 platform.json (framework-arduinoespressif32 ~3.20014.0, framework-espidf ~3.50102.0); .github/workflows/arduino_ci.yml:53,61@1cc41eb; commits da301bc, 216a520, 395fa46, a1e3c39; issue #351; issue #326 (lewisxhe 2026-03-09)

**Action:** Treat the vendor examples as behavioural references only; Gogadget on IDF 6.1 must be validated independently on hardware.

### K42: README power wording vs maintainer statements

Relevance: medium. Novelty: refines.

**Fact:** After the 2026-10-13 doc fix, README still calls GPIO15 'LCD power control' and says without it 'the LCD and other peripherals will not work', whereas the maintainer says the header pins are directly on the LDO (#352) and FAQ #1 says GPIO15 is needed to light the backlight on battery/5V-header power. The README LED section says the green V3V LED stays on in USB mode and follows GPIO15 on battery, which implies the GPIO15-switched rail is forced on whenever USB is present (inference).

**Evidence:** README.md:149,161-171,191-200,253-256@1cc41eb; commit 1cc41eb; issue #352 (lewisxhe 2026-10-13: 'The pin header is connected directly to the LDO output and cannot be controlled.')

**Action:** In always-on USB mode don't rely on GPIO15 LOW to blank anything; blank via AW9364 shutdown (BL low >2.5 ms) and DISPOFF/SLPIN; confirm the USB-forced rail with a meter.

### K43: Electrical limits and enclosure

Relevance: low. Novelty: new.

**Fact:** README electrical table: USB-C 5 V in; battery 3.7-4.2 V on a JST 1.25 mm connector; VBUS header 5 V (from USB-C, load depends on the adapter); VBAT header 4.2 V; 3V header load <=100 mA. dimensions/ has LilyGO STL shell parts; its README recommends a LiPo of about 7x18x50 mm (the photo shows a 501646 380 mAh cell).

**Evidence:** README.md:264-275@1cc41eb; dimensions/README.MD@1cc41eb; image/image3.png@1cc41eb; dimensions/LilyGo-Shell/*.stl@1cc41eb

**Action:** Check the 250 mAh cell's connector and polarity (JST 1.25 mm); the official shell fits cells up to ~7x18x50 mm if an enclosure is needed.

### K44: tools/ image converter

Relevance: low. Novelty: new.

**Fact:** tools/ contains only Image2Lcd v2.9 (Windows, 2005 binaries, Chinese help files) and a screenshot of the settings LilyGO used: C array, horizontal scan, 16-bit RGB565 (R5 G6 B5), 320x170, no header, not MSB-first.

**Evidence:** tools/Image2Lcd.zip (Img2Lcd.exe dated 2005-02-01); tools/Image2Lcd_configuration.png@1cc41eb (added 1fb5bc4 2022-11-22)

**Action:** Use LVGL 9's own image converter (LVGLImage.py) in the build/simulator pipeline instead.

### K45: Repo maintenance signals

Relevance: low. Novelty: new.

**Fact:** issues.yaml marks issues stale after 30 days and closes them 14 days later (PRs after 1+1 days), so many 'closed' issues were never resolved. CI only compiles 19 examples; there are no runtime or screenshot tests.

**Evidence:** .github/workflows/issues.yaml:13-22@1cc41eb; .github/workflows/arduino_ci.yml and platformio.yml matrices@1cc41eb

**Action:** When mining issues read the thread instead of trusting 'closed'; there is no LilyGO test harness to borrow for Gogadget's headless LVGL screenshot tests.

### K46: Minor examples, one line each

Relevance: low. Novelty: new.

**Fact:** I2CScan: Wire scan on SDA 18/SCL 17 (non-touch board: bus has only pull-ups). MPR121TouchSensor: two external MPR121s at 0x5A/0x5B (MIDI shield). sd: SD_MMC 1-bit on CLK 11/CMD 13/D0 12 (TF shield) mounted with format_if_mount_failed=true (factory does the same). WIFI_Audio_I2S: MP3 stream to a PCM5102A on 43/44/18 (alt 11/12/13), hard-coded factory AP 'LilyGo-AABB', GPL header. T-Display-S3-MIDI: shield bring-up (PCA9535 0x20, 2x MPR121, IRQ GPIO2, PCM5102A 11/13/12, SD-SPI 16/21/3/10). touch_test/CapacitiveTouch: touch variant only; touch_test holds TOUCH_RES with gpio_hold_en across deep sleep and releases it with gpio_hold_dis at boot. ULP_ADC/ULP_Count: ULP-FSM macro programs on Arduino 3.0.0-rc3 (ADC on GPIO1, not the battery pin GPIO4), GPL headers.

**Evidence:** examples/I2CScan/I2CScan.ino:41-52; examples/MPR121TouchSensor/MPR121TouchSensor.ino:44-68; examples/sd/sd.ino:174-175 and examples/factory/factory.ino:390-391; examples/WIFI_Audio_I2S/WIFI_Audio_I2S.cpp:9,20-32; examples/T-Display-S3-MIDI/pin_config.h:41-65; examples/touch_test/touch_test.ino:66,149-154; examples/ULP_ADC/ULP_ADC.ino:9,38 (all @1cc41eb)

**Action:** Never auto-format storage; if deep-sleep pin states must be latched (e.g. GPIO15 LOW), use gpio_hold_en before sleep and gpio_hold_dis early at boot, and verify on the meter; the rest is not applicable to Gogadget.

### K47: Wi-Fi association quirk

Relevance: low. Novelty: new.

**Fact:** One user could not join WPA/WPA2-mixed networks until setting a minimum security of WPA2-PSK (another user's board was simply defective). A password ending in '.' crashed an older Arduino core during association (#221, attributed by the maintainer to the ESP core).

**Evidence:** issue #247 (mo-kuh 2024-07-07: 'WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK)'); issue #221 (lewisxhe 2024-03-05)

**Action:** Set wifi_config.sta.threshold.authmode explicitly (WPA2_PSK, allowing WPA3 transition) and test the console provisioning with special-character passwords.

### K48: AW9364 timing in the factory driver

Relevance: medium. Novelty: refines.

**Fact:** factory setBrightness: level 0 = BL low then 3 ms delay (shutdown); from off, BL high then 30 us before pulsing; each step is one back-to-back low/high toggle with no explicit pulse-width delay; levels 0-16, the number of pulses = (16 + (16 - target) - (16 - current)) % 16. It relies on Arduino digitalWrite latency and an uninterrupted loop.

**Evidence:** examples/factory/factory.ino:223-248,371-378@1cc41eb (added 2d3c294 for #278)

**Action:** In Gogadget's own driver keep the 30 us enable and >=3 ms shutdown, add explicit ~1 us high/low phases (esp_rom_delay_us) and emit the pulse train inside a short critical section so a preemption can't stretch a low phase into a shutdown; check against the AW9364 datasheet (it is not in datasheet/).

## Reusable code

### 1. IANA-to-POSIX time-zone table (Vietnam entry 'Asia/Ho_Chi_Minh' -> '<+07>-7') and the setenv/tzset usage

**Source:** examples/factory/zones.h:251 and examples/factory/factory.ino:724-768@1cc41eb (data originally from github.com/nayarsystems/posix_tz_db zones.csv)

**Licence:** MIT (repo LICENSE); upstream posix_tz_db is MIT

**Port notes:** For Gogadget only the constant '<+07>-7' is needed; call setenv("TZ", "<+07>-7", 1); tzset(); on every boot before localtime_r. If a zone picker is added later, turn the 461-entry Arduino String table into `static const struct {const char *iana; const char *posix;}` in flash (not RAM).

### 2. AW9364 pulse-count brightness routine (0-16 levels, 30 us enable, 3 ms shutdown)

**Source:** examples/factory/factory.ino:223-248@1cc41eb

**Licence:** MIT (repo LICENSE)

**Port notes:** Replace digitalWrite with gpio_set_level, or with direct GPIO register writes plus esp_rom_delay_us(1) per phase. Wrap the pulse train in portENTER_CRITICAL/portEXIT_CRITICAL so no low phase is stretched into the >2.5 ms shutdown window. Keep the level state in RTC memory so brightness survives deep sleep (the backlight itself restarts from off).

### 3. Button state machine (debounce 50 ms, click 400 ms, long press 800 ms, multi-click)

**Source:** lib/OneButton/src/OneButton.cpp:217-330 and lib/OneButton/src/OneButton.h:153-155@1cc41eb (identical to mathertel/OneButton tag 2.0.4)

**Licence:** BSD-3-Clause (Matthias Hertel), attribution required

**Port notes:** Rewrite as a C struct per button with tick(now_ms, level) driven by a 5-10 ms esp_timer, started by a GPIO interrupt so the timer can stop when idle. Add an init path that starts in DOWN state when the button caused the ext1 wake. Emit events to a FreeRTOS queue, not callbacks into LVGL.

### 4. SNTP init plus a sync callback that ends network use

**Source:** examples/factory/factory.ino:453-471 and 716-722@1cc41eb

**Licence:** MIT (repo LICENSE)

**Port notes:** Port to esp_netif_sntp: config with 2 servers, server_from_dhcp=false, sync_cb sets an RTC-memory time_valid flag and posts an event so the Wi-Fi task calls esp_netif_sntp_deinit() and esp_wifi_stop(). Use esp_netif_sntp_sync_wait() with a timeout suited to battery.

### 5. SSID masking helper for on-screen/console output

**Source:** examples/factory/factory.ino:431-451@1cc41eb

**Licence:** MIT (repo LICENSE)

**Port notes:** Trivial C port: show at most 18 characters, replace the last 2 with '**', and show '<hidden>' for empty SSIDs. Use it in the USB-console 'wifi status' command and on the settings page.

### 6. esp_lcd i80 + ST7789 bring-up sequence for the T-Display-S3 (RD high, orientation, gap, disp_on, CMD_1 loop with 0x80 = delay flag)

**Source:** examples/factory/factory.ino:126-149 and 257-331@1cc41eb (commit c9b939f added disp_on_off)

**Licence:** MIT (repo LICENSE)

**Port notes:** For IDF 6.1: replace .color_space with .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB; replace psram/sram_trans_align with .dma_burst_size; use LCD_CLK_SRC_DEFAULT and 10 MHz pclk; keep invert(true), swap_xy(true), mirror(false,true) (USB left) and set_gap(0,35); call esp_lcd_panel_disp_on_off(true) before sending the CMD_1 table; send each entry with len & 0x7f and wait 120 ms when bit 7 is set; consider flags.swap_color_bytes=1 instead of LVGL byte swapping.

### 7. Lock-protected ring buffer to move data from a stack callback task to the app task without blocking

**Source:** lib/ESP32_Host_MIDI/src/BLEConnection.cpp:112-135 and BLEConnection.h:47-58@1cc41eb

**Licence:** MIT (Saulo Veríssimo 2025)

**Port notes:** In C prefer xQueueSend with 0 timeout (or esp_ringbuf) from Wi-Fi/BLE/SNTP callbacks and drain in the UI/app task; keep the drop-when-full policy so stack tasks never block.

### 8. I2S standard-mode (IDF 5+) init for a simple PCM5102A/buzzer tone output

**Source:** examples/T-Display-S3-Piano/SynthEngine.h (ESP_ARDUINO_VERSION_MAJOR >= 3 branch: i2s_new_channel / i2s_channel_init_std_mode / i2s_channel_enable)@1cc41eb

**Licence:** MIT (repo LICENSE; contributed by Saulo Veríssimo)

**Port notes:** Only if an audible alarm via an I2S DAC is added later. Uses driver/i2s_std.h, which is still in IDF 6.1; strip the Arduino and FreeRTOS-queue wrapper; set unused pins (mclk, din) to I2S_GPIO_UNUSED so GPIO0 is never touched.

## Open questions

- Is the Gogadget board one of the older v1.2 PCBs with the weaker antenna match, or a later unit? #340 shows a photo of the new layout but gives no reliable marking; the production-date silkscreen is the only hint.
- How much does the RTC drift on the internal RC slow clock over hours or days of deep sleep on this board? Nothing in the repo or issues measures it, so the NTP resync interval is still a guess.
- Root cause of #297 (NTP failed with DHCP, worked with static IP): a DHCP-provided NTP server, router behaviour, or something else? Unconfirmed.
- Does USB presence force the GPIO15-switched rail (backlight supply, green LED, ADC divider) on regardless of GPIO15, as the README LED wording implies? Needs the schematic or a meter.
- Does ext1 ANY_LOW wake on GPIO0 + GPIO14 work reliably with the board's pull-ups, and does holding BOOT (GPIO0) at wake have any strapping side effect? No LilyGO example or issue covers waking on GPIO0 or on both buttons.
- Do LCD_CAM hardware byte swap (flags.swap_color_bytes) and ST7789 little-endian mode (RAMCTRL) both give correct colours with LilyGO's CMD_1 init on this panel? LilyGO only ever swaps bytes in software.
- What are the AW9364's exact pulse-width limits? The datasheet is not in datasheet/, and the factory code relies on digitalWrite latency rather than explicit pulse timing.
- Licence of the factory's 'font_Alibaba' 120 px digit font (likely Alibaba PuHuiTi): not stated anywhere in the repo.
- With CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG on IDF 6.1, how much do log writes delay a battery-powered wake when no USB host is attached? The README only documents the Arduino CDC wait.
- #231 ('app returns to the old version after reset') was closed without diagnosis; it is unclear whether it was an otadata/OTA-slot issue that Gogadget's flashing procedure should guard against.
