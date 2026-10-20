# Simulator architecture: sharing UI code between device and host

Date: 2026-10-20

Method: multi-agent research workflow: one research agent (run alongside the stack fact-checks)

Status: unverified research notes; the recommended option S1 was adopted in the settled decisions

Notes: the decisions adopt S1 with these differences: the IDF project lives in `firmware/` (not `fw/`), `core/` and `ui/` are C in a C23 subset accepted by GCC 15 and Apple clang (not C11), the test display is 320x170 landscape, and buttons use an own ISR + FSM driver (not iot_button).

Question: how to structure the repo so the same UI code (and as much app logic as possible) runs on the device and on the Mac, with a headless mode for automated screenshots. Options: **S1** a plain host CMake project (`sim/`) building LVGL 9.6, the `ui/` library and a pure-logic `core/` library with fake service adapters; **S2** the ESP-IDF linux target; **S3** ESP-IDF QEMU with the esp_lcd_qemu_rgb virtual framebuffer.

## Report

**Verdict: S1.** Of the three options, only S1 compiles the UI outside the IDF component graph. That matters for your previous failure. Inside IDF, every component implicitly gets `freertos`, `esp_hw_support`, `esp_system`, `log` and other headers ([Common Component Requirements](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/build-system.html)). So with S2 or S3, nothing in the build stops `ui/` from reaching into the platform again.

| | How the agent gets PNGs | CI | Maintenance | Stops UI depending on bsp/wifi |
|---|---|---|---|---|
| S1 | LVGL test module writes PNG files | any Linux or macOS runner; the headless build needs no SDL | low | yes: the host has no IDF headers, so such an include fails to compile |
| S2 | same test module, inside an IDF linux ELF | runner with a full ESP-IDF install | high | partly |
| S3 | QEMU `screendump` (PPM, then convert to PNG) | runner with ESP-IDF plus qemu-xtensa | high | no |

### S1: host CMake project `sim/` (recommended)

- **Versions:** LVGL 9.6.0 was released 2026-09-16 ([releases](https://github.com/lvgl/lvgl/releases)). The ESP Component Registry has `lvgl/lvgl 9.6.0~1` and `esp_lvgl_port 2.9.0`.
- **macOS:** LVGL's SDL driver page documents macOS setup with `brew install sdl2` ([SDL driver](https://lvgl.io/docs/open/integration/pc/sdl)). LVGL's maintained [lv_port_pc_vscode](https://github.com/lvgl/lv_port_pc_vscode) is a ready CMake + SDL template.
- **Headless PNGs:**
  - Enable `LV_USE_TEST` and `LV_USE_TEST_SCREENSHOT_COMPARE`; the second turns on LVGL's built-in lodepng.
  - Create the display with `lv_test_display_create(170,320)` and set it to RGB565 to match the panel.
  - Use `lv_test_fast_forward()` for deterministic time, and `lv_test_key_hit()` if the UI uses a keypad input device.
  - `lv_test_screenshot_compare()` creates the reference PNG when it is missing and writes `<name>_err.png` on a mismatch ([Test module](https://lvgl.io/docs/open/debugging/test)).
  - No window server is needed, so the agent (which can open PNGs directly) and CI run the same binary.
- **Same LVGL config on both sides:** LVGL's CMake build accepts `LV_BUILD_USE_KCONFIG` plus a list of config files in `LV_BUILD_DEFCONFIG_PATH` ([main.cmake](https://github.com/lvgl/lvgl/blob/v9.6.0/env_support/cmake/main.cmake)). One `lvgl.defconfig` file of `CONFIG_LV_*` settings can then feed both the firmware's `SDKCONFIG_DEFAULTS` and the simulator. A small simulator-only file adds the SDL and test options.
- **One CMakeLists that builds both ways:** `if(ESP_PLATFORM) idf_component_register() else() add_library()` is the pattern ESP-IDF documents for plain CMake components. LVGL builds itself this way ([CMakeLists](https://github.com/lvgl/lvgl/blob/v9.6.0/CMakeLists.txt)).
- **Keep the simulator single-threaded (no FreeRTOS):** on macOS, SDL must run on the main thread. Tactility had to write a main-thread bridge because FreeRTOS POSIX tasks run on separate pthreads ([sdl_bridge.h](https://github.com/TactilityProject/Tactility/blob/main/Devices/simulator/Source/drivers/sdl_bridge.h)).
- **CI:**
  - Job 1 runs `idf.py build` in the `espressif/idf:v6.1` container.
  - Job 2 runs `cmake` and `ctest` for the simulator and uploads any `*_err.png` files.
  - A third check confirms the simulator's LVGL tag matches `fw/dependencies.lock`.
- **Maintenance:** one extra CMake project, one fake per service interface, and keeping the shared LVGL config file in step. Nothing in IDF is mocked.

Suggested layout:
```
core/  C11 state machine, view-models, ports/*.h (no esp_*, no lvgl)
ui/    LVGL-only screens; links lvgl + core
fw/    idf.py project: main/ = composition root
       components/board_tdisplay_s3 (esp_lcd i80+ST7789, esp_lvgl_port, buttons, sleep)
       components/svc_idf (Wi-Fi/SNTP/NVS/battery adapters)
sim/   lvgl (same version), ui, core, fakes/, sim_sdl.c, shots/ (lv_test)
config/lvgl.defconfig   tests/golden/*.png
```
Map the GPIO0 and GPIO14 buttons to core events (press and long-press). Then `iot_button` on the device, SDL keys in the simulator, and the tests all drive the identical UI.

### S2: ESP-IDF `linux` target

- **Experimental:** the docs say there is "no guarantee for API stability".
  - The FreeRTOS POSIX simulator is driven by signals. The docs say to avoid `printf` and not to call FreeRTOS from threads FreeRTOS did not create.
  - Simulated components: esp_event, esp_netif, lwip, esp_http_client/server, mqtt, nvs_flash, fatfs/spiffs, log, heap, pthread.
  - Mock-only components: `driver`, `esp_timer`, `spi_flash` ([host apps](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/host-apps.html)).
- **More than a board-layer swap:**
  - `esp_lcd` stops early on linux ([CMakeLists](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_lcd/CMakeLists.txt)). `esp_lvgl_port` requires `esp_lcd`, so it cannot build ([CMakeLists](https://github.com/espressif/esp-bsp/blob/master/components/esp_lvgl_port/CMakeLists.txt)).
  - `esp_driver_gpio` and the local `esp_wifi` register as empty components.
  - LVGL setup and its tick source would therefore differ from the device too.
- **LVGL + SDL on macOS arm64: I found no evidence that it works.**
  - `app_main` runs on a FreeRTOS pthread, while the process main thread sits in `vTaskStartScheduler()` ([port_idf.c](https://github.com/espressif/esp-idf/blob/v6.1/components/freertos/FreeRTOS-Kernel/portable/linux/port_idf.c)).
  - SDL video belongs on the thread that initialised it, and SDL recommends the main thread ([SDL wiki](https://wiki.libsdl.org/SDL2/SDL_PumpEvents)).
  - On the linux target `main()` is a weak symbol ([startup.c](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_system/startup.c)), so a home-made bridge is possible. Nothing documents one.
- **No examples:** there are none in ESP-IDF or esp-bsp. GitHub code search turned up only hobby experiments.
- **Stability record:**
  - A release-v6.0 linux-target build break stayed open from May to July 2026 ([#18667](https://github.com/espressif/esp-idf/issues/18667)).
  - A macOS `set-target linux` build failure has been open since 2023 ([#12846](https://github.com/espressif/esp-idf/issues/12846)).
  - No build-time figures are published; every build runs idf.py's full configure step.
- The PNGs would come from the same LVGL test module as S1, so S2 adds risk without adding capability.

### S3: QEMU for ESP32-S3

- **Can it run the i80 ST7789 app unmodified? No.**
  - QEMU emulates its own virtual RGB framebuffer and octal PSRAM.
  - GPIO matrix/IOMUX, general-purpose SPI, I2S and Wi-Fi are not emulated, and LCD_CAM (the i80 peripheral) is not in the feature list ([feature table](https://github.com/espressif/esp-toolchain-docs/blob/main/qemu/README.md)).
  - You would swap the panel driver for `esp_lcd_qemu_rgb` ([component](https://github.com/espressif/idf-extra-components/tree/master/esp_lcd_qemu_rgb)). Its latest version is 1.0.2 from Feb 2024, and its example targets LVGL 8.3 without esp_lvgl_port.
  - Buttons would need some injection path, such as UART commands. Networking would need QEMU's emulated OpenCores Ethernet instead of Wi-Fi.
- **macOS arm64:** supported. Espressif ships prebuilt binaries and `idf.py qemu --graphics monitor` opens the display ([QEMU guide](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32s3/api-guides/tools/qemu.html)). Release `esp-develop-9.2.2-20260417` includes an `aarch64-apple-darwin` build ([releases](https://github.com/espressif/qemu/releases)).
- **Screenshots:** the QEMU monitor's `screendump` command writes PPM by default; PNG output needs a QEMU built with libpng ([ui.json](https://github.com/qemu/qemu/blob/master/qapi/ui.json)). Espressif's macOS build script uses `--without-default-features` and does not enable PNG ([configure-macos.sh](https://github.com/espressif/qemu/blob/esp-develop/.github/workflows/scripts/configure-macos.sh)), so plan to convert PPM to PNG.
- **Support and speed:** Espressif states it "does not provide support for QEMU". No speed figures are published, and every UI change means a full Xtensa build plus a flash image.

### Option for screenshots from the real device

`espressif/esp_lcd_screenshot` 0.1.0 (Sept 2026, needs ESP-IDF 6.0 or later) wraps a real `esp_lcd` panel. It streams the frames as base64 text over serial so a host-side test can compare them ([README](https://github.com/espressif/idf-extra-components/tree/master/esp_lcd_screenshot)). Later, a development-only Kconfig option could use it to compare the real ST7789 output with the simulator's reference images. The component is brand new, so keep it optional.

## Recommendation

Adopt S1. Put `core/` (pure C with service interfaces only) and `ui/` (LVGL only) at the repo root, each with one CMakeLists that builds both as an IDF component and as a plain CMake library. The `fw/` IDF project and a single-threaded `sim/` host project both use them; `sim/` builds LVGL 9.6.0 at exactly the version locked in `fw/dependencies.lock`, from one shared `CONFIG_LV_*` config file. Give the agent two host targets: an SDL window for interactive work, and a headless LVGL test-module binary (170x320 RGB565 test display, `lv_test_fast_forward`, `lv_test_screenshot_compare`) that writes PNGs and diff images. Make the host build plus ctest a required CI check next to `idf.py build` in `espressif/idf:v6.1`, so any `esp_*` or `freertos` include in `ui/` or `core/` fails the PR. Don't use S2 (experimental; no esp_lcd, esp_lvgl_port, GPIO or Wi-Fi on the linux target; conflicts with SDL's main-thread requirement on macOS; no examples) or S3 (no i80/LCD_CAM or GPIO emulation, a different panel driver and LVGL setup, no Espressif support) for UI work. Optionally add `esp_lcd_screenshot` later behind a development-only Kconfig option to compare real ST7789 frames with the simulator's reference images.

## Examples of shared UI code in open-source projects

### ESPTransit (joined/ESPTransit): ESP32-P4 transit departures display, 2026

https://github.com/joined/ESPTransit

`shared/` holds the app logic and `shared/ui/{screens,components,fonts}`. `esp/` is the ESP-IDF project: `esp/main/CMakeLists.txt` globs `../../shared` sources into the main component, and `esp/components/bsp_<board>` has one component per board, plus a vendored copy of esp_lvgl_port. `simulator/` is a plain CMake project: it fetches LVGL v9.4.0, matching the firmware lockfile's 9.4.0, and the FreeRTOS-Kernel POSIX port. Its `src/mock/` fakes the BSP display, esp_log/esp_err, NVS (stored as a JSON file), Wi-Fi and HTTP, and an HTTP control server accepts taps and screenshot commands. `tests/ui/` holds pytest flows and golden PNGs per board and orientation; CI runs them with SDL_VIDEODRIVER=dummy, and firmware builds in espressif/idf:v5.5.2. The repo has AGENTS.md and CLAUDE.md for coding agents. Caveats: the shared UI compiles inside the IDF main component, which pulls in esp_wifi and other platform components, so only the simulator enforces the boundary; it also carries a vendored copy of esp_lvgl_port.

### Kern (odudex/Kern): ESP32-P4 air-gapped Bitcoin signer in C, 2025-2026

https://github.com/odudex/Kern

The ESP-IDF project is at the repo root: `main/{core,ui,pages,qr,utils}`, plus `components/` with one BSP per board (wave_35/43/4b/5/7b, crowpanel) and libraries, and `sdkconfig.defaults.<board>` files. `simulator/` is a plain CMake project that lists `../main/ui`, `../main/core`, `../main/utils` and component sources directly. `simulator/platform/` provides `bsp_sim` (lv_sdl_window_create, SDL mouse, lvgl_port_lock on a pthread mutex), `esp_idf_stubs` (FreeRTOS, NVS and system calls on pthreads and files), `sd_card_sim`, `video_sim` and a hand-written sdkconfig.h. The simulator uses `../managed_components/lvgl__lvgl` when it exists, so it gets exactly the firmware's LVGL; otherwise it fetches v9.5.0, matching the lockfile's 9.5.0. It builds on macOS (Homebrew sdl2 and mbedtls) and Linux; firmware CI builds in espressif/idf:v6.1. The README says the simulator was written by an AI agent. Caveats: it stubs IDF APIs rather than separating them out, and the source lists are long and maintained by hand.

### Tactility (TactilityProject/Tactility): ESP32 operating system, 905 stars, active Oct 2026

https://github.com/TactilityProject/Tactility

One root CMakeLists.txt switches on ENV{ESP_IDF_VERSION}. With ESP-IDF, it builds an IDF project whose EXTRA_COMPONENT_DIRS are Tactility, Devices/<board>, Drivers, Modules, Platforms/platform-esp32, TactilityKernel and TactilityFreeRtos. Without it, it builds a plain CMake project that adds the same modules plus Platforms/platform-posix, Devices/simulator, Libraries/lvgl, Libraries/FreeRTOS-Kernel (GCC_POSIX port) and a static Libraries/SDL. The board layer is Devices/<board>, about 70 boards including lilygo-tdisplay-s3 and the simulator. The Build Simulator workflow runs on ubuntu-24.04 x86_64 and macos-latest aarch64, and passed on 2026-10-19; firmware builds with ESP-IDF v6.1 via esp-idf-ci-action. Devices/simulator/Source/drivers/sdl_bridge.* runs SDL on the macOS main thread while the LVGL task lives on a FreeRTOS pthread. Caveat: the architecture is OS-sized, much more than a single-device app needs.
