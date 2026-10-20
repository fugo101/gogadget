# Starting points: example repos and BSPs

Date: 2026-10-20

Method: single research agent (data from the GitHub API and the ESP Registry API; no files written)

Status: unverified research notes; stack-critic.md corrects the hiruna reference and adds LilyGo-Display-IDF

Notes: the settled decisions use an own ISR + FSM button driver (not espressif/button) and target the non-touch board (no CST816 driver). stack-critic.md shows that hiruna's AW9364 code is LEDC PWM, so use that repo for pin maps only.

Superseded by decision: CMD_1 is sent byte-for-byte on every init, not optionally (ADR-0006); LVGL is pinned at exactly 9.6.0 (ADR-0002); the simulator is Gogadget's own host CMake project in `sim/`, not lv_port_pc_vscode (ADR-0003). Corrected later: GPIO15 switches only V3V (backlight supply, green LED) and the GPIO4 divider, while the panel logic runs on the always-on 3.3 V rail ([t-display-s3.md](../hardware/t-display-s3.md#switched-rails)).

Question: which starting point to use: KamranAghlami/T-Display-S3, the official Xinyuan-LilyGO/T-Display-S3, a PlatformIO project, or a native ESP-IDF project with the Component Manager.

**Recommendation: start a native ESP-IDF project (option D).** None of the existing repos is a good base to fork. There is no ready-made T-Display-S3 board package (BSP) to install; you write one small board file yourself.

## 1. KamranAghlami/T-Display-S3

- **What it is:** an app boilerplate, not a library. PlatformIO + **Arduino** + LVGL.
- **Repo stats:** MIT, 56 stars, 4 forks, 0 open issues (4 closed), 57 commits, one release (v1.0.0).
- **Maintenance:** last commit **2024-12-02** ("bump lvgl up to v9.2.2"). Dormant for about 22 months; the 2025 issues got no reply from the author.
- **Build:** `platform = espressif32` and `lvgl/lvgl @ ^9.2.2` are both **unpinned**.
  - It was built on Arduino core 2.0.x (ESP-IDF 4.4).
  - A fresh install would now pull PlatformIO espressif32 v8.0.0 (released 2026-10-19, Arduino 3.3.12) and LVGL 9.6.0. **UNVERIFIED** whether it still compiles.
  - Custom OTA partitions: 2×2.94 MB app slots plus 10.9 MB SPIFFS.
- **Structure:** C++ with no `-std` set.
  - An abstract `application` class (`on_create` / `on_update`).
  - Singletons in `hardware/{display,button,battery,storage,config.h}`.
  - A physics demo as the example.
  - About 580 hand-written lines, plus the 271-line example.
- **Display and LVGL:**
  - Calls ESP-IDF's maintained `esp_lcd` directly: i80 bus plus the built-in ST7789 driver at 10 MHz, with gap 35, color invert and swap_xy.
  - The LVGL 9 glue is hand-written: two 16-line DMA buffers in internal RAM, partial rendering, software byte-swap, an `esp_timer` tick, and `lv_timer_handler()` called in `loop()`.
  - The 1 MB LVGL heap comes from `malloc`, so it lands in PSRAM.
  - The two buttons act as a keypad input.
- **Weak spots:**
  - Hardware is initialised in static constructors, before `setup()` runs.
  - No touch support and no LVGL locking.
  - Backlight uses `analogWrite`, which in practice is only on/off.
  - It skips LilyGO's extra init table for the newer panel revision.
  - It uses `.color_space`, which ESP-IDF 6.x removed, so it breaks on Arduino core 4.0 (now at RC1, built on IDF 6.1).
- **Tests / simulator:** none.
- **CI:** GitHub Actions runs `pio run` and `buildfs` and publishes releases on tags. It also has a step that posts to the author's private Node-RED webhook; remove that in any fork.

## 2. Xinyuan-LilyGO/T-Display-S3 (official)

- **Repo stats:** MIT, 1194 stars, 274 forks, 2 open issues. Last commit **2026-10-13**; the maintainer (lewisxhe) actively answers issues.
- **Examples:** 31, all Arduino: factory, lv_demos, tft, TFT_Rainbow, Arduino_GFX demos, touch_test, CapacitiveTouch, Sleep, ota, nes, usb_hid_pad, ULP, MIDI/BLE.
- **ESP-IDF:** no IDF examples here. They live in a separate repo, `LilyGo-Display-IDF`: IDF 5.3.0 + LVGL 8, last commit 2025-11-03, 9 open issues.
- **Bundled libraries in `lib/`:**
  - lvgl **8.3.0-dev** (a 2022 snapshot) with its own `lv_conf.h`.
  - TFT_eSPI 2.5.43, **patched** (Setup206 selected, `USE_HSPI_PORT` added).
  - Arduino_GFX 1.4.2, TouchLib 0.0.1, SensorLib 0.2.0, OneButton 2.0.4.
- **Arduino core support:** 2.x only.
  - PlatformIO is pinned to `espressif32@6.5.0` (Arduino core 2.0.14); the Arduino CI uses core 2.0.17.
  - The README says it is tested on 2.0.14 and newer cores "may not work", citing TFT_eSPI#3329 (still open).
  - `lv_demos.ino` contains an `#error` on ESP-IDF 5+, so it refuses to build on core 3.x.
  - Only the factory example was fixed for core 3.3.11 (2026-07-28).
  - One `platformio.ini` drives all examples (`src_dir = examples/${default_envs}`, with per-env `lib_ignore`).
- **Recurring issue themes:**
  - TFT_eSPI setup and core-version errors (#319, #320, #326).
  - Black screen with only the backlight on, often power or hardware (#293, #323, #343).
  - Stale build environments (#345).
  - Documentation errors (#316, #352).

**Hardware facts.** These match across the README, the factory `pin_config.h`, the schematic, Kamran's repo and hiruna's repo.
- **Display wiring:** D0–D7 = 39, 40, 41, 42, 45, 46, 47, 48; WR 8; RD 9 (hold it HIGH); DC 7; CS 6; RST 5.
- **Panel:** ST7789V, 170×320, gap 35, colors inverted. There are two panel revisions, and LilyGO sends an extra init table (`LCD_MODULE_CMD_1`).
- **GPIO15:** powers the LCD and peripherals and must be driven HIGH. The schematic says "Battery power must set IO15 to HIGH". The header 3V pin is **not** switched by GPIO15 (#352).
- **Backlight:** GPIO38 drives an **AW9364** chip with 16-step pulse dimming. LilyGO removed its PWM backlight code as "invalid" on 2026-03-09.
- **Battery:** ADC on GPIO4, ×2 divider; the reading is only valid with USB unplugged.
- **I2C:** SDA 18, SCL 17. The touch-variant pinmap image had these swapped (#316).
- **Touch:** INT 16, RST 21. The controller is CST816 at 0x15; early units used CST328 at 0x1A.
- **Buttons, memory:** buttons on GPIO0 and GPIO14. Flash is 16 MB QIO; PSRAM is 8 MB OPI.
- **Qwiic connector:** UNVERIFIED.

## 3. ESP-IDF BSPs and ports

- **No T-Display-S3 BSP exists on the ESP Component Registry.** I searched lilygo, t-display(-s3), tdisplay(s3), t_display_s3 and st7789. Near misses:
  - espp has BSPs only for t-deck, t-dongle-s3 and t5-47.
  - LilyGO's `llgok/lilygo_device_driver` 2.1.0 (GPL-3.0) covers only P4/C5 boards.
  - ESP32_Display_Panel 1.0.4 and esp-board-manager list no LilyGO boards.
- **Maintained building blocks:**
  - `lvgl/lvgl` 9.6.0~1 (2026-09-17).
  - `espressif/esp_lvgl_port` 2.9.0 (2026-08-07): supports LVGL 8 and 9, runs the LVGL task and lock, and handles touch, button and encoder input. Needs IDF ≥5.2 and includes IDF 6 fixes.
  - `esp_lcd_touch_cst816s` 1.1.2 (2026-08-12).
  - ST7789 and the i80 bus are still **built into `esp_lcd`** (checked in IDF v5.5.6 and v6.1).
  - Espressif's own `i80_controller` example defaults to ST7789 + LVGL 9.2.2.
- **Templates on GitHub:**
  - **hiruna/esp-idf-t-display-s3** (26 stars, MIT, last commit 2025-03-01) is the best reference. It has a BSP-style `tdisplays3` component and uses IDF ≥5.3 (locked 5.5.0), LVGL ^9 (locked 9.2.2), esp_lvgl_port, 16-step AW9364 dimming and an ADC battery reading. Caveat: it pulls esp_lvgl_port from esp-bsp git master and the AW9364 driver from git `main`, both unpinned.
  - zef/t-display-s3-espidf-lvgl (17 stars): PlatformIO with the ESP-IDF framework (IDF 5.3.1), LVGL 9.1.0, last commit 2025-03-11, license unclear.
  - Everything else has 0 stars.

## 4. Comparison

| | A: fork Kamran | B: LilyGO repo | C: PlatformIO (Arduino or IDF) | D: native IDF + component registry |
|---|---|---|---|---|
| Stability | Worked on hardware in 2024; with today's floating versions, untested | Tested by the vendor, but only on core 2.0.x | Arduino 3.3.12 in official PlatformIO is 1 day old; the pioarduino fork is the proven route | Display and LVGL code maintained by Espressif; `dependencies.lock` makes builds reproducible |
| Version freshness | LVGL 9 (floating) | LVGL 8.3-dev, IDF 4.4 | LVGL 9.6; IDF 5.5.5 (Arduino) or 6.1 (IDF framework) | LVGL 9.6.0, IDF 6.1 / 5.5.6 on release day |
| Hand-maintained code | About 580 lines of hardware and LVGL glue | Needs a rewrite to LVGL 9 | Arduino: same glue as A (registry components need `arduino, espidf`); IDF: like D | About 150–250 lines in one board file |
| Upgrade path | Dormant upstream; breaks on IDF 6 | Vendor stays on old cores | Waits on PlatformIO releases (IDF 6.0 support came about 6 weeks after release) | `idf.py update-dependencies` plus Espressif's migration guides |
| Ecosystem | Arduino libraries; small community | Largest community | PlatformIO registry plus Arduino libraries | ESP Registry, esp-bsp patterns |
| AI / CLI friendliness | Small, readable code | Monorepo with vendored libraries confuses agents | `pio` CLI works well, but mixing platform forks and core 2.x/3.x APIs is a common LLM trap | `idf.py`, text `sdkconfig.defaults`, lockfile, Linux host target for tests, and `idf.py mcp-server` (IDF ≥6.0) |

## Verdict

1. **Create the project:** run `idf.py create-project`.
2. **Pin dependencies:** `lvgl/lvgl` 9.6.x, `espressif/esp_lvgl_port` 2.9.x, `espressif/button` and `esp_lcd_touch_cst816s`, using the built-in ST7789 driver over i80. Pin ESP-IDF to v6.1, falling back to v5.5.6 if a component lags (IDF 6 support for each component is UNVERIFIED).
3. **Write one small `board/` component**, using hiruna's repo and LilyGO's factory example as references. It should:
   - drive GPIO15 HIGH and RD HIGH;
   - set gap, invert and rotation;
   - optionally send the panel-revision init table;
   - include a roughly 30-line AW9364 pulse driver of your own rather than a dependency on git `main`;
   - read the battery ADC;
   - probe the touch controller to tell CST816 from CST328.
4. **Keep the UI in its own component** that also builds against LVGL's SDL simulator (`lv_port_pc_vscode` runs on macOS after `brew install sdl2 cmake`). That lets Claude iterate on screens without flashing hardware.

Use B only as the source of hardware facts. Borrow A's layering of app over hardware code, but don't fork it.

## Sources

- https://github.com/KamranAghlami/T-Display-S3
- https://github.com/Xinyuan-LilyGO/T-Display-S3 (issues 316, 319, 320, 326, 345, 352; commits 02f0e9d, c9b939f; schematic/T_Display_S3.pdf)
- https://github.com/Xinyuan-LilyGO/LilyGo-Display-IDF
- https://github.com/Xinyuan-LilyGO/lilygo_device_driver
- https://github.com/hiruna/esp-idf-t-display-s3
- https://github.com/zef/t-display-s3-espidf-lvgl
- https://github.com/Bodmer/TFT_eSPI/issues/3329
- https://components.espressif.com/components/lvgl/lvgl
- https://components.espressif.com/components/espressif/esp_lvgl_port
- https://components.espressif.com/components/espressif/esp_lcd_touch_cst816s
- https://github.com/espressif/esp-idf/tree/v6.1/examples/peripherals/lcd/i80_controller
- https://github.com/espressif/esp-idf/blob/v6.1/tools/idf_py_actions/mcp_ext.py
- https://github.com/espressif/esp-idf/releases
- https://github.com/espressif/arduino-esp32/releases
- https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0
- https://github.com/pioarduino/platform-espressif32/releases
- https://github.com/lvgl/lv_port_pc_vscode
