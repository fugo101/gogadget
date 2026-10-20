# Stack review: architect's critique of the research digest

Date: 2026-10-20

Method: multi-agent research workflow: a senior-embedded-architect reviewer, allowed to check primary sources, reviewed a digest of the five early reports before the stack was locked

Status: review notes; not separately verified (several points were later confirmed by the LilyGO deep dive)

Notes: where the settled decisions (docs/adr/) differ, they win: a 250 mAh cell, a >= 1 week target and default backlight 6/16; USB on the left by default, and the display stays on while on USB; no webcam or current meter (power is checked from firmware logs, the palette once from a photo); a `core/` part holds the state as a reducer that ui/ renders, with no LVGL input devices or groups; the press that wakes the device is consumed, not delivered; backlight off before SLPIN; GPIO38 needs no hold (AW9364 internal pull-down); ext1 wake uses the external pull-ups with RTC_PERIPH powered down; the RTC clock source is picked after a 24 h drift measurement; local time comes from the fixed UTC+7 offset in core/, not from TZ strings; log level WARN; a LittleFS `storage` partition takes the rest of the flash; golden-image updates are reviewed by a human; and the console commands follow the decisions' list (for example wifi.add).

Superseded by decision: time syncs run on screen-off timer wakes about four times a day, not on a button wake after 12 h plus an optional nightly wake (ADR-0011); the epoch is also saved to NVS every minute while on USB, so the time survives a reset that clears RTC memory (the Time ADR proposal keeps it in RTC memory only); AW9364 pulses are 1-2 µs, not 2-5 µs, and EN stays HIGH for at least 30 µs at start (ADR-0007); CI builds only in espressif/idf:v6.1, with no 6.0.3 fallback job. The 'ST7789V datasheet v1.3 §7.4.1' cited under Contradictions is most likely the v1.4 copy in LilyGO's repository, where §7.4.1 (p.41) gives TWC ≥ 66 ns; no v1.3 copy was hashed ([datasheets.md](upstream/datasheets.md)).

Input: a digest of five early reports: the analysis of zen-clock (the previous project) in previous-project-analysis.md, plus example-repos.md, framework-choice.md, lvgl-ecosystem.md and ide-tooling.md.

## Contradictions between the reports

- Pixel clock, 17 vs 6.528 vs 2 MHz: each number is tied to a specific setup, and only 17 MHz is plainly wrong. The ST7789V datasheet v1.3 §7.4.1 gives TWC (write cycle) of at least 66 ns, so the 8080 bus tops out near 15.15 MHz. 17 MHz (58.8 ns) is out of spec, and so is LilyGO's Arduino factory sketch at 16 MHz. The 17 MHz came from hiruna's README, which wrongly cites a '17 MHz ST7789 datasheet maximum', and was copied into zen-clock bsp_display.c. 6.528 MHz is LilyGO's Arduino lv_demos value: exactly 60 fps × 320×170×2 B on an 8-bit bus. LilyGO's own ESP-IDF repo uses 10 MHz. 2 MHz is the IDF i80 example's Kconfig default only when EXAMPLE_LCD_I80_COLOR_IN_PSRAM is set on S3; otherwise the default is 10 MHz. For the new design, with draw buffers in internal RAM: use 10 MHz by default with a hard ceiling of 15 MHz. 2 MHz matters only if DMA reads from PSRAM.
- 'hiruna/esp-idf-t-display-s3 is the best reference, with AW9364 16-step dimming' is misleading. hiruna/esp-idf-aw9364 aw9364.c maps 16 steps onto LEDC duty plus ledc fades. That is PWM, not the AW9364 EN pulse-count protocol. zen-clock's lcd_backlight is this same code (SPDX: Hiruna 2025). The same template also sets 17 MHz, PSRAM double buffers, 1/10 buffers and a 4 KB LVGL task stack, and pulls an unpinned dependency (git, version: main). It is therefore the source of several of the previous project's defects; use it for pin maps only. Its main branch is tested on IDF 5.5.x + LVGL 9.2.x, not 'IDF >=5.3' (5.3 + LVGL 8.3 is an old branch); the last commit is 2025-03-01. The correct dimming reference is setBrightness() in LilyGO's factory.ino.
- The starter report says the official repo is Arduino-only, but the LilyGO README points to Xinyuan-LilyGO/LilyGo-Display-IDF, an official ESP-IDF example repo. Its T-Display-S3 path uses a 10 MHz pixel clock, always sends the LCD_MODULE_CMD_1 table, drives the backlight as a plain GPIO HIGH, and sets USB-Serial-JTAG as the primary console. It is LVGL 8; last push 2025-11-03. The digest omits it.
- 'GPIO15 = LCD/peripheral power' is wrong for the header pins. In issue #352 (2026-10-13) the LilyGO maintainer confirmed that the header 3V pins sit directly on the LDO output and are NOT switched by GPIO15; the README was in error. GPIO15 switches only the display/backlight supply, so external loads keep drawing current in deep sleep.
- 'Two panel revisions, with an extra init table for the newer one': LilyGO's current factory and lv_demos sketches define LCD_MODULE_CMD_1 by default, and LilyGo-Display-IDF sends the table unconditionally. Treat it as always sent through esp_lcd_panel_io_tx_param, with no revision detection. The table contains its own SLPOUT + 120 ms, on top of esp_lcd's SLPOUT + 100 ms.
- The IDF i80 example is not the combination the digest implies. At the v6.1 tag it pins lvgl/lvgl 9.2.2 (plus esp_lcd_nt35510 and littlefs); only the release/v6.1 branch head pins 9.6.0. It swaps bytes in software (lv_draw_sw_rgb565_swap in the flush callback) rather than with the i80 flag, and uses an 8 KB LVGL task stack. The LVGL report's choice still works: io_config.flags.swap_color_bytes exists in IDF 6.1 and is implemented by the LCD_CAM swizzle. But it must never be combined with esp_lvgl_port swap_bytes=true, which would swap twice and produce wrong colors.
- Release dates and the adoption story:
  - IDF v6.1: tag created 2026-08-25, GitHub release published 2026-08-27 (rc1 on 08-18), so either date can be quoted.
  - v6.0.3 (2026-09-02) and v5.5.6 (2026-10-16) are confirmed. EOL around Feb 2029 matches the 30-month SUPPORT_POLICY.
  - IDF v6.0 GA was 2026-03-20 and 6.0.1 was 2026-04-27. zen-clock's first commit is 2026-05-09, on PlatformIO platform 7.0.1 (IDF 6.0.1, published 2026-05-12).
  - So it did not adopt IDF 6.0 'the week it shipped'; it adopted PlatformIO's packaging of 6.0.1 that week. The instability came mainly from the Tailscale/WireGuard forks and the PlatformIO glue, not from IDF GA age.
- 'PlatformIO v8.0.0 (2026-10-19): Arduino 3.3.12 + IDF 6.1' mislabels the release. It is platform-espressif32 v8.0.0, which added Arduino core 3.3.12. IDF 6.1.0 support arrived earlier, in v7.1.0 (2026-08-31). The 7.x line went from IDF 6.0.1 straight to 6.1.0 and never shipped 6.0.2 or 6.0.3, which confirms that it skips IDF patch releases.
- 'IDF 6.1's console defaults to UART0 with USB as secondary' is not new in 6.1. zen-clock's IDF 6.0.1 sdkconfig already has CONFIG_ESP_CONSOLE_UART_DEFAULT plus SECONDARY_USB_SERIAL_JTAG. The secondary console is output-only: the IDF docs say input or REPL needs USB-Serial-JTAG selected as primary. UART0 (GPIO43/44) is not wired to the board's USB-C, so the old firmware had no console input at all.
- Button 'quirks':
  - Issue #104 is in zen-clock's own tracker, not Espressif's.
  - The 100 Hz rate comes from the project's CONFIG_BUTTON_PERIOD_TIME_MS=10 (component default 5 ms = 200 Hz), and the timer runs only while the chip is awake.
  - The 'deep-sleep conflict' was a feared risk, never a reproduced bug: enable_power_save installs a light-sleep gpio_wakeup on GPIO0/14, the same pads deep_sleep configures for ext1 plus RTC pull-ups.
  - The 'phantom release' is the documented PRESS_UP that follows a long press.
  - The 'inverted level' is iot_button_get_key_level() returning the logical pressed state.
  - The LVGL report is right that esp_lvgl_port's nav helper needs all three handles with button v4. It maps them to LV_KEY_LEFT/RIGHT/ENTER, not PREV/NEXT.
- 'The same UI sources build for firmware and simulator' does not hold out of the box. lv_port_pc_vscode release/v9.6 pins its LVGL submodule at 8df3de9, which is v9.6.0 plus 10 commits, not the tag the firmware gets from the registry. Its CMakeLists sets CMAKE_C_STANDARD 99, while the firmware builds with gnu23. Its README says macOS needs Homebrew LLVM for -fsanitize=leak and lists only Linux/WSL as tested; the shipped VS Code debug config uses gdb, which does not run on Apple Silicon.
- 'This Mac has python3' is not enough. /usr/bin/python3 is 3.9.6, below IDF 6.1's minimum of 3.10 (tools/python_version_checker.py). Homebrew python@3.14 (3.14.8) and LLVM 23.1.3 are installed but not on PATH. Homebrew 'sdl2' is an alias of sdl2-compat 2.32.74 running over SDL3 3.4.18. Homebrew cmake is 4.4.4, while IDF 6.1 tools.json pins cmake 4.0.3 and ninja 1.12.1.
- 'Fonts via npx github:lvgl/lv_font_conv' runs unpinned master. The latest npm release is 1.5.3 (2024-06-10), and master still reports version 1.5.3 although it has newer fixes (e.g. 2026-09-17 'move stride guard'), so the version string does not identify the code. Its package.json engines field pins Node 24.11.1; this Mac has Node 26.8.1.
- QEMU as a visual-verification path: Espressif's QEMU for S3 provides a virtual RGB framebuffer, but it does not emulate LCD_CAM/i80, USB, LEDC, RMT or the GPIO matrix (esp-toolchain-docs table). It cannot run the real display, backlight, input or sleep code, so it only duplicates the LVGL simulator.
- Diagnostics are internally inconsistent: the IDE report relies on 'idf.py coredump-info', but the old sdkconfig had CONFIG_ESP_COREDUMP_ENABLE_TO_NONE=y and partitions.csv had no coredump partition. The StoreProhibited crashes could never have been analysed post mortem.
- Old rule vs new plan: zen-clock's README and ui README say 'the LVGL buffer must be full screen; smaller buffers smear', while the new plan uses 2 × 320×40 internal buffers. The old observation was made with PSRAM buffers at 17 MHz and a 4 KB LVGL stack, so it does not transfer. It is still an unverified claim that the new pipeline must disprove with a test, not simply ignore.
- Smaller items, confirmed with a nuance:
  - esp-idf-monitor's 'command-stream mode' exists under the name 'non-interactive command mode' plus 'expect --timeout' (v1.10.0; IDF 6.1 constraints require >=1.10.0).
  - 'idf.py mcp-server' exists in 6.1 but exposes only build/set-target/flash/create/clean plus config/status/devices resources, with no monitor or log capture.
  - 'eim run' exists, but EIM is pre-1.0 (v0.20.1).
  - zen-clock's comment that gpio_deep_sleep_hold_en() 'would latch every digital pad including the wake buttons' contradicts IDF gpio.h. That call only affects digital pads that already have gpio_hold_en, and GPIO0/14 are RTC pads.

## Risks of the candidate stack

- USB console vs deep sleep:
  - Deep sleep powers off the USB-Serial-JTAG controller and PHY, so /dev/cu.usbmodem* disappears and re-enumerates on every wake.
  - ROM, bootloader and early-app logs are lost while the host re-enumerates.
  - Light sleep also silences USJ; the host may need a replug, and CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION only blocks automatic light sleep.
  - On battery, with no host, the console stalls once for 50 ms when its buffer fills.
  - For an unattended agent, a sleeping board cannot be flashed or monitored without a human pressing a button.
  - Mitigation: a dev-mode rule 'never deep-sleep while usb_serial_jtag_is_connected()', console commands stay-awake / sleep-now, an RTC-memory ring of boot milestones and last errors, core dump to flash, and host capture via esp-idf-monitor ≥1.10 non-interactive mode with --no-reset and timeouts.
- Boot-to-first-frame (each wake is a full boot), per the IDF 6.1 speed guide:
  - CONFIG_SPIRAM_MEMTEST costs about 1 s per 4 MB, so about 2 s per wake on 8 MB. zen-clock had it set to y.
  - Comprehensive heap poisoning costs about 300 ms per 4 MiB.
  - XIP_FROM_PSRAM / FETCH_INSTRUCTIONS / RODATA copy text and rodata into PSRAM on every boot (all were y in zen-clock).
  - The bootloader validates the image unless BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP is set.
  - LCD bring-up: the esp_lcd ST7789 reset takes 2×10 ms and SLPOUT 100 ms, and the LilyGO CMD_1 table adds another SLPOUT + 120 ms.
  - Expected result: about 2.5–3 s with the old config vs a feasible 0.3–0.5 s, at roughly 40–100 mA while awake. Set a budget, log milestones, and gate regressions.
- Deep-sleep current and board limits:
  - LilyGO issue #289 measured 6.42 mA asleep with GPIO15 and backlight low, and 263 µA once the panel was also put into SLPIN. zen-clock cut GPIO15 without SLPIN and never measured.
  - The header 3V rail is never switched (#352).
  - GPIO38 is a digital pad: holding it low needs gpio_hold_en plus gpio_deep_sleep_hold_en.
  - LCD bus pins float in sleep, and the wake pins need RTC pull-ups with RTC_PERIPH kept on.
  - A ~0.26 mA floor already costs about 6 mAh/day before any wake.
  - Without a current measurement setup, 'battery-first' cannot be verified.
- espressif/button:
  - Its periodic esp_timer runs the whole time the chip is awake, causing 100–200 wakeups/s and blocking automatic light sleep.
  - Its power-save fix reconfigures the same pads used for the ext1 wake.
  - The press that woke the chip is already in progress when the driver starts hundreds of ms later, so the first event can be lost or arrive as a bare release.
  - esp_lvgl_port's nav helper requires three buttons, and the board has two.
- Timekeeping for a clock:
  - The S3's XTAL_32K_P pin is GPIO15, which this board uses as the display power enable, so no 32.768 kHz crystal is possible.
  - Time across deep sleep therefore runs on the internal ~136 kHz RC (zen-clock: RTC_CLK_SRC_INT_RC). IDF docs only endorse it if 'several seconds per day' of drift is acceptable, and it varies with temperature.
  - Options: RTC_CLK_SRC_INT_8MD256 (about +5 µA) and/or periodic NTP.
  - Measure 24 h drift. Never set RTC_CLK_CAL_CYCLES=0 just to save boot time.
- IDF v6.1.0 maturity: there is no bugfix release yet, which repeats the previous project's early-adoption pattern. The 6.x line also renamed the GPIO wake APIs and deprecated esp_sleep_get_wakeup_cause() in favour of the bitmap version, so example code found online drifts. Mitigation: pin the exact tag and the Docker digest, keep 6.0.3 (the same gnu23 / Picolibc / Mbed TLS 4 API surface) building in CI as a fallback, adopt 6.1.1 only after about 2 weeks of age and a green CI/HIL run, and never consume IDF through PlatformIO.
- LVGL 9.6.0 maturity:
  - 9.6.0 (2026-09-16) is the last v9 release, and it rewrote the software blend paths. The release/v9.6 branch already carries 10 fixes and there is no 9.6.1 yet.
  - Deprecated names emit #warning and disappear in v10.
  - The public API moved to include/lvgl and Kconfig was restructured (LV_COLOR_DEPTH is deprecated).
  - esp_lvgl_port 2.9.0 predates 9.6.0, still uses v8 aliases (lv_disp_*), and its S3 SIMD blend hooks target the pre-9.6 blend API.
  - Mitigation: pin 9.6.0 exactly via dependencies.lock, keep LV_USE_DRAW_SW_ASM off, apply -Werror only to own code, and watch for 9.6.x.
- Limits of esp_lvgl_port's LVGL 9 path:
  - swap_bytes swaps the buffer in place.
  - direct_mode / full_refresh is broken on SPI/i80: esp-bsp #609 (open) swaps the wrong span and passes an unoffset full-frame pointer; #548 reports corruption.
  - trans_size is ignored under LVGL 9 (#726).
  - Rotation never re-applies esp_lcd_panel_set_gap.
  - A 5 ms periodic tick esp_timer runs for as long as the port is started.
  - Mitigation: use partial mode, fixed orientation, the hardware swap and a custom keypad indev only. Keep a thin display-port seam so the ~150-line glue from the IDF example (tested with 9.6.0 on release/v6.1) can replace the port.
- Picolibc, the default libc since IDF 6.0:
  - stdin/stdout/stderr are global; per-task redirection is gone.
  - In Newlib-compat mode, libraries built against Newlib headers that touch struct reent can corrupt task stacks.
  - sys/signal.h is removed.
  - printf, strftime and TZ behaviour differ from macOS libc, so text rendered in the simulator can differ from the device. Olson TZ names work on macOS but not on the device.
  - Mitigation: keep formatting in pure, host-tested helpers, use POSIX TZ strings, and compare device vs sim strings in HIL.
- Mbed TLS 4 (6.1 ships 4.1) with PSA as the crypto API:
  - Most legacy mbedtls_* crypto primitives are removed. This is what forced the WireGuard/Tailscale fork patches.
  - psa_crypto_init() must run before any crypto.
  - TLS 1.2 finite-field DHE, RSA key exchange and static ECDH are gone; secp192 is removed; flash footprint grows.
  - Any OTA/HTTPS/MQTT endpoint must offer ECDHE or TLS 1.3. Reject third-party crypto components that are not already PSA-native.
- C23 code shared with the macOS clang simulator:
  - Firmware builds with -std=gnu23 on GCC 15.2 (IDF 6.1 build.cmake).
  - Apple clang 21 on this Mac compiles constexpr/auto/typeof/nullptr/[[attr]], but <stdbit.h> is missing (tested).
  - Plain char is unsigned on Xtensa (GCC xtensa.h DEFAULT_SIGNED_CHAR 0) but signed on arm64 macOS.
  - long, size_t and pointers are 32-bit vs 64-bit, and int32_t is long on xtensa-esp, so format warnings differ.
  - GCC 15 adds warnings clang lacks (-Wunterminated-string-initialization, -Wheader-guard) and vice versa, and the LVGL simulator template forces C99.
  - Mitigation: a banned-feature list, -funsigned-char on host builds, CI compiling ui/ with both compilers, -Werror on own code only.
- Simulator/device divergence can make green sim tests meaningless. Causes:
  - the simulator's LVGL commit differs from the firmware's tag;
  - the configs come from two sources (IDF Kconfig vs lv_conf.h);
  - the test display defaults to XRGB8888 while the device renders RGB565;
  - the desktop heap is unlimited while the device has a budget;
  - LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE defaults to y, so a missing golden is silently created and the test passes.
- Agent verification blind spots: sim screenshots and on-device lv_snapshot show what LVGL rendered, not what the glass shows. Bus timing errors, a wrong init table or gamma, inversion, gap/offset, orientation, tearing (the panel has no TE pin) and backlight behaviour are all invisible to them. Without a camera, or a human sign-off at bring-up and after each display-path change, 'verify visuals without a human' covers only the logical UI. The esp_lcd i80 driver cannot read the panel back, so there is no cheap self-test either.
- sdkconfig drift:
  - Symbols renamed or removed in IDF or LVGL upgrades are silently dropped from sdkconfig.defaults.
  - save-defconfig omits options that equal today's defaults, so a future default change (console, libc, LVGL options) changes behaviour with no diff.
  - Component-manager regeneration already reset zen-clock's CONFIG_ML_* and CONFIG_BUTTON_* settings.
  - Mitigation: write critical options explicitly and assert them in CI.
- OTA rollback combined with deep sleep: every wake is a reboot. With BOOTLOADER_APP_ROLLBACK_ENABLE, a freshly updated image that goes to sleep before calling esp_ota_mark_app_valid_cancel_rollback() is marked ABORTED by the bootloader and rolled back.
- Internal-RAM pressure: two 320×40 RGB565 draw buffers take 51.2 KB of DMA-capable RAM. Together with the LVGL task stack, Wi-Fi (about 50–80 KB) and BLE (Bluedroid about 110 KB per zen-clock) they compete for roughly 300–350 KB of usable internal RAM. Allocation failures under Wi-Fi plus BLE are exactly the crash class seen before. Mitigation: prefer NimBLE or no BLE in v1, put the LVGL heap in PSRAM, and log the minimum free internal heap.
- macOS toolchain maturity:
  - EIM is pre-1.0 (idf-im-ui v0.20.1, 2026-10-16) and its CLI may change.
  - IDF issue #19266 (open since 2026-10-18) reports ccache failing with 'bad compiler arguments'.
  - gdb does not run on Apple Silicon; use lldb on the host and IDF's OpenOCD + gdb for the target.
  - Homebrew cmake 4.4 differs from the IDF-pinned 4.0.3.
  - Mitigation: pin the EIM version, document export.sh as a fallback, and treat the Docker CI build as the reference.
- GPIO0 is the BOOT strapping pin. Holding it during power-on, brownout or RTC-WDT reset enters download mode (deep-sleep wake does not re-sample the straps), and the device then looks dead. A long-press gesture on GPIO0 on a sagging battery can cause exactly that. Put long-press and combo actions on GPIO14.

## Likely root causes of the previous project's unexplained problems

- i80 artifacts 'when updating UI from tasks'. Most likely cause: an out-of-spec bus with DMA reading from PSRAM, not the tasks themselves.

  Evidence in zen-clock:
  - PIXEL_CLOCK_HZ = 17 MHz, above the ST7789V's ~15.15 MHz limit (66 ns TWC).
  - Full-screen double buffers in PSRAM (buff_spiram=true), clocked at 8.5× the 2 MHz Espressif uses for PSRAM-sourced buffers.
  - CONFIG_SPIRAM_XIP_FROM_PSRAM / FETCH_INSTRUCTIONS / RODATA put all code and rodata, including fonts, in PSRAM, and SPIRAM_TRY_ALLOCATE_WIFI_LWIP put the Wi-Fi/LWIP buffers there too.
  - NVS/flash writes stall the same shared MSPI bus.
  - LCD_CAM has no flow control, so any GDMA underrun sends garbage pixels to the panel.
  - Updates 'from tasks' coincided with network, BLE and NVS activity, which made the correlation look causal. The early note 'smaller buffers smear' (a 1/10 buffer in PSRAM means more DMA-while-render overlap) fits the same mechanism.
  - Minor contributors: RD (GPIO9) was only weakly pulled up where LilyGO drives it HIGH, and LVGL was called from about 7 tasks.

  New design: internal DMA buffers, 10 MHz, no XIP from PSRAM, hardware byte swap, RD driven high, a single LVGL owner. Then prove it with a 10-minute stress test (rapid partial invalidations + Wi-Fi traffic + NVS writes), checked by camera or a human, before dropping the 'no task updates' rule.
- StoreProhibited with lv_obj_create containers. zen-clock's ui README (commit 70930e2, 2026-05-10) records 'StoreProhibited at address -320 during font rendering'.

  Evidence:
  - EXCVADDR 0xFFFFFEC0 is NULL minus one 320-byte row. That is the signature of LVGL's draw-buffer goto_xy at y = -1 on a NULL buffer (one row of a 160-px RGB565 layer or a 320-px A8 mask): a failed, unchecked allocation or a corrupted layer pointer. Nothing about it is specific to containers.
  - The same commit raised the LVGL task stack from 4 KB (hiruna's value) to 6 KB, still below esp_lvgl_port's 7 KB default and the IDF example's 8 KB.
  - The build had COMPILER_STACK_CHECK_MODE_NONE, canary-only overflow checks, heap poisoning off and core dumps disabled, so the crash could not be root-caused.
  - Containers add draw recursion depth, layers and scrollbars. Banning them only moved the load back under the failure threshold.
  - Cross-task LVGL access at the time is a further candidate.

  New design:
  - LVGL task stack 12 KB with logged high-water mark; FREERTOS_WATCHPOINT_END_OF_STACK and a STRONG stack protector in dev builds.
  - LVGL 9.6 LV_USE_CHECK_ARG + LV_CHECK_ARG_ASSERT_ON_FAIL, LV_USE_CHECK_OBJ_VALIDITY, ASSERT_NULL/MALLOC/OBJ with an abort() handler.
  - LVGL heap in PSRAM; ELF core dump to flash.
  - Simulator under ASan/UBSan with a nested-container stress test.
  - Delete the 'no containers' rule once both pass; flex and grid layouts need containers.
- Backlight dimming via LEDC PWM on the AW9364. Per the datasheet (V2.4), the AW9364 is a pulse-count dimmer, not a PWM one:
  - each EN rising edge after a low of 0.5–500 µs steps the current down one of 16 levels (20 mA to 1.25 mA, wrapping back to full);
  - EN held low for more than 2.5 ms shuts it down;
  - it needs at least 20 µs after enable.

  A 5 kHz LEDC signal turns every 200 µs period into a step, so the current cycles through all 16 levels at about 312 Hz whatever the duty. Brightness barely tracks the setting, fades are meaningless and flicker appears; only 0% (shutdown) and about 100% behave. zen-clock's lcd_backlight is hiruna's LEDC code, and LilyGO removed LEDC backlight code as invalid in commit 02f0e9d (2026-03-09).

  New design: a pulse-count driver modelled on LilyGO factory.ino setBrightness(). Track the current level; emit (16 + to − from) mod 16 pulses with about 2–5 µs low/high inside a short critical section (at most ~150 µs); shut down with at least 3 ms low; wait 20 µs or more after enable. Expose levels 0–16; fades become 16 discrete steps.
- Wake latency and battery drain (never measured in zen-clock). Likely costs:
  - SPIRAM_MEMTEST=y, about 2 s per wake on 8 MB;
  - the XIP-from-PSRAM copy at every boot;
  - LV_DEF_REFR_PERIOD 10 ms, LVGL max sleep 10 ms, a 1 kHz FreeRTOS tick and the 100 Hz button timer while awake;
  - the Bluedroid BLE stack;
  - cutting the LCD rail without SLPIN (LilyGO #289: 6.42 mA vs 263 µA).

  New design: the boot-time and power ADRs, with every number measured.
- Smaller recurring problems:
  - Button quirks were API semantics, and adopting espressif/button replaced a working ISR-driven driver with an always-on timer (zen-clock #46/#104). Write an own ISR + FSM driver instead.
  - Font files were hand-edited after each regeneration; the include line had been changed to 'lvgl.h'. Use lv_font_conv --lv-include (or LV_LVGL_H_INCLUDE_SIMPLE) and exclude generated files from clang-format/tidy.
  - No simulator was possible because ui/ depended on bsp/wifi/microlink. Enforce a 'ui depends only on lvgl' rule through the simulator build.
  - Core-level patching came from Tailscale/WireGuard forks using legacy mbedtls_* APIs that Mbed TLS 4 removed. Accept no third-party crypto that is not PSA-native.

## Gaps to close before scaffolding

- Panel facts for this specific board: which ST7789 revision the v1.2 unit carries, and a one-time check (camera or human) that CMD_1 + invert + gap(0,35) + mirror(false,true) give correct colours, gamma and edges.
- Power numbers and how to measure them:
  - deep-sleep current with the planned sequence (LilyGO #289 brackets it at 263 µA vs 6.42 mA);
  - awake current at 160 vs 240 MHz and at each backlight level;
  - energy per wake;
  - the instrument that will measure it.
- Facts still to take from the schematic:
  - what GPIO15's LDO feeds (panel VCC, AW9364 VIN?);
  - the battery-divider values and its standing drain;
  - whether GPIO0/14 have external pull-ups;
  - whether any VBUS sense exists (hiruna only uses battery ADC > 4350 mV as a USB heuristic, and the ADC is invalid while USB is plugged).
- Energy-budget inputs: battery capacity, wakes per day, awake timeout and default brightness, and from them the target battery life.
- A wake-latency target, plus a measured baseline built from milestone timestamps.
- Partition table: not yet decided, including the coredump partition, the NVS size (Wi-Fi, settings, PSA ITS, wear) and whether an assets filesystem is needed.
- sdkconfig baseline and its dev/release overlays: not yet written, nor the list of options CI should assert.
- Console and dev-mode policy: USJ as primary console, stay awake while a host is connected, log levels per build type, and which REPL commands exist.
- LVGL memory and config: allocator, heap budget, layer-buffer size, cache sizes, the measured LVGL stack, and an internal-RAM budget with Wi-Fi (with and without BLE). Also the mechanism that keeps the simulator and firmware LVGL configs identical (Kconfig fragment vs lv_conf.h).
- Fonts:
  - families and licences (can DS-Digital be redistributed in the repo?);
  - sizes, glyph ranges, bpp and compression;
  - flash estimate (likely under 300 KB, so flash is not the constraint);
  - Vietnamese stacked-diacritic line-height test strings.
- UX spec: screens, navigation map, two-button gesture map (short/long/combo), settings list, themes, and low-battery behaviour.
- Timekeeping: RTC slow-clock source, a measured 24 h drift, NTP cadence, the TZ representation and where the TZ setting is stored.
- Networking scope: provisioning method, BLE yes or no, and the OTA source/URL with a TLS configuration compatible with Mbed TLS 4.
- Bootstrapping this Mac:
  - Python ≥3.10 for IDF: the system has 3.9.6 and Homebrew's 3.14.8 is not on PATH; confirm EIM and IDF 6.1 accept 3.14, otherwise install 3.12 or 3.13.
  - An EIM version pin.
  - IDF's cmake 4.0.3/ninja vs the Homebrew versions.
  - sdl2-compat, LLVM for sanitizers, lldb.
  - Docker for local CI parity.
  - Camera (TCC) permission if a webcam is used.
- CI/HIL design: runner types, caching (watch IDF #19266, the ccache bug), the golden-image update and approval workflow, and whether a HIL board and runner exist.
- Pass criteria for retiring the old folklore rules: a bus stress test, a nested-container stress test and a partial-buffer test. Optionally, a pixel-clock headroom sweep from 6.5 to 15 MHz under Wi-Fi and NVS load.
- Whether PSRAM is needed at all: measure boot time and awake current against the convenience of an LVGL heap and snapshot buffer in PSRAM.
- Reuse inventory from zen-clock: which pure modules, Unity tests and ADRs move over, and which zen-clock rules are now disproven.

## Questions for the owner (with recommended answers)

- What is in v1? Recommended: an offline-capable clock, battery indicator, settings, and Wi-Fi/NTP sync. Wi-Fi credentials are set over the USB console (scriptable by the agent) and stored in NVS. No BLE provisioning, and no Tailscale/WireGuard: that is where the IDF 6 fork patching came from. OTA is deferred, but the partition layout is ready for it. Add NimBLE or SoftAP provisioning later only if you need to set Wi-Fi without a computer.
- Which IDF baseline: v6.1 (2026-08-27; no 6.1.1 yet; supported to about Feb 2029) or the more patched v6.0.3 (supported to about Sep 2028)? Recommended: v6.1, pinned to the exact tag and Docker digest, with 6.0.3 kept building in CI as a fallback for the first months. Move to 6.1.1 once it is about 2 weeks old and CI/HIL is green. Never through PlatformIO.
- What battery and responsiveness targets? Battery capacity, wakes per day, awake timeout, acceptable button-to-first-frame time, default brightness. Suggested starting point: ≤500 ms to first frame, 10–15 s awake, default brightness 8/16, at least 30 days on a 1000 mAh cell at about 50 wakes/day. These will be validated by measurement; the board's sleep floor is reported at about 0.26 mA.
- How much clock drift is acceptable, and may the device turn on Wi-Fi by itself? Recommended: RTC_CLK_SRC_INT_8MD256 (about +5 µA). Run an NTP sync in the background on a button wake when the last sync is more than 12 h old, with an optional nightly timer wake. Show a 'not synced' badge after 24 h.
- How should it behave on USB power? Recommended: stay awake with a dimmed screen and backlight auto-off while a USB host is detected; on a plain charger, follow the battery policy. The other option is a desk-clock mode. Note that this board can only detect USB power through an ADC heuristic.
- Languages and fonts: Vietnamese + English, which font family, and may the DS-Digital TTF be redistributed in your repo? Recommended: Vietnamese + English through lv_translation, with NFC-normalised strings. Use one OFL family with full Vietnamese coverage (e.g. Be Vietnam Pro or Noto Sans) at three sizes and 4 bpp, plus a digits-only display face whose licence allows redistribution.
- Orientation and two-button mapping: keep zen-clock's landscape layout (buttons on the left, USB on the right) and its button mapping? Recommended: keep landscape, fixed at init. GPIO14 short press = next, GPIO14 long press = back/sleep, GPIO0 short press = select. Keep long-press actions off GPIO0, which is a strapping pin.
- Will you mount a cheap USB webcam facing the display and add a current meter, for example a Nordic PPK2 standing in for the battery, and grant the terminal camera permission once? Recommended: yes to both. The camera is the only way the agent can catch bus, init or orientation faults; the PPK2 makes the battery claims measurable from scripts. Without them, plan human sign-offs at bring-up and after each display-path change.
- Do you need flash encryption or secure boot? Recommended: neither for v1. That keeps BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP and easy reflashing. Revisit before the device stores anything more sensitive than a Wi-Fi password.
- Start a fresh repository or evolve zen-clock? Recommended: a fresh repo. Port only the pure modules with their Unity tests (backoff, timezone_fmt, settings_table, battery_view, input_policy), the ADR lessons, and your Conventional Commits / release-please workflow. Leave the BSP, LVGL glue, button and backlight code behind.
- zen-clock was developed in CLion (.idea and .run folders are present). Will you switch day-to-day to VS Code with ESP-IDF extension v2.3.0 and the Claude Code extension? Recommended: VS Code for IDF-native flash, monitor, menuconfig and debugging. CLion can stay as an editor via compile_commands.json, but nothing in the build should depend on either IDE.
- CI and hardware-in-the-loop: GitHub-hosted runners only, or also a self-hosted runner with a board attached? Recommended: start with GitHub-hosted build plus headless UI tests in the pinned Docker image. Add a self-hosted Mac with a spare T-Display-S3 and a webcam once bring-up is stable.

## Decisions the agent should take and record as ADRs

- Toolchain and pinning ADR:
  - ESP-IDF v6.1 installed via EIM locally (EIM version pinned); CI uses espressif/idf:v6.1 by digest.
  - Manifests: idf '>=6.1,<6.2' (CI also builds 6.0.3 while 6.1.0 is young), lvgl/lvgl '9.6.0' exact, espressif/esp_lvgl_port '~2.9.0'.
  - Commit dependencies.lock.
  - Forbid override_path, git-branch dependencies and local patches.
  - Review IDF, LVGL and port updates monthly; adopt a patch release after it has aged about 2 weeks.
- sdkconfig ADR. Policy: curated sdkconfig.defaults + .esp32s3 + dev/release overlays, selected via SDKCONFIG_DEFAULTS; the generated sdkconfig is gitignored; critical options are written explicitly and asserted by a CI script.

  Baseline:
  - flash 16 MB, QIO, 80 MHz;
  - octal PSRAM at 80 MHz with MEMTEST=n, and no XIP_FROM_PSRAM / FETCH_INSTRUCTIONS / RODATA / TRY_ALLOCATE_WIFI_LWIP;
  - console on USB-Serial-JTAG, no secondary console;
  - APP_ROLLBACK and SKIP_VALIDATE_IN_DEEP_SLEEP; bootloader log WARN;
  - app log INFO (dev) or WARN (release), with MAXIMUM_LEVEL_DEBUG;
  - ELF core dump to flash;
  - CPU 160 MHz until measured; FREERTOS_HZ 1000.

  The dev overlay adds the STRONG stack protector, FREERTOS_WATCHPOINT_END_OF_STACK and light heap poisoning.
- Repository layout ADR:
  - firmware/: main, board, display_port, backlight_aw9364, input_buttons, power, timekeeping, net, devconsole.
  - ui/: depends only on lvgl and ui_model.h.
  - sim/: headless tests and an optional SDL viewer.
  - tests/host/: CMake + Unity/CTest, not the experimental IDF linux target.
  - tools/ and docs/adr/.

  The simulator build is what enforces the 'ui depends only on LVGL' rule.
- Display pipeline ADR:
  - esp_lcd i80, 8-bit, with the built-in ST7789 driver.
  - Pixel clock 10 MHz, never above 15 MHz.
  - Two 320×40 RGB565 draw buffers in internal DMA RAM (esp_lcd_i80_alloc_draw_buffer), partial render mode only.
  - Byte swap via io flag swap_color_bytes=1, with the port's swap_bytes=false.
  - Orientation fixed at init: swap_xy, mirror(false,true), gap(0,35), invert on.
  - LilyGO's CMD_1 init table always sent via tx_param.
  - GPIO9 (RD) driven high; GPIO15 high before init; backlight on only after the first frame.
  - LVGL task: 12 KB internal stack, trimmed later to 1.5× the measured high-water mark; priority 4; core 1.
- UI concurrency ADR: after init, the LVGL task is the only code that touches LVGL. Services post immutable messages to a queue (ui_post). The LVGL side drains the queue and updates lv_subjects that widgets bind to. Each post wakes the LVGL task (lvgl_port_task_wake) instead of a 250 ms poll. The simulator drives the same ui_post API from scripted scenarios.
- LVGL config ADR:
  - One config source shared by firmware and sim; LVGL 9.6 generates lv_conf from its Kconfig tree, so there are never two hand-maintained copies.
  - RGB565; refresh period 33 ms; LV_USE_OS NONE; one software draw unit; draw ASM off.
  - LV_STDLIB_CUSTOM allocator: PSRAM-first on the device, plain malloc in the sim so ASan sees every allocation.
  - LV_USE_SNAPSHOT and LV_USE_OBJ_NAME on (stable selectors for tests and the console); LVGL log routed to ESP_LOG.
  - Dev builds add the asserts, LV_USE_CHECK_ARG with assert-on-fail, LV_USE_CHECK_OBJ_VALIDITY, an abort() handler and sysmon.
  - Demos, examples, unused widgets and the Montserrat font sizes are disabled; LV_USE_TEST is enabled only in the sim.
- Backlight ADR: an own AW9364 component that implements the datasheet pulse protocol, with levels 0–16. If the tracked level is ever in doubt, re-sync by a full shutdown and re-enable. No LEDC. The level arithmetic is unit-tested on the host.
- Input ADR: an own two-button driver. An any-edge GPIO ISR feeds a queue; a pure FSM handles debounce, long press, double press and combos and is unit-tested on the host; its timer runs only while a button is held. The press that woke the chip is injected as a real event using esp_sleep_get_wakeup_causes() and the ext1 status. An LVGL keypad indev is fed from the same queue. Neither espressif/button nor esp_lvgl_port's nav helper is used.
- Power ADR. Sleep sequence:
  1. Stop the UI.
  2. esp_lcd_panel_disp_sleep(true) to put the panel into SLPIN.
  3. Backlight EN low for at least 3 ms.
  4. GPIO15 low with gpio_hold_en.
  5. Park the LCD bus pins and hold GPIO38 (gpio_hold_en + gpio_deep_sleep_hold_en).
  6. ext1 ANY_LOW wake on GPIO0|GPIO14, with RTC pull-ups and RTC_PERIPH kept on.
  7. Enter deep sleep.

  On boot, release the holds in the documented order. Dev mode keeps the board awake while a USB host is connected. Every power and latency figure is measured and recorded in the ADR.
- Boot-time ADR: target ≤500 ms button-to-first-frame, pending the user's own target. PSRAM memtest off, no XIP from PSRAM, skip image validation on deep-sleep wake, low log levels in battery builds. Milestone timestamps are kept in RTC memory. HIL fails the build on a regression of more than 20%.
- Diagnostics ADR: a coredump partition with ELF core dump to flash, read via idf.py coredump-info. Reset reason and the last N log lines go into an RTC_NOINIT ring buffer. On battery, a panic reboots rather than halts. Dev builds log task stack high-water marks and the minimum free internal and PSRAM heap on every wake.
- Console and HIL ADR: the USB-Serial-JTAG primary console runs an esp_console REPL in dev builds, with ui.goto, ui.key, ui.snap, power.stay, power.sleep, bl.set, heap, tasks, log.dump, time.set and wifi.set. Host scripts use esp-idf-monitor ≥1.10 in non-interactive mode (--no-reset, expect with timeouts) together with idf.py build/flash.
- Visual-test ADR:
  - Headless lv_test at 320×170 with the display set to RGB565, one golden PNG per screen and state.
  - Tolerance 0 while draw ASM is off.
  - CREATE_REFERENCE_IMAGE=0 in CI; goldens change only through an explicit update command plus a commit, reviewed by the agent from the *_err.png files.
  - On the device, ui.snap output is compared against the same goldens to catch config drift.
  - Camera checks only for panel-level properties.
- Simulator ADR: an own minimal CMake project, not a fork of lv_port_pc_vscode. It builds ui/ against LVGL at exactly the dependencies.lock version (FetchContent by tag/SHA, verified in CI), with gnu23 and -funsigned-char to mirror Xtensa. Tests build with Homebrew LLVM under ASan, UBSan and LSan. An SDL viewer is an optional target. The sim's heap cap equals the device's LVGL budget.
- C dialect ADR: a C23 subset that both GCC 15.2 and Apple clang 21 accept. Banned: <stdbit.h>, #embed, plain-char arithmetic, and assumptions about integer widths. Use PRIx32-style format macros. -Wall -Wextra -Werror apply to own components only. CI compiles ui/ and the pure modules with both compilers.
- Partition ADR: nvs 64 KB, otadata 8 KB, phy_init 4 KB, ota_0 and ota_1 at 4 MB each, coredump 256 KB. About 7.6 MB stays unallocated: no SPIFFS, and LittleFS only if assets need it. Rollback is on, and the app marks itself valid after a self-test and before its first deep sleep.
- Fonts ADR:
  - lv_font_conv pinned to a git SHA in package.json + lockfile, run under Node 24 to match its engines pin, with --lv-include lvgl.h.
  - Glyph ranges: ASCII, the 134 precomposed Vietnamese letters and the symbols in use; 4 bpp.
  - One regeneration script, with CI failing if the output differs; generated files excluded from format and lint.
  - An NFC check over all UI strings.
  - Screenshot tests that include stacked-diacritic strings (e.g. 'Ẫ Ỗ Ữ ậ ặ').
- Time ADR: system time is carried across deep sleep by the RTC. Only POSIX TZ strings such as '<+07>-7' are used, never Olson names. Date/time formatting lives in pure helpers. The RTC clock source defaults to INT_8MD256 pending the user's accuracy answer. The last sync time and drift estimate are kept in RTC memory, not NVS.
- Networking ADR: SNTP via esp_netif_sntp, with Wi-Fi started only when a sync is due. If BLE ever returns, use NimBLE, never Bluedroid. Endpoints must support TLS 1.3 or ECDHE. No third-party crypto that is not PSA-native.
- PSRAM ADR: PSRAM holds only the LVGL heap, layers and the snapshot buffer. It is never a DMA source for the LCD and never holds code or rodata. Re-evaluate disabling PSRAM entirely if the measurements show a cost.
- Agent-docs ADR: CLAUDE.md keeps only rules that cite a root cause or a test. The three folklore rules (no containers, no updates from tasks, full-screen buffer) are replaced by the single-owner rule and by stress tests that must pass first. Every ADR links its evidence: datasheet sections, issue numbers, measurements.
