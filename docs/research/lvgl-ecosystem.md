# LVGL ecosystem and simulator options

Date: 2026-10-20

Method: single research agent (web and primary sources; no files written, nothing built)

Status: unverified research notes; key claims were re-checked later in stack-lvgl.md (B1-B7)

Notes: the settled decisions differ in places: buttons map GPIO14 press = Next, hold = Back and GPIO0 press = Enter; the CPU runs at 160 MHz; the pixel clock is 10 MHz; lv_font_conv runs from a pinned script, not from unpinned GitHub master; navigation lives in core/ state, so the UI uses no `lv_group` or LVGL keypad device; and the layout adds `core/` and keeps the shared LVGL fragment in `config/lvgl.defconfig`.

Superseded by decision: fonts are converted ahead of time (DSEG7 and Be Vietnam Pro through the pinned lv_font_conv script), not rendered at runtime with Tiny TTF or FreeType; `LV_ATTRIBUTE_FAST_MEM_USE_IRAM` stays off because it breaks the LVGL 9.6 build with GCC 15 (`firmware/sdkconfig.defaults`). Corrected later: GPIO15 does not power the LCD logic; it switches the backlight supply (V3V) and the GPIO4 divider ([t-display-s3.md](../hardware/t-display-s3.md#switched-rails)).

Question: what LVGL offers in 2026 for an agent-built UI on this board: versions, a desktop simulator on macOS, headless screenshot tests, design tools and licences, ESP-IDF integration, fonts (Vietnamese) and translations.

I didn't build or run anything, so all commands below are untested. This Mac currently has no cmake, ninja, pkg-config, SDL or ESP-IDF; Node 26 and python3 are installed.

## 1. What https://lvgl.io/docs/open is

- It is the documentation site for "LVGL Open", the free MIT-licensed library, currently at v9.6. docs.lvgl.io now redirects there; sibling sites are /docs/pro and /docs/safe.
- It is neither a design tool nor a simulator for your own UI. The widget pages embed in-browser (WASM) runs of LVGL's 300+ built-in examples only.
- sim.lvgl.io still exists but only offers MicroPython up to LVGL 9.0.
- The free "simulator" the user is looking for is the desktop PC project (§3) plus LVGL's test module (§4).
- For the agent: add `.mdx` to any docs URL to get Markdown, and `/docs/open/llms.txt` exists.

## 2. Versions

- **Latest stable:** v9.6.0, released 16 Sep 2026; the ESP Registry has `lvgl/lvgl 9.6.0~1` from 17 Sep.
  - The changelog calls it "the last release of the v9 series… a stability release".
  - It brings a faster software renderer, public headers under `include/lvgl/`, Kconfig as the single config source, and `LV_COLOR_FORMAT_DEFAULT` replacing `LV_COLOR_DEPTH`.
  - Everything marked `LV_DEPRECATED` is removed in v10.
- **Cadence:**
  - Policy: a minor release every 3–4 months, a major about yearly, and 1 year of support per minor.
  - Actual: 9.3 (3 Jun 2025), 9.4 (16 Oct 2025), 9.5 (18 Feb 2026), 9.6 (16 Sep 2026).
  - There has been no patch release since 9.2.2.
- **v10:** master is `10.0.0-dev`. The v10.0.0 milestone is open with no due date and no announced release.
- **Recommendation:** start on 9.6.0.
  - Pin it with the git tag and a committed `dependencies.lock`.
  - Turn deprecation `#warning`s into errors so the code is ready for v10.

## 3. Desktop simulator on macOS

- **Project options:**
  - `lv_port_pc_vscode` uses CMake and SDL, with optional FreeRTOS.
  - `lv_port_pc_eclipse` is described as "Recommended on Linux and Mac".
  - Both master branches follow LVGL master (v10-dev), so use branch `release/v9.6`.
  - `lv_port_linux` is Linux-only.
- **SDL driver built into LVGL:**
  - `LV_USE_SDL` with `lv_sdl_window_create`, `lv_sdl_keyboard_create` and `lv_sdl_mouse_create`.
  - It uses the SDL2 API; 9.6 has no native SDL3 driver.
  - Homebrew's `sdl2` now installs sdl2-compat 2.32.74 running on SDL3 3.4.18, which provides the SDL2 API.
  - 9.6's CMake looks for dependencies with find_package, then pkg-config, then downloads and builds them.
- **Build commands:**
```
brew install cmake ninja pkg-config sdl2
git clone --recursive -b release/v9.6 https://github.com/lvgl/lv_port_pc_vscode
cd lv_port_pc_vscode && cmake -B build -G Ninja && cmake --build build && ./bin/main
```
- **Known pitfall:** a CMake 4.x configure error (issue #113) was fixed on master only.
  - `release/v9.6` still adds `${PROJECT_SOURCE_DIR}` to the lvgl include directories, which is what caused it.
  - Remove that path if configure fails (UNVERIFIED whether 9.6 triggers it).
- **Same UI sources for firmware and simulator:** yes.
  - LVGL's own CMakeLists already branches on `ESP_PLATFORM`.
  - LVGL Pro's exported projects use the same idea: the UI is a `lib-ui` library linked to an `lvgl` target.
  - Proposed layout (my suggestion, not an official template):
```
ui/        pure LVGL C + assets; if(ESP_PLATFORM) idf_component_register() else() add_library()
firmware/  ESP-IDF app (i80 panel, esp_lvgl_port, buttons); EXTRA_COMPONENT_DIRS ../ui
sim/       lvgl @v9.6.0; SDL app + headless tests
tests/ref/ golden PNGs
lvgl.defconfig  shared CONFIG_LV_* (LV_BUILD_DEFCONFIG_PATH / SDKCONFIG_DEFAULTS)
```
- **Input:** the UI code should only know an `lv_group` and the keys `LV_KEY_NEXT`, `LV_KEY_ENTER` and `LV_KEY_ESC`.
  - Then the GPIO buttons, SDL keys (Tab, Enter, Esc) and `lv_test_key_hit()` all drive the same code.
- **Colour:** render RGB565 in the simulator and tests so screenshots match the panel.

## 4. Checking visuals without a human

- **Version and setup:**
  - The test module has been public since v9.3.0 (3 Jun 2025); before that it lived only inside LVGL's own repo.
  - Enable `LV_USE_TEST`, `LV_USE_TEST_SCREENSHOT_COMPARE` (which pulls in the bundled PNG library) and `LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE` (on by default).
- **Headless display:** `lv_test_display_create(w,h)` creates an in-memory display with no window.
- **Simulated input:** `lv_test_indev_create_all()`, `lv_test_key_hit`, `lv_test_encoder_turn`, `lv_test_mouse_click_at`.
- **Fast-forwarding time:** `lv_test_wait` and `lv_test_fast_forward` advance LVGL's tick counter directly, so don't install a tick callback in tests.
- **Screenshot compare:** `lv_test_screenshot_compare("ref.png")` returns PASSED, FAILED or NO_REFERENCE_IMAGE.
  - On failure it writes `ref_err.png` next to the reference.
  - Compile-time macros `REF_IMGS_PATH` and `REF_IMG_TOLERANCE` (default 0) set the folder and the allowed per-channel difference.
```c
lv_display_t *d = lv_test_display_create(320,170);
lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
lv_test_indev_create_all();  /* attach keypad to your group */
lv_screen_load(ui_home_create()); lv_test_fast_forward(1000);
assert(lv_test_screenshot_compare("tests/ref/home.png") == LV_TEST_SCREENSHOT_RESULT_PASSED);
```
- **Dumping a PNG of any screen:**
  - Call `lv_snapshot_take(obj, LV_COLOR_FORMAT_ARGB8888)`.
  - Swap the blue and red bytes (`lv_color32_t` is stored B, G, R, A).
  - Encode with the bundled `src/libs/lodepng/lodepng.h`.
  - Shortcut: compare against a file that doesn't exist yet, and the module creates it.
- **Agent loop, locally:** run `ctest`, then open the PNGs with the Read tool. A 3× nearest-neighbour upscale makes the 320×170 images easier to inspect.
- **Agent loop, in CI:**
  - No SDL is needed.
  - Build with `LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE=0` so missing references fail.
  - Upload the `*_err.png` files as artifacts.
- **Determinism:** TinyTTF, FreeType and ThorVG use floating point, so output may not be bit-identical across CPU architectures (UNVERIFIED). Generate the reference images on the CI OS, or allow a small tolerance.
- **LVGL Pro alternative:** its CLI (`lvglpro screenshot`, `run-all-tests`) does the same for XML projects, but it needs a paid licence token.

## 5. UI design tools and licensing

- **LVGL Pro:** Editor v2.0.5, released 14 Oct 2026, with a macOS Apple Silicon build.
  - **Free tier:** since v2.0.0 (6 Jul 2026), the Community tier is free with no time limit for personal and open-source work. It includes all editor features and online sharing for public repos, but no commercial use.
  - **No headless tooling for free:** the CLI is "not available with the Community and Evaluation licenses"; the Product tier costs $20,000 per product.
  - **No runtime loader:** the engine that loads XML at runtime "is not publicly available".
  - **Restrictive spec licence:** the XML format is under an "XML License v1.0" that forbids third-party editors and converters.
- **XML runtime in open-source LVGL:** it shipped in 9.3–9.4, covering components, styles, subjects (data binding), translations and tests. It was removed in 9.5.0; the changelog says "development continues outside the main repository". 9.6 has no XML runtime.
- **SquareLine Studio** 1.6.2 (10 Aug 2026) supports LVGL up to 9.5, not 9.6.
  - The free Personal tier is non-commercial and capped at 10 screens, 150 widgets and 1 component.
  - Small Business costs 66 USD/month.
  - Its command-line export only works after a licence has been activated in the GUI.
- **EEZ Studio** v0.29.0 (2 Aug 2026) is open source (GPL-3.0) and you own the generated code.
  - It supports LVGL 8.4 and 9.2.2–9.5.
  - It can't generate code from the command line (issue #340, open since 2024).
- **NXP GUI Guider** v2.0.1 (14 Aug 2026, LVGL 9.4) is free, but its licence restricts use to "combination with a NXP Product", so it can't be used for an ESP32.
- **What an agent can maintain long-term:**
  - Only hand-written LVGL C plus the simulator and the test module gives a free loop the agent can run entirely on its own.
  - LVGL Pro's XML is agent-friendly text, but on the free tier the agent can't validate it, generate code or take screenshots without the GUI.
  - SquareLine and EEZ store projects as JSON and regenerate the C on export.
- **Also useful:** LVGL runs a read-only documentation MCP server at `https://lvgl.mcp.kapa.ai/` that works with Claude Code. It needs a login; whether it's free for non-Pro users is UNVERIFIED.

## 6. ESP-IDF integration

- **Component versions:**
  - `espressif/esp_lvgl_port` 2.9.0 (7 Aug 2026) requires IDF ≥5.2 and accepts `lvgl >=8,<10`, so it pulls in 9.6.0~1.
  - There is no explicit statement that it supports 9.6. However, Espressif's esp-bsp CI builds against 9.6 (PR #832, 5 Oct 2026).
  - The latest lvgl/lvgl on the ESP Registry is 9.6.0~1.
  - Current IDF releases are v6.1 (27 Aug 2026) and v5.5.6.
- **What esp_lvgl_port supports:**
  - SPI, i80 and I2C panels through esp_lcd, plus RGB and MIPI-DSI.
  - Buffer options: `buffer_size`, `double_buffer`, `buff_dma`, `buff_spiram`, `trans_size`, `full_refresh`, `direct_mode`, `swap_bytes`.
  - Hardware or software rotation. Its `avoid_tearing` option only works with RGB and DSI panels, not i80.
  - Inputs: touch, USB HID, encoder (`espressif/knob` 1.1.0) and navigation buttons (`espressif/button` 4.2.2).
- **Two-button problem:** the navigation-button helper requires exactly three buttons (prev, next, enter) and rejects a missing one. With two buttons, write a small keypad input device instead, e.g. click = next, long-press = enter.
- **ESP32-S3 SIMD drawing:** set `CONFIG_LV_DRAW_SW_ASM_CUSTOM=y`, `CONFIG_LV_USE_DRAW_SW_ASM=255` and `CONFIG_LV_DRAW_SW_ASM_CUSTOM_INCLUDE="esp_lvgl_port_lv_blend.h"`.
- **Configuration:** use Kconfig rather than `lv_conf.h`.
  - Put the `CONFIG_LV_*` options in `sdkconfig.defaults`.
  - The `ui/` component's `idf_component.yml` should depend on `lvgl/lvgl`.
- **T-Display-S3 pins**, from LilyGO's `pin_config.h`:
  - 8-bit i80 data lines D0–D7 on GPIO39–42 and 45–48; WR 8, RD 9, DC 7, CS 6, RST 5; backlight 38.
  - GPIO15 must be driven high to power the LCD.
  - Buttons are on GPIO0 and GPIO14.
  - There is no tearing-effect (TE) pin.
- **Panel init:** `invert_color(true)`, `swap_xy(true)`, `mirror(false,true)`, `set_gap(0,35)`. LilyGO's example uses a 6.528 MHz pixel clock.
- **Rotation pitfall:** esp_lvgl_port never re-applies the panel's 35-pixel offset, so rotating at runtime breaks the image. Fix the orientation at init.
- **Performance:**
  - Keep draw buffers in internal DMA-capable RAM rather than PSRAM.
  - Espressif measured that buffers above 25% of the screen gain little and below 10% hurt badly. For example, use two 320×40 RGB565 buffers of 25.6 KB each.
  - Swap the RGB565 byte order with the i80 option `flags.swap_color_bytes` or `LV_COLOR_FORMAT_RGB565_SWAPPED`, not on the CPU.
  - Also set `COMPILER_OPTIMIZATION_PERF`, 240 MHz CPU, `LV_ATTRIBUTE_FAST_MEM_USE_IRAM` and `LV_DEF_REFR_PERIOD` of 10–16 ms.
  - Without a TE pin, tearing can't be synchronised away; a faster pixel clock only narrows the window.

## 7. Fonts and images

- **Font conversion:** the npm release of `lv_font_conv` (1.5.3, Jun 2024) lags the GitHub master, so run it from GitHub:
  `npx github:lvgl/lv_font_conv --font X.ttf --size 16 --bpp 4 --format lvgl -r <ranges> -o font_vi_16.c`
  - The online converter (lvgl.io/tools/fontconverter) is a web form only.
- **Runtime font rendering:** use Tiny TTF (`LV_USE_TINY_TTF`) or FreeType instead of pre-converted fonts. With IDF ≥5.5, enabling FreeType automatically adds `espressif/freetype`.
- **Vietnamese:** LVGL's built-in Montserrat fonts contain only `0x20-0x7F,0xB0,0x2022` plus icons, so no Vietnamese. These ranges cover all 134 Vietnamese letters plus ₫ (I computed them from the precomposed forms):
  `0x20-0x7E,0xC0-0xC3,0xC8-0xCA,0xCC-0xCD,0xD2-0xD5,0xD9-0xDA,0xDD,0xE0-0xE3,0xE8-0xEA,0xEC-0xED,0xF2-0xF5,0xF9-0xFA,0xFD,0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9,0x20AB`
- **Vietnamese pitfalls:**
  - Convert strings to precomposed form (Unicode NFC). Text with separate combining accent marks likely won't render correctly, because LVGL doesn't position them (UNVERIFIED for 9.6).
  - Stacked accents (Ấ, Ỗ) can be clipped with tight line spacing or 9.6's new `text_leading_trim`. Keep a reference screenshot of a Vietnamese pangram to catch this.
- **Image conversion:** `scripts/LVGLImage.py` in the LVGL repo.
  - It takes PNG input and needs the Python packages pypng and lz4.
  - Options: `--ofmt C|BIN`, `--cf RGB565|RGB565A8|ARGB8888|I1–I8`, `--compress RLE|LZ4`, `--rgb565dither`.
- **Generating assets at build time:**
  - Use CMake `add_custom_command` in `ui/` to run both converters.
  - An ESP-IDF-only alternative is esp_lvgl_port's `lvgl_port_create_c_image()`, which runs when CMake configures.
  - Committing the generated `.c` files keeps the firmware and simulator builds identical.

## 8. Translations (i18n)

- LVGL has a built-in translation module, `lv_translation` (`LV_USE_TRANSLATION`), since v9.4.0 (16 Oct 2025).
- Main calls: `lv_translation_add_static/dynamic`, `lv_translation_set_language`, `lv_tr()`, and `lv_label_set_translation_tag`. `LV_EVENT_TRANSLATION_LANGUAGE_CHANGED` is sent to widgets when the language changes.
- 9.6 adds dropdown and roller support, and a missing translation falls back to the tag name.
- An older alternative is `lv_i18n` (MIT, YAML ↔ C, plurals).

## Sources

- https://lvgl.io/docs/open
- https://lvgl.io/docs/open/introduction/policies
- https://lvgl.io/docs/open/debugging/test
- https://lvgl.io/docs/open/integration/pc/sdl
- https://lvgl.io/docs/open/main-modules/translation
- https://lvgl.io/docs/open/integration/chip_vendors/espressif
- https://github.com/lvgl/lvgl/blob/v9.6.0/docs/src/changelog/CHANGELOG.mdx
- https://github.com/lvgl/lvgl/blob/v9.5.0/docs/src/CHANGELOG.rst
- https://github.com/lvgl/lv_port_pc_vscode
- https://lvgl.io/pro/pricing
- https://lvgl.io/docs/pro/cli
- https://lvgl.io/docs/pro/integration/loading-xml-files
- https://lvgl.io/docs/pro/ai
- https://github.com/lvgl/lvgl_pro
- https://docs.squareline.io/docs/miscellanos/changelog
- https://squareline.io/locales/en/pricing.json
- https://github.com/eez-open/studio
- https://docs.mcuxpresso.nxp.com/guiguider/latest
- https://components.espressif.com/components/espressif/esp_lvgl_port
- https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port
- https://github.com/Xinyuan-LilyGO/T-Display-S3
- https://github.com/lvgl/lv_font_conv
