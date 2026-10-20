# Stack fact-check: ESP-IDF

Date: 2026-10-20

Method: multi-agent research workflow: an adversarial fact-checker re-checked claims from the earlier single-agent reports against primary sources and tried to refute each one

Status: verified: 4 confirmed / 3 partially / 0 refuted

Notes: the v6.1 commit quoted under A1 is right: `git ls-remote` on 2026-10-20 gives `refs/tags/v6.1^{}` = `fff9895c82d744c7237be8847347bdd1b07c6643`. The `4dc1c655...` value in lilygo-upstream-tracking.md is the annotated tag object.

The claims below came from the early reports, mainly framework-choice.md, with some from ide-tooling.md, lvgl-ecosystem.md and example-repos.md. Verdicts: **confirmed** (primary evidence found), **partially** (with the precise correction), **refuted**, or **unverifiable**.

## Summary

| Claim | Topic | Verdict |
|---|---|---|
| A1 | ESP-IDF v6.1 release date and support window | confirmed |
| A2 | esp_lvgl_port 2.9.0 with IDF 6.1 and LVGL 9.6 | partially |
| A3 | i80 example: 2 MHz pixel clock with PSRAM buffers on S3 | confirmed |
| A4 | esp-idf-monitor 1.10 non-interactive (command-stream) mode | partially |
| A5 | `idf.py mcp-server` in IDF 6.x | confirmed |
| A6 | IDF linux target and QEMU on macOS | partially |
| A7 | PlatformIO platform-espressif32 v8.0.0 | confirmed |

## A1. ESP-IDF v6.1 release date and support window

**Claim:** ESP-IDF v6.1.0 was released in late August 2026 (sources disagree: 25 vs 27 Aug). As of 2026-10-20 there is no v6.1.x bugfix release; v6.0.3 (2 Sep 2026) is the latest 6.0 patch; v5.5.6 was released 16 Oct 2026. v6.1 service ends Aug 2027, EOL Feb 2029.

**Verdict:** confirmed

**Correction:** No factual error. Minor points: refer to it by the tag name v6.1 (EIM uses `eim install -i v6.1`). release/v6.1 is already 829 commits ahead of the v6.1 tag (GitHub branch head dated 2026-09-22), so a v6.1.1 is pending but not yet released.

**Evidence:** GitHub API, espressif/esp-idf: the tag is named "v6.1", not "v6.1.0". It is an annotated tag with tagger date 2026-08-25T03:21:22Z (commit fff9895c). The GitHub release "ESP-IDF Release v6.1" has published_at 2026-08-27T01:56:12Z. That explains the disagreement: 25 Aug is the tag date and 27 Aug is the GitHub publication date. Espressif's own support data (https://dl.espressif.com/dl/esp-idf/idf_versions.js) uses start_date 2026-08-25, end_service 2027-08-25 and end_date (EOL) 2029-02-25. SUPPORT_POLICY.md gives 12 months of service plus 18 months of maintenance, 30 months in total. The tag list has v6.1-dev, v6.1-beta1, v6.1-rc1 and v6.1, but no v6.1.x. v6.0.3 was published 2026-09-02T11:55Z (tagged 2026-08-31). v5.5.6 was published 2026-10-16T14:32Z (tagged 2026-10-15). The docs.espressif.com 'stable' alias now points to v6.1.

## A2. esp_lvgl_port 2.9.0 with IDF 6.1 and LVGL 9.6

**Claim:** espressif/esp_lvgl_port 2.9.0 (7 Aug 2026) works with ESP-IDF 6.1 and lvgl 9.6.0 (its manifest accepts lvgl >=8,<10; esp-bsp CI builds against LVGL 9.6 per PR #832 of 5 Oct 2026). Check whether esp-bsp CI covers IDF 6.1, and list open esp_lvgl_port issues involving IDF 6.1, LVGL 9.6 or i80 panels.

**Verdict:** partially

**Correction:** "Works with IDF 6.1 + LVGL 9.6.0" is only build-verified. esp-bsp CI compiles against the release/v6.1 branch head (not the v6.1 tag) with lvgl 9.6.0~1. No on-target test runs on IDF 6.1: hardware tests run only on release-v5.5, and nightly has been skipping them since 29 Aug. Nothing in CI covers i80. PR #832 only fixed BSP example warnings. The manifest accepts any future LVGL 9.x automatically.

**Evidence:** **esp_lvgl_port release:** registry API (components.espressif.com/api/components/espressif/esp_lvgl_port) shows 2.9.0 created 2026-08-07T13:53Z with deps idf >=5.2 and lvgl/lvgl >=8,<10 (public). No newer version exists. The last esp_lvgl_port commit is 2026-08-03 (CherryUSB). lvgl/lvgl 9.6.0 reached the registry on 2026-09-16 and 9.6.0~1 on 2026-09-17 (GitHub v9.6.0: 2026-09-16), so 2.9.0 predates LVGL 9.6 by about 6 weeks and contains no 9.6-specific changes.

**esp-bsp CI coverage:** in esp-bsp/.github/workflows/build-run-applications.yml the build matrix is latest, release-v6.1, release-v6.0 and release-v5.5 down to v5.2. release-v6.1 was added by commit e22f6d9 on 2026-07-03. The on-hardware `run-target` matrix is **only release-v5.5**.
- Nightly run 39349066813 (2026-10-20): all 10 release-v6.1 build jobs passed. The logs resolve esp_lvgl_port (2.9.0) + lvgl/lvgl (9.6.0~1), and components/esp_lvgl_port/test_apps/lvgl_port built for esp32s3 ("build success").
- Only the `latest` (IDF master, 6.2-dev) jobs fail, because of esp_codec_dev SOC_ADC_CHANNEL_NUM. As a result 'Run apps' is skipped, and the nightly workflow has failed every night since 2026-08-29.
- From about 17 Sep to 5 Oct, nightly builds also failed on every IDF version, release-v6.1 included. The cause was LVGL 9.6 deprecation warnings (lv_list, lv_win) in BSP examples.

**PR #832** (created 2026-10-02, merged 2026-10-05) fixed those warnings in examples/display_audio_photo and examples/display_sdcard only. It did not change esp_lvgl_port.

**i80 coverage:** no BSP in esp-bsp drives an i80 panel through esp_lvgl_port. Only the nt35510 and st7796 driver test apps use i80, and esp_lvgl_port's own test app uses SPI ILI9341.

**Open esp_lvgl_port issues:** none explicitly concern IDF 6.1, LVGL 9.6 or i80. Related open items:
- #820: the DSI on_frame_buf_complete guard fails to compile on IDF 5.5.0–5.5.4 and 6.0.x. It compiles on 6.1 and is ESP32-P4 only.
- #811: LVGL9 direct_mode/full_refresh without avoid_tearing asserts on RGB/DSI panels.
- #803: high memory use with LVGL built-in malloc.
- #776: a white screen is painted before the app's first frame (no splash screen possible).
- #726: trans_size is ignored in the LVGL9 path. Verified: 0 references in src/lvgl9, 10 in src/lvgl8. Result: PSRAM buffers on SPI panels fail.
- #716: flush_ready is signalled after the first chunk when trans_size chunking is used.
- #548: direct_mode on ST7789 corrupts the display.
- #836: touch read aborts (fix in PR #838).
- PR #609: swap-bytes handling in direct mode under LVGL9.

## A3. i80 example: 2 MHz pixel clock with PSRAM buffers on S3

**Claim:** In ESP-IDF v6.1 examples/peripherals/lcd/i80_controller, the pixel clock is lowered to 2 MHz when colour buffers live in PSRAM on ESP32-S3, so LVGL draw buffers should be in internal DMA-capable RAM. Also report the example's default pixel clock and buffer placement, and what the esp_lcd docs say about PSRAM buffers + i80 on S3.

**Verdict:** confirmed

**Correction:** No error. One nuance: Espressif's own S3 default for this example is the slow path (PSRAM buffers at 2 MHz), and the esp_lcd docs give no PSRAM guidance. The internal-RAM recommendation comes from the example README, esp_lvgl_port's performance doc and issue #12342. The T-Display S3 frame is small: 170×320×2 = 108,800 B, so internal DMA buffers are easily affordable.

**Evidence:** **Pixel clock:** v6.1 examples/peripherals/lcd/i80_controller/main/Kconfig.projbuild sets EXAMPLE_LCD_PIXEL_CLOCK_HZ to `default 2000000 if EXAMPLE_LCD_I80_COLOR_IN_PSRAM && IDF_TARGET_ESP32S3`, otherwise `default 10000000`.

**Buffer placement:** EXAMPLE_LCD_I80_COLOR_IN_PSRAM depends on SOC_PSRAM_DMA_CAPABLE (=1 on S3) and defaults to n. However, sdkconfig.defaults.esp32s3 sets CONFIG_SPIRAM=y, SPIRAM_SPEED_80M, SPIRAM_XIP_FROM_PSRAM=y **and** CONFIG_EXAMPLE_LCD_I80_COLOR_IN_PSRAM=y. So an esp32s3 build of the example defaults to PSRAM draw buffers at 2 MHz; other targets default to internal RAM at 10 MHz.
- Two partial buffers of 240 px × 100 lines in RGB565 (48,000 B each), allocated with esp_lcd_i80_alloc_draw_buffer(io, size, caps): caps 0 gives internal DMA-capable RAM, caps MALLOC_CAP_SPIRAM gives PSRAM.
- LV_DISPLAY_RENDER_MODE_PARTIAL, max_transfer_bytes = 240*100*2, dma_burst_size = 64, trans_queue_depth = 10.
- The example drives raw lvgl/lvgl pinned at "9.2.2", not esp_lvgl_port.
- README troubleshooting: an unstable UI with PSRAM colour buffers is caused by "limited PSRAM bandwidth". Fixes it lists: lower PCLK, raise SPIRAM_SPEED, lower FPS, or enable SPIRAM_XIP_FROM_PSRAM.

**What the esp_lcd docs say:** v6.1 docs/en/api-reference/peripherals/lcd/i80_lcd.rst has no PSRAM-specific text. It only says a higher pclk_hz "may cause flickering if the DMA bandwidth is not sufficient". esp_lcd_io_i80.h notes that esp_lcd_i80_alloc_draw_buffer handles DMA-burst and cache-line alignment.

**Corroboration:**
- esp_lvgl_port docs/performance.md (ESP32-S3, 16-bit i80, 800x480): internal RAM + DMA runs at 20 MHz. PSRAM needs the LCD clock at 2 MHz, and 10 MHz is described as "not working".
- espressif/esp-idf#12342 (still open): S3, 8-bit i80, octal PSRAM buffers at 10 MHz produce corruption under Wi-Fi traffic. An Espressif engineer attributes it to a MEM_SPI bandwidth hardware limitation.

## A4. esp-idf-monitor 1.10 non-interactive (command-stream) mode

**Claim:** esp-idf-monitor >= 1.10.0 (2026-08-27) has a Command-Stream Mode enabled automatically when stdin is not a TTY, with commands like reset, sleep, expect --timeout, exit; ESP-IDF 6.x requires esp-idf-monitor >= 1.10. Verify the exact command syntax and whether `idf.py monitor` passes stdin through.

**Verdict:** partially

**Correction:** 1.10.0 was publicly released on 2026-09-03; 2026-08-27 is only the changelog date. In esp-idf-monitor and in v6.1 the feature is called the non-interactive (scripting) mode, and v6.1 does not document it. Piping into `idf.py monitor` works on v6.1 only by stdin inheritance, with SIGINT ignored and exit codes collapsed to 2. To keep exit codes 110/2 and normal signal handling, run `python -m esp_idf_monitor` directly; otherwise always end scripts with `expect --timeout ...` and `exit`, or stop the session with SIGTERM/timeout(1). The 'IDF 6.x requires >=1.10' part is confirmed.

**Evidence:** **Release date:** the esp-idf-monitor v1.10.0 release notes are headed "v1.10.0 (2026-08-27)" (tag/changelog date). The GitHub release and the PyPI upload are both dated 2026-09-03 (PyPI: 12:24 UTC). 1.10.0 is the latest version.

**Mode selection:** esp_idf_monitor/idf_monitor.py sets `non_interactive = not sys.stdin.isatty()` and then uses CommandReader. The README section is titled "Non-Interactive (Scripting) Mode". The name "Command-Stream Mode" appears only in ESP-IDF **master** docs (commit 93576da216, 2026-09-22); the v6.1 idf-monitor docs do not document the mode at all.

**Exact syntax (base/command_reader.py):**
- One command per line; commands are case-insensitive; empty lines and lines starting with '#' are ignored; each command is echoed to stderr.
- Available commands: reset, flash, flash-all, app-flash, `send <text>`, `sleep <seconds>` (float or inf), `expect <regex>` (re.search with a 1024-line look-back buffer), `expect --timeout <seconds> <regex>` (the flag must come immediately after `expect`), output, log, timestamps, bootloader, exit.
- An expect timeout aborts the rest of the script with exit code 110; an invalid script exits with 2.
- EOF after at least one command ends the session. Empty stdin (/dev/null) gives watch-only mode until SIGINT or SIGTERM.

**Version requirement:** the constraint files fetched by install.sh/EIM (https://dl.espressif.com/dl/esp-idf/espidf.constraints.v6.0.txt and .v6.1.txt) both contain `esp-idf-monitor>=1.10.0,<2`. v5.5 requires >=1.6.2.

**stdin pass-through via idf.py in v6.1:**
- RunTool calls asyncio.create_subprocess_exec with stdout/stderr=PIPE and no stdin argument, so stdin is inherited. tools/idf_monitor.py then calls subprocess.run, also inheriting stdin. So `printf ... | idf.py monitor` does reach the monitor.
- However, v6.1 serial_ext.py calls `signal.signal(SIGINT, SIG_IGN)` before spawning the monitor. That disposition is inherited, so Ctrl+C cannot stop a watch-only or command-stream session; master fixes this and documents why.
- Any non-zero monitor exit (110 or 2) becomes FatalError, i.e. idf.py exits with 2.
- `idf.py monitor --command-file` exists only on master, not on v6.1 or release/v6.1.

## A5. `idf.py mcp-server` in IDF 6.x

**Claim:** `idf.py mcp-server` exists for ESP-IDF >= 6.0 (installed with an --enable-mcp option), offering build/flash/set-target/clean/create-project tools; a monitor tool is merged on master/release-v6.0 but not in a v6.1 release.

**Verdict:** confirmed

**Correction:** Accurate. Omission: the v6.1 tag's mcp-server has bugs that are fixed only after the tag on release/v6.1:
- Tool subprocesses inherit the JSON-RPC stdin and can hang on tools/call (a6c3f023, issue #18961). In v6.1 these are `subprocess.run(..., capture_output=True)` calls with no stdin=DEVNULL.
- "MCP Server running on stdio..." is printed to stdout, i.e. onto the JSON-RPC channel (cf09ccd7).
- The `mcp` SDK 2.x FastMCP→MCPServer rename makes v6.1 report "MCP dependencies not available" (6d8d63da, issue #18904). The constraint files work around this by pinning `mcp<2`.

The monitor tool is likely to ship first in a v6.0.x bugfix release, not in v6.1.x.

**Evidence:** **Availability:** tools/idf_py_actions/mcp_ext.py exists from v6.0 onward and is absent in v5.5.6. It registers the `mcp-server` action using FastMCP.

**Installation:** tools/requirements/requirements.mcp.txt (`mcp[cli]`) says the feature "can be enabled by running install.{sh,bat,ps1,fish} --enable-mcp", and install_util.py maps --enable-* flags to features. The v6.1 docs (api-guides/tools/idf-py.rst) instead recommend installing the `mcp` feature through EIM and running `eim run "idf.py mcp-server"`. They also give `claude mcp add --transport stdio esp-idf -- idf.py mcp-server`.

**Tools in v6.1:** build_project, set_target, flash_project, create_project, clean_project, all with an optional project_dir. Resources: project://config, project://status, project://devices. v6.0 through v6.0.2 had no create_project; it arrived in v6.0.3 and v6.1.

**monitor_device tool:**
- master: commit 3f005c8a, 2026-09-11.
- release/v6.0: commit 93b1e202, merged 2026-10-12 (merge 5806593a).
- Absent from the release/v6.1 branch and from every tag.

## A6. IDF linux target and QEMU on macOS

**Claim:** The ESP-IDF linux target ("host apps") is experimental but works on macOS; ESP-IDF QEMU can emulate ESP32-S3 on arm64 macOS with a virtual framebuffer component (espressif/esp_lcd_qemu_rgb).

**Verdict:** partially

**Correction:** Emulating the ESP32-S3 in QEMU on arm64 macOS, including a virtual framebuffer, is supported. Using esp_lcd_qemu_rgb on S3 is plausible but untested by Espressif, and it needs a separate QEMU build configuration: UART console, RGB virtual panel instead of i80, and optionally the octal-PSRAM flag via --qemu-extra-args. The linux-target "works on macOS" claim is officially stated but contradicted for some configurations by open issue #19124. The linux target also cannot host esp_lcd or esp_lvgl_port, so a UI simulator has to be a separate LVGL host build.

**Evidence:** **Linux target:**
- v6.1 api-guides/host-apps.rst calls running on host "an experimental feature", linux is a "preview target" (`idf.py --preview set-target linux`), and the FreeRTOS POSIX/Linux simulator "currently also works on macOS". The sources do contain Darwin-specific handling.
- But espressif/esp-idf#19124 (open, 2026-09-23, "ESP-IDF linux target doesn't build on Apple/Darwin", v5.5.5 and later) reports several breakages. The lwip linux port adds GNU-ld `-Wl,--wrap=` flags (still present in v6.1 components/lwip/CMakeLists.txt), CoreFoundation has to be linked, and an SDL window cannot run because Cocoa needs the main thread.
- On the linux target, esp_lcd returns early ("not supported by the POSIX/Linux simulator") and esp_timer is headers-only (mock). So esp_lcd and esp_lvgl_port cannot run there.

**QEMU on arm64 macOS:**
- v6.1 tools/tools.json lists qemu-xtensa esp_develop_9.2.2_20260417 with macos-arm64 builds and supported_targets [esp32, esp32s3]. qemu_ext.py defines esp32s3 as `-M esp32s3 -m 32M`, and docs conf QEMU_TARGETS = esp32, esp32c3, esp32s3. macOS needs `brew install libgcrypt glib pixman sdl2 libslirp`.
- The S3 machine in espressif/qemu (hw/xtensa/esp32s3.c) implements a virtual RGB framebuffer (VRAM 0x20000000, registers 0x21000000). These match the addresses hard-coded in esp_lcd_qemu_rgb. The v6.1 qemu.rst points to espressif/esp_lcd_qemu_rgb together with `idf.py qemu --graphics`, which uses `-display sdl` (a window, not headless).
- esp_lcd_qemu_rgb's latest release is 1.0.2 (2024-02-18; source otherwise only reformatted since). Its example build rule enables only esp32 and esp32c3 ("QEMU currently only supports ESP32 and ESP32-C3"), so Espressif does not CI-test it on S3.
- QEMU S3 has no LCD_CAM model, so the i80 ST7789 path cannot run. Its USB-Serial-JTAG device is a stub (reads 0, writes dropped). Octal PSRAM needs `-global driver=ssi_psram,property=is_octal,value=true`.

## A7. PlatformIO platform-espressif32 v8.0.0

**Claim:** The official PlatformIO platform-espressif32 v8.0.0 was released 2026-10-19 with Arduino-ESP32 3.3.12 (IDF 5.5.5) and ESP-IDF 6.1 for framework=espidf; PlatformIO skipped several IDF patch releases (5.5.1, 5.5.2, 6.0.2, 6.0.3).

**Verdict:** confirmed

**Correction:** Accurate, but the context is incomplete:
- IDF 6.1 support first shipped in v7.1.0 (2026-08-31); it is not new in 8.0.0.
- v8.0.0 is the first official release that bundles Arduino-ESP32 3.x; 6.x and 7.x bundled 2.0.17.
- IDF 5.5.4, 5.5.5 and 5.5.6 were never offered for framework=espidf.
- In v8.0.0 the Arduino framework (IDF 5.5.5) and the espidf framework (6.1.0) are built on different IDF bases.

**Evidence:** **v8.0.0:** platformio/platform-espressif32 release v8.0.0 was published 2026-10-19T10:23Z. Its notes list Arduino v3.3.12 ("based on IDF v5.5.5") and ESP-IDF v6.1. platform.json at v8.0.0 has framework-espidf ~4.60100.0 (6.1.0) and framework-arduinoespressif32 ~4.30312.0. The arduino-esp32 3.3.12 release notes (2026-09-18) say "IDF v5.5.5".

**IDF versions per platform release:**
- v6.12.0 (2025-07-31): IDF 5.5.0
- v6.13.0 (2026-02-26): IDF 5.5.3, so 5.5.1 and 5.5.2 were skipped
- v7.0.0 (2026-04-30): IDF 6.0.0
- v7.0.1 (2026-05-12): IDF 6.0.1
- v7.1.0 (2026-08-31): IDF 6.1.0, so 6.0.2 and 6.0.3 were skipped
- v7.1.1–v7.1.3: tooling fixes only

## Fact-checker's notes

Related facts the decision needs that the claims get wrong or leave out (all checked against primary sources unless marked as inference):

1. **Support windows** (dl.espressif.com idf_versions.js):
   - v5.5 left its service period on 2026-07-21 and now gets maintenance fixes only.
   - v6.0 is in service until 2027-03-20, with EOL 2028-09-20.
   - v6.1 has no bugfix release yet. release/v6.1 is 829 commits ahead of the tag.

2. **IDF 6.1 tool changes** (migration guide 6.1/tools): `idf.py flash` now fast-reflashes by default and verifies afterwards; `-a` forces a full flash. v6.1 also supports CMake presets (`idf.py --preset`), which is useful for separate hardware and QEMU configurations.

3. **IDF 6.0 LCD API changes:**
   - psram_trans_align and sram_trans_align were replaced by dma_burst_size.
   - color_space and rgb_endian were replaced by rgb_ele_order.
   - esp_lcd_panel_disp_off was removed.
   - GPIO numbers are now gpio_num_t.

   As a result, vendor T-Display-S3 sample code written for IDF 4.x/5.x will not compile on 6.x without porting.

4. **LVGL 9.6:**
   - lv_list and lv_win are deprecated (compiler warnings; LVGL suggests flex columns instead).
   - Open regression lvgl/lvgl#10818: image inner-align STRETCH/COVER no longer fills the image.
   - The test module moved to src/debugging/test. Relevant options: LV_USE_TEST and LV_USE_TEST_SCREENSHOT_COMPARE (requires LV_USE_LODEPNG); LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE defaults to 1. This supports the headless screenshot plan.
   - Version pinning: the registry lvgl 9.6.0~1 manifest has conditional deps (freetype, libpng, libjpeg-turbo, lz4) that apply only on IDF >=5.5 when the matching LV_USE_* option is enabled. Pin lvgl in the project manifest and commit dependencies.lock, because esp_lvgl_port accepts any LVGL from 8 up to (not including) 10.

5. **esp_lvgl_port specifics:**
   - Its LVGL9 path ignores trans_size (#726), so there is no PSRAM-to-SRAM staging. Use buff_dma=true with internal RAM.
   - #776: lvgl_port_add_disp unlocks and LVGL paints a white frame before the app draws. This matters on every deep-sleep wake.

6. **Deep sleep and USB console:** the v6.1 usb-serial-jtag-console doc says the USB Serial/JTAG device disappears from the host during Deep-sleep and reconnects on wake. Monitor and MCP sessions therefore lose the port on every sleep cycle.

7. **Claude Code Bash stdin (observed in this session):** stdin is not a TTY (isatty False, character device). So an agent-launched `idf.py monitor` always runs in non-interactive mode. With empty stdin it goes watch-only, and on v6.1 it ignores SIGINT. Always pipe a script that ends with `exit` or an `expect --timeout`.

8. **esp-idf-monitor open issue #45:** it spawns one addr2line process per address-bearing line, causing multi-second stalls. These can trip expect timeouts. `--disable-address-decoding` exists on `python -m esp_idf_monitor`; v6.1 `idf.py monitor` has no such flag.

9. **Framebuffer inspection over GDB:** the v6.1 LCD docs mention the idf-drivers-gdb package (`framebuffer_display` over GDB). Inference: with an i80 panel only LVGL's draw buffers are in MCU RAM, since the ST7789 holds its own GRAM. A full-frame capture on hardware would therefore need a full-size internal buffer (108,800 B).

10. **QEMU console:** QEMU S3 USB-Serial-JTAG is a stub, so QEMU builds need a UART0 console.

11. **i80 test coverage:** Espressif's IDF CI runs only a boot-log pytest (generic runner) for the i80 example on SOC_LCD_I80_SUPPORTED targets. It does not check what appears on screen.
