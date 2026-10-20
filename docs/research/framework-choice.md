# Framework and build system choice

Date: 2026-10-20

Method: single research agent (web and primary sources; no files written, nothing built)

Status: unverified research notes; key claims were re-checked later in stack-esp-idf.md (A1-A3, A6, A7)

Notes: the settled decisions pin LVGL to exactly 9.6.0 (this report suggested `~9.6.0`) and rule out Arduino entirely, including the Arduino-as-a-component option mentioned at the end.

Question: Arduino or ESP-IDF, and PlatformIO or native idf.py, for this board (ESP32-S3R8, 16 MB flash, ST7789 on 8-bit i80, LVGL 9, Wi-Fi + NTP, NVS, OTA later), with stability, no hand-patched drivers, painless upgrades and an agent-friendly CLI as priorities.

**Verdict:** use native **ESP-IDF v6.1 with idf.py and the Component Manager**, and only pieces Espressif maintains: esp_lcd's i80 bus with its built-in ST7789 driver, `espressif/esp_lvgl_port`, and `lvgl/lvgl` 9.6. Skip Arduino and PlatformIO for this project.

## 1. ESP-IDF

- **Releases and support periods** (from Espressif's chart, generated 16 Oct 2026):

  | Line | Released | Latest patch | Service until | End of life |
  |---|---|---|---|---|
  | v6.1 | 25 Aug 2026 | none yet | 25 Aug 2027 | 25 Feb 2029 |
  | v6.0 | 20 Mar 2026 | v6.0.3 (2 Sep 2026) | 20 Mar 2027 | 20 Sep 2028 |
  | v5.5 | 21 Jul 2025 | v5.5.6 (16 Oct 2026) | ended 21 Jul 2026 | 21 Jan 2028 |
  | v5.4 | — | — | ended | 5 Jul 2027 |
  | v5.3 | — | — | ended | 25 Jan 2027 |

  Each release gets 30 months: 12 of service (recommended for new projects), then 18 of maintenance (security and severe fixes only).
- **v6.0 breaking changes relevant here:**
  - Default C standard is now gnu23 and C++ is gnu++26; Picolibc replaces Newlib.
  - mbedTLS 4 with PSA crypto: DHE and RSA key-exchange cipher suites are gone, and deprecated CAs were removed from the certificate bundle. Test your OTA server before relying on it.
  - cJSON, MQTT and wifi_provisioning moved out of IDF into the component registry.
  - Legacy ADC, I2S and RMT drivers were removed; the legacy I2C driver is end-of-life.
  - lwIP's `sntp.h` was removed; use `esp_sntp.h`.
  - esp_lcd: pins are now `gpio_num_t`, `dma_burst_size` replaces `*_trans_align`, `rgb_ele_order` replaces `color_space`, and `esp_lcd_panel_disp_off()` is gone.

  v6.1 is "mostly compatible" with 6.0. Its new FourCC colour-format change only affects RGB/DSI panels; I checked that `bits_per_pixel` is still in the v6.1 panel config header.
- **Display path:**
  - The i80 bus and `esp_lcd_new_panel_st7789()` are still built into v6.1 (only the NT35510 driver moved out).
  - Espressif's official example `examples/peripherals/lcd/i80_controller` defaults to ST7789, an 8-bit bus and LVGL 9 pulled from the registry. It drops the pixel clock to 2 MHz when colour buffers sit in PSRAM on the S3, so keep LVGL draw buffers in internal DMA RAM.
  - In the registry: `lvgl/lvgl` 9.6.0 (16 Sep 2026) and `esp_lvgl_port` 2.9.0 (7 Aug 2026, accepts LVGL 8 or 9). Its open issues are about RGB, DSI and touch, not i80.
  - There is no official board-support package for the T-Display S3. The board setup is roughly 100 lines using public APIs: GPIO15 high for power, backlight on GPIO38, RD (GPIO9) held high, a 35-pixel column offset, and colour inversion.
- **Component Manager:**
  - Current versions are v3.1.2 (2 Sep 2026; the v3 line needs IDF 6.0+) and v2.5.2 for IDF 5.x.
  - It only re-resolves versions when a manifest changes, the target changes, the IDF version changes, or you run `idf.py update-dependencies`. Otherwise it just reports that newer versions exist.
  - Commit `dependencies.lock` and gitignore `managed_components`.
  - To patch a component, move it into `components/` (this is Espressif's own FAQ advice).
  - Detecting local edits needs strict-checksum mode (`IDF_COMPONENT_STRICT_CHECKSUM=1`; the variable name is UNVERIFIED).
- **sdkconfig:**
  - Commit `sdkconfig.defaults` and `sdkconfig.defaults.esp32s3`; the base file must exist for the chip-specific one to load.
  - Generate defaults with `idf.py save-defconfig`.
  - Values already in `sdkconfig` take precedence, so regenerate `sdkconfig` after editing the defaults.
- **Host-side testing:**
  - The linux target is "experimental" and "also works on macOS". It runs either with CMock mocks or a POSIX simulation that covers nvs_flash, esp_event, lwip, esp_netif and esp_http_client.
  - QEMU emulates the ESP32-S3 on arm64 macOS and has a virtual framebuffer (`espressif/esp_lcd_qemu_rgb`), so CI can render LVGL screens.
  - pytest-embedded 2.9.3 (31 Aug 2026) has idf, qemu, wokwi and arduino services.
- **Agent tooling:**
  - `build/compile_commands.json` is generated natively, and v6.1 installs esp-clangd by default.
  - esp-idf-monitor 1.10 or later (required by IDF 6.x) switches to a scriptable mode when stdin is not a terminal, e.g. `printf 'reset\nexpect --timeout 10 READY\n' | idf-monitor PORT`. Whether this also works through `idf.py monitor` is UNVERIFIED.

## 2. Arduino-ESP32

- **Releases:**
  - 3.3.12 (18 Sep 2026) is built on IDF 5.5.5, which is now in maintenance.
  - 4.0.0-RC1 (23 Sep 2026) is built on IDF 6.1 and refactors Bluetooth/BLE.
  - There is no 3.x→4.0 migration guide yet. Milestones for 3.4 and 3.5 exist, but how long 3.x will be maintained is UNVERIFIED.
- **Arduino as an ESP-IDF component** (registry versions 3.3.12 and 4.0.0-rc1):
  - The docs say 3.3.12 is "compatible with ESP-IDF v5.5" and call this route "recommended for advanced users".
  - It requires `CONFIG_FREERTOS_HZ=1000`.
  - Its manifest pulls in 25 conditional dependencies (esp-sr, Zigbee, RainMaker and others).
- **TFT_eSPI — avoid:**
  - The last tagged release is V2.5.43 (6 Mar 2024). Fixes for core 3.x exist only on master (Feb 2026, untagged); the PlatformIO registry still serves 2.5.43.
  - It has 355 open issues, #3329 (S3 broken on cores after 2.0.14) is still open, and the T-Display S3 setup PR #3818 is unmerged.
  - Parallel-bus DMA is supported only on RP2040.
  - LilyGO's own repo still pins `espressif32@6.5.0` (Arduino 2.0.14), says TFT_eSPI fails above 2.0.14, and ships patched copies of the libraries.
- **LovyanGFX** 1.2.33 (13 Oct 2026): very active and already has IDF 6 fixes. But its S3 8-bit parallel bus writes LCD_CAM and GDMA registers directly, which ties it to IDF internals. It has no preset for the non-AMOLED T-Display S3.
- **Arduino_GFX** 1.6.9 (7 Oct 2026): has a T-Display S3 preset. On core 3.x and later its `Arduino_ESP32LCD8` bus wraps the public esp_lcd i80 API; the README note saying it is unsupported on v3 is out of date. This is the least fragile Arduino option.
- **LVGL 9.6 on Arduino:** `lv_conf.h` has to sit next to the library folder. LVGL has built-in glue for LovyanGFX and TFT_eSPI.

## 3. PlatformIO

- **Official `platform-espressif32` v8.0.0** (released 19 Oct 2026, yesterday): Arduino 3.3.12 (on IDF 5.5.5) and ESP-IDF 6.1. Using `framework = arduino, espidf` together forces IDF 5.5.5. Every release from 6.12.0 (Jul 2025) through v7.1.3 (11 Sep 2026) shipped Arduino 2.0.17.
- **Relationship with Espressif:**
  - In Nov 2023 PlatformIO's CEO wrote that "Espressif has ceased support for new products in PlatformIO".
  - Espressif's Arduino project manager called PlatformIO a separate project that "we are not able to keep compatible".
  - PlatformIO's 19 Oct 2026 press release promises "a more direct role" and thanks Espressif, but announces no partnership and no update schedule.
- **Pain points with ESP-IDF through PlatformIO:**
  - Each platform release supports one IDF version. IDF 6.0 arrived 41 days late, and IDF 5.5.1, 5.5.2, 6.0.2 and 6.0.3 were never shipped.
  - PlatformIO runs IDF's CMake only to read its project model, then rebuilds everything with its own SCons build. That brings `sdkconfig.<env>` file naming, a forced `IDF_COMPONENT_OVERWRITE_MANAGED_COMPONENTS=1`, no spaces allowed in paths, and a 250-character path limit.
  - Open bugs include #1730 (Espressif's own esp_lvgl_adapter component breaks the build; the fix PR #1756 has been open since July), #1663 (Component Manager crash, open since Oct 2025) and #1633 (configure fails on macOS).
  - `pio run -t compiledb` leaves out toolchain include paths by default.
- **pioarduino fork:**
  - Stable 55.03.312-1 (22 Sep 2026) is Arduino 3.3.12 with IDF 5.5.5; 61.04.00-RC1 is Arduino 4.0-RC1 with IDF 6.1. It ships about 2 days after each Arduino release.
  - It needs PlatformIO Core 6.2.0 or later and downloads its packages from GitHub URLs. IDF comes from a fork, `pioarduino/esp-idf` (whether patched is UNVERIFIED).
  - It is mostly one maintainer (Jason2866, 853 commits). Board bugs "will not be fixed" by the maintainers, its documentation is an AI-generated DeepWiki, and "-1" re-releases are common.
  - Espressif ships `tools/pioarduino-build.py` inside arduino-esp32.

## 4. Why ESP-IDF, the risks, and keeping upgrades painless

- **Why native ESP-IDF:**
  - v6.1 has the longest support (end of life Feb 2029).
  - Your exact display path is Espressif's default example.
  - Every dependency is first-party and hash-locked.
  - All CLI tools are first-party.
  - There is no CMake-to-SCons bridge, which is what forced the deep digging last time. Both CLIs work fine for an agent, so the bridge is what decides it.
- **Risks:**
  - **ESP-IDF:**
    - More boilerplate for Wi-Fi, SNTP and NVS.
    - Third-party registry components may lag behind 6.x; check each manifest's `idf` range.
    - The mbedTLS 4 changes affect OTA over TLS.
    - v6.1 has no bugfix release yet. The conservative alternative is v6.0.3, then a minor upgrade later.
  - **Arduino 3.3:** it sits on an IDF in maintenance, the 4.0 migration is imminent, and the display libraries are third-party.
  - **Official PlatformIO:** Arduino 3 support is one day old after a 3-year gap, it skips IDF patch releases, and it has the bridge bugs above.
  - **pioarduino:** depends mostly on one maintainer, and packages come from GitHub URLs.
- **Keeping upgrades painless:**
  - **IDF version:**
    - Build in CI on the `espressif/idf:v6.1` Docker image pinned by digest; the `release-v6.1` and `latest` tags move.
    - Docker on macOS can't pass USB through, so flash from the host or over RFC2217.
    - Install locally with `brew tap espressif/eim && brew install eim && eim install -i v6.1`.
  - **Dependencies:**
    - In `main/idf_component.yml`, pin `idf: ">=6.1,<6.2"`, `espressif/esp_lvgl_port: "~2.9.0"` and `lvgl/lvgl: "~9.6.0"`, and commit the lock file.
    - Bump them with `idf.py update-dependencies` in separate commits, gated by CI (a build plus a QEMU/pytest smoke test).
  - **Cadence:**
    - Take 6.1.x bugfix releases promptly.
    - Do one IDF minor upgrade a year using the migration guides; each LVGL minor is supported for one year.
    - If you need Arduino libraries later, add Arduino 4.x as a component on the same IDF 6.1.

**Sources:**
- github.com/espressif/esp-idf/releases (v6.0, v6.1)
- dl.espressif.com/dl/esp-idf/support-periods.svg
- github.com/espressif/esp-idf/blob/master/SUPPORT_POLICY.md
- docs.espressif.com/projects/esp-idf/en/latest/esp32s3/migration-guides/release-6.x/6.0/peripherals.html
- docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-reference/peripherals/lcd/i80_lcd.html
- github.com/espressif/esp-idf/tree/v6.1/examples/peripherals/lcd/i80_controller
- components.espressif.com (lvgl/lvgl, espressif/esp_lvgl_port, espressif/arduino-esp32)
- github.com/espressif/idf-component-manager/releases
- docs.espressif.com/projects/idf-component-manager/en/latest/ (version solver, FAQ)
- docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/ (host-apps, tools/qemu, tools/idf-docker-image)
- github.com/espressif/esp-idf-monitor
- github.com/espressif/pytest-embedded
- github.com/espressif/arduino-esp32/releases
- docs.espressif.com/projects/arduino-esp32/en/latest/esp-idf_component.html
- github.com/Bodmer/TFT_eSPI (issues #3329, #3355, #3818)
- github.com/Xinyuan-LilyGO/T-Display-S3
- github.com/lovyan03/LovyanGFX/releases
- github.com/moononournation/Arduino_GFX (src/databus/Arduino_ESP32LCD8.cpp)
- lvgl.io/docs/open/9.6/integration/frameworks/arduino
- github.com/platformio/platform-espressif32/releases/tag/v8.0.0
- piolabs.com/blog/press-releases/esp32-arduino-v3-ai-development.html
- github.com/platformio/platform-espressif32/issues/1225#issuecomment-1826363522
- github.com/espressif/arduino-esp32/pull/8606#issuecomment-1805781410
- docs.platformio.org/en/latest/frameworks/espidf.html
- github.com/platformio/platform-espressif32/issues/1730, /1663, /1633
- github.com/pioarduino/platform-espressif32/releases
- hub.docker.com/r/espressif/idf/tags
