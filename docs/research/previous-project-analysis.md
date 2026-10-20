# Previous project analysis: zen-clock

Date: 2026-10-20

Method: single research agent; read-only analysis of the old repository and its git history (nothing built or modified)

Status: unverified research notes; stack-critic.md refines some points (issue #104 is in zen-clock's own tracker, the button 'quirks' are documented API semantics, and the project adopted PlatformIO's packaging of IDF 6.0.1 rather than IDF 6.0 in its release week)

Superseded by decision: several reusable items listed here were replaced. The USB test is GPIO15 LOW then GPIO4, not the ≥4600 mV VSYS threshold; time is fixed UTC+7 in `core/`, so `timezone_fmt` is not ported; the partition table is new (ADR-0013); the display bring-up follows ADR-0006 (10 MHz, internal DMA buffers) instead of zen-clock's `bsp_display.c` (17 MHz, PSRAM buffers); the fonts are DSEG7 and Be Vietnam Pro, not DS-Digital.

This analyses zen-clock (the previous project) to find exactly what made it hard to maintain, so the new stack avoids it, and what is worth reusing. Its repository location and remotes are left out of this public copy.

## Executive summary

1. **Main cause: the project started on ESP-IDF 6.0.0 the week it shipped** (first sdkconfig header, `34b66c9`, 2026-05-09). The ecosystem wasn't ready for it. `esp_lvgl_port` had to come from esp-bsp git master. The Tailscale stack (MicroLink + wireguard-lwip, written for IDF 5) needed patches for mbedTLS 4/PSA, GCC 15 and lwIP/esp_netif internals.
2. **Upstream was dead, so the user ended up maintaining and publishing two forks.** That is 59 + 15 of their own commits in the local microlink and wireguard-lwip fork checkouts, against 102 on zen-clock `main`. This is the "dig into the core layers".
3. **The build glue and the threading made it worse.**
   - Build: PlatformIO's repackaged IDF, plus one tracked, generated 130 KB sdkconfig that holds secrets and has no `sdkconfig.defaults`, plus scripts working around PlatformIO quirks.
   - Threading: a hand-written WiFi state machine, BLE provisioning, and the LVGL lock taken from about 7 tasks.
   - Result: a 42 KB CLAUDE.md full of "don't fix this back" traps, and a UI that cannot be built off the device.
4. **No downloaded component was edited in place.** Every `.component_hash` matches `dependencies.lock`, no file is newer than its hash, and there are no patch files (except LVGL's own upstream rlottie one). All the patches live in the two forks.
5. **Worth reusing:**
   - the pure-logic modules with their 66 host tests
   - the T-Display-S3 board facts and the partition table
   - the ADR-0007 "publish, then reconcile" UI pattern, CONTEXT.md and the ADRs
   - the git-workflow skill, release-please and the CI gates
   - the clock-face design and colour palette

   Caveats: the DS-Digital font is shareware, and no font in the project has Vietnamese glyphs.

## 1. Stack

- **Framework and build:** ESP-IDF only, no Arduino. `platformio.ini` uses `platform = espressif32@7.0.1`, which is the official platform. Its installed `framework-espidf` 4.60001.0 is IDF 6.0.1, with GCC 15.2. Other settings:
  - `extra_scripts = post:scripts/fix_compiledb.py`
  - a custom `partitions.csv`
  - clang-tidy `check_flags`
  - an `[env:native]` environment (Unity) for host tests
- **How the CMake and dependency files are wired:**
  - The root `CMakeLists.txt` is a plain IDF project. `src/` acts as the main component (`src/CMakeLists.txt`).
  - `src/idf_component.yml` declares lvgl ^9.5.0, esp_lvgl_port ^2.9.0, microlink ^3.0.0 (the owner's fork), adc_battery_estimation ^0.2.2, and `idf >=6.0,<7.0`.
  - `components/bsp/idf_component.yml` declares espressif/button ^4.2.0; `components/ble_provisioning/idf_component.yml` declares network_provisioning ^1.2.4.
  - `dependencies.lock` is committed and CI checks it for drift.
  - Configuration is the tracked `sdkconfig.lilygo-t-display-s3`; there is no `sdkconfig.defaults`.
- **LVGL:** 9.5.0, configured only through Kconfig (`CONFIG_LV_CONF_SKIP=y`, no `lv_conf.h`). Montserrat 12/14/20/24/48 and the QR-code widget are on. `LV_USE_PERF_MONITOR`/`SYSMON` are also left on.
- **Display:**
  - IDF's own `esp_lcd` 8-bit i80 bus at 17 MHz plus its built-in ST7789 driver, through `esp_lvgl_port` 2.9.0.
  - Full-screen double buffer in PSRAM with byte swap; LVGL task on core 1, priority 2, 6 KB stack (`components/bsp/src/bsp_display.c`).
  - Backlight via LEDC PWM at 5 kHz/10-bit (`components/lcd_backlight`, adapted from hiruna).
  - No TFT_eSPI or LovyanGFX.
- **Input:** two active-low buttons through espressif/button 4.2.0 (since `d0f6070`). Short press, long press (800 ms) and a 3 s emergency hold map to four abstract actions (UP/DOWN/SELECT/BACK). Holding both buttons triggers deep sleep.
- **Language:** C only (32 .c / 29 .h files, about 9.3k lines), compiled with `-std=gnu23 -Werror -O2`. The code uses C23 features (`static constexpr` in `bsp_buttons.c:56`, `[[maybe_unused]]` in `lcd_backlight.c:18`), so it is tied to GCC 15.

## 2. Board variant

- **Non-touch** T-Display-S3 v1.2 (per README). There is no CST816, I2C or touch-input code.
- **Chip:** ESP32-S3R8 with 8 MB octal PSRAM at 80 MHz (code can execute from PSRAM) and 16 MB QIO flash at 80 MHz. CPU at 240 MHz, FreeRTOS tick at 1 kHz.
- **Pins** (`include/board_config.h`):
  - LCD data D0–D7 on 39, 40, 41, 42, 45, 46, 47, 48.
  - LCD control: WR 8, RD 9 (input with pull-up), CS 6, DC 7, RST 5, backlight 38, LCD power 15.
  - Battery on GPIO4 (ADC1_CH3, 100k/100k divider); buttons on GPIO0 and GPIO14.
- **Panel setup:** gap (0, 35), colour invert, `swap_xy` + `mirror_y`. Rotation must match in two places.
- **Peripherals:** WiFi and BLE (Bluedroid with BLE 4.2 features). Nothing else.

## 3. Pain points

**A. Bleeding-edge IDF 6.0**
- The platform was unpinned at first, and `esp_lvgl_port` was taken from the esp-bsp git repo with `pre_release` because no IDF-6 registry release existed. The dev machine and CI also resolved different IDF versions. Both were fixed only in `4898a29`/`c504b9d` (2026-08-10).
- `idf` is capped below 7.0 "because this project carries ESP-IDF 6.x-specific patches" (`src/idf_component.yml`).

**B. Forking the Tailscale stack (the core-layer work)**
- `7fbc4f2` added `vendor/microlink` as a git submodule on a fork branch (`esp-idf-6x-compat`), plus symlinks into `components/` (wireguard_lwip through a symlink of a symlink).
- The fork's patches are listed in the microlink fork's `ESP_IDF_6X_COMPAT.md`:
  - headers that went private in mbedTLS 4 / TF-PSA, and removal of the entropy/CTR-DRBG/`ssl_conf_rng` code
  - fixes for GCC 15 turning warnings into errors
  - the `esp_driver_uart` split
  - esp_netif's lwIP callback reading `netif->state` as an `esp_netif_t*`, which crashed with LoadProhibited. The workaround NULLs `netif->state` around 5 `netif_set_link_*` call sites.
- wireguard_lwip was a diverged vendored copy, and its receive path entered the lwIP core outside `tcpip_thread`, causing a pbuf double-free (`git show a56a8e9^:THIRD_PARTY.md`).
- With upstream inactive, the user cherry-picked open upstream PRs (`82bb587`), then published both forks to the ESP Component Registry (`a56a8e9`, a breaking change). That was blocked on Espressif approving the namespace (`git show f534524^:docs/microlink-registry-migration.md`).
- The `CONFIG_ML_*` options only exist after the component downloads. Keeping them relies on a component-manager sdkconfig snapshot that CLAUDE.md calls "a workaround, not a guarantee" (CLAUDE.md, Tailscale section).

**C. One generated sdkconfig as the only source of truth**
- It is tracked with no defaults file. The config generator wipes comments, so the reasons live in `docs/adr/0005`, and no drift check is possible. 23 commits touched it.
- On 2026-05-21 the CI setup thrashed: `sdkconfig.defaults` added then removed (`7707ae8`/`288d5bf`), `sdkconfig_path` (`a16ec3b`), a pre-copy script (`246395f`). The root cause was git's skip-worktree flag hiding local changes, so CI lacked `CONFIG_MBEDTLS_CHACHAPOLY_C` and failed to link (`ebb176e`).
- menuconfig writes the Tailscale auth key straight into the tracked file. That forced `scripts/check_secrets.py` plus a CI step, because PlatformIO cannot layer config fragments (README, Tailscale Credentials section).
- Button timing lives in `CONFIG_BUTTON_*`, guarded by `_Static_assert`s (`bsp_buttons.c:49-53`).

**D. PlatformIO-on-IDF glue**
- `scripts/fix_compiledb.py` rewrites compiler paths in `compile_commands.json` for clangd, and `.clangd` strips Xtensa-only flags.
- `pio check` injects `--checks=*`, which overrides `.clang-tidy`. The checks list is therefore duplicated in `platformio.ini`, with `scripts/pio_check.py` guarding against drift (`docs/clang-tidy-suppressions.md`). There are 64 NOLINT suppressions.
- PlatformIO's IDF builder only re-runs CMake when the root or `src/` CMakeLists changes. Its SCons build cache can't see IDF 6 compiler flags (passed in response files), so cache directories are keyed by a hash (ADR-0005, `ci.yml`).
- The release workflow churned through v0.2.1–0.2.3 (`312bc2e`, `583249a`, `6c83c70`). A single self-hosted CI runner is the only thing that can pass the required check, so a runner outage blocks all merges.

**E. Framework quirks and threading**
- `lvgl_port_lock(0)` waits forever. Taking it from the WiFi, timer and BLE tasks caused stalls and status icons that stayed wrong. A bounded lock (`8ac47d3`) came first, then the publish/reconcile rewrite (`ca136fd`, `7981583`, `1a89d64`; ADR-0007).
- Some rules were never root-caused: "UI updates from FreeRTOS tasks cause artifacts on the i80 bus", and "using `lv_obj_create` as a container crashes with StoreProhibited" — yet `clock_face_text.c` does exactly that.
- The hand-written `wifi_manager.c` (818 lines) and `src/app_handlers.c` (858 lines, the most-changed file at 29 commits) produced the wedges and races: a stale stop bit killing WiFi, stop/scan races, a QR-overlay use-after-free, a BLE teardown leak (`51c0972`, `1762f68`; `git show e70f4f0^:docs/AUDIT.md`).
- network_provisioning quirks:
  - You must call `network_prov_mgr_reset_wifi_sm_state_on_failure` (`faf9e93`).
  - It leaves WiFi connected, so a disconnect plus a 300 ms wait is needed before reconnecting.
  - Releasing the 110 KB of BT memory is one-way and needs a reboot to provision again.
- espressif/button quirks:
  - a phantom release event after every long press
  - the lowest threshold must equal the Kconfig value
  - `iot_button_get_key_level()` is inverted relative to the raw pin level
  - a 100 Hz timer runs forever, and its power-save mode conflicts with deep-sleep wake (zen-clock issue #104)
- SquareLine Studio exports needed `patch_ui.bat` after every export, so it was dropped on day 2 (`70930e2`). `lv_font_conv` output is hand-edited after every regeneration (`components/ui/README.md`).
- The net result: CLAUDE.md is 592 lines and was touched by 51 commits.

## 4. Architecture

- **Layout:**
  - `src/main.c` only runs the boot sequence; `src/app_handlers.c` holds all event glue plus two worker tasks.
  - `components/`: bsp, lcd_backlight, ui, settings, wifi_manager, ble_provisioning, sntp_sync, deep_sleep, backoff, battery_view, input_policy.
  - The `ui` component directly requires bsp, settings, wifi_manager, microlink, sntp_sync and deep_sleep, so it cannot be built on a PC.
- **Screens:**
  - `nav.c` is a state machine: Clock ↔ Menu ↔ Settings / System Info, plus the QR overlay. Every screen has create/destroy, and the status bar is rebuilt on each screen change.
  - The clock face is swapped at compile time through `clock_face.h`.
  - Settings is table-driven (`settings_row_t` linked to the settings descriptor table).
- **Tasks:**
  - Tasks: LVGL (core 1), buttons (core 0), btn_worker, net_worker, wifi_mgr, sntp_sync, deep_sleep.
  - Timers: WiFi reconnect backoff, a 10 s Tailscale poll, and the inactivity sleep timer.
  - Locking: other tasks only publish state (under spinlocks); a 250 ms LVGL timer in `ui.c` repaints. Only `nav_handle_action` takes the LVGL lock from outside the LVGL task.
- **Events:** components fire callbacks into `app_handlers.c`. The pure `input_policy` decides what a press means; nav returns any slow follow-up work, which runs on `btn_worker`.
- **Settings:**
  - NVS namespace `zenclock` with 8 keys (`settings_table.c`), clamped on both read and write; edits are saved after a 1 s debounce.
  - WiFi uses namespace `wifi_cred` (SSID/password plus a cached AP BSSID/channel for fast reconnect); MicroLink uses `microlink`/`ml_peers`.
- **Network and time:**
  - BLE provisioning uses Security 2 (SRP6a) with a QR code. Reconnects back off from 30 s doubling to 300 s.
  - NTP servers: `1.ntp.vnix.vn`, pool.ntp.org, time.google.com, plus any from DHCP. The last sync time is kept in RTC memory across deep sleep.
  - Time zone is a whole-hour UTC offset from −12 to +14, applied as a POSIX TZ string; no DST.
- **OTA and partitions:**
  - No OTA code at all, but `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`.
  - `partitions.csv`: nvs 20 KB, otadata, two 4 MB app slots, and a ~7.9 MB SPIFFS partition that is unused.

## 5. Quality and AI tooling

- **Docs:**
  - `CLAUDE.md` and the `CONTEXT.md` glossary.
  - `docs/adr/0001-0008`; the docs evolved README.ai.md → AUDIT.md → DECISIONS.md → ADRs.
  - `docs/agents/` covers the issue tracker (`gh`) and domain docs, using Matt Pocock's skill conventions.
- **`.claude/`:** `skills/git-workflow/SKILL.md` (tracked; Conventional Commits and branch naming) and `settings.local.json` (clangd-lsp and commit-commands plugins). No commands or agents.
- **`.agent/`:** a generic, gitignored "Everything Claude Code" v1.10.0 install for Antigravity. Discard it.
- **`.rtk/filters.toml`:** an RTK (Rust Token Killer) filter that trims `pio run` output for AI agents. `rtk` itself is not installed any more.
- **Lint and format:** `.clang-format` (LLVM base, 2 spaces, Allman braces, 120 columns), `.clang-tidy` (bugprone, cert, clang-analyzer, naming), `.clangd`.
- **Tests:** `test/test_pure_logic` — 66 Unity tests running on the host (PlatformIO `native`), over symlinked pure modules. No on-device tests and no UI tests.
- **CI:**
  - `ci.yml` runs on the self-hosted runner: secrets check, a guard against committed `override_path`, format check, native tests, build, lockfile drift, clang-tidy, artifacts.
  - `release-please.yml` uses a GitHub App token and attaches bootloader, partitions, otadata and firmware binaries with checksums.
  - Dependabot updates the Actions.
- **IDE:** CLion with the PlatformIO plugin (`.idea` last updated 2026-08-24, `.run/PlatformIO.run.xml` tracked, CLion mentioned in `platformio.ini`). `.vscode` was only used 12–18 May.
- **Simulator:** none.

## 6. Reusable assets

- **Fonts:**
  - DS-Digital TTFs, converted with `lv_font_conv` at 4 bpp: the 48 px font has digits, colon, slash and space; the 16 px font has only "A", "M", "P" for AM/PM.
  - The licence (`components/ui/fonts/DIGITAL.TXT`) is shareware: $20 personal, $45 commercial.
  - The rest of the UI uses LVGL's built-in Montserrat. No Vietnamese diacritics anywhere.
- **Images:** none — only LVGL's built-in symbols.
- **Design:**
  - Clock face uses fixed 25 px boxes per digit so digits don't jitter.
  - Palette: background `#e6e6e6` (light) / `#0d0d0d` (dark); digits `#333333` / `#01ddff`; date `#666666` / `#007a99`.
  - The status icon colours have defined meanings.
- **Pure modules worth porting with their tests:** `backoff`, `battery_view`, `input_policy`, `settings_table` + `timezone_fmt`, `prov_session`, `ui_utils`.
- **Board knowledge:**
  - `board_config.h` and the `bsp_display.c` init sequence.
  - The LCD power-rail latch for deep sleep (ADR-0003).
  - Battery: adc_battery_estimation, with ≥4600 mV meaning USB power.
  - Waking from deep sleep on either button (GPIO0/GPIO14).
- **Conventions:** ADR-0007 (publish/reconcile), ADR-0008 (only a successful NTP sync proves the internet works), ADR-0002, `CONTEXT.md`, the git-workflow skill, `release-please-config.json`, `scripts/check_secrets.py`, the lockfile-drift check, and the `.clang-format`/`.clang-tidy` files.

## 7. Git

- **Remote:** omitted from this public copy.
- **Current branch:** `fix/wifi-cancel-pending-reconnect`, one commit (`170f26c`) ahead of `main` and pushed. A release-please branch with 0.6.0 is pending.
- **Dates:** first commit 2026-05-09 (`34b66c9`), last 2026-08-23. There is a gap between 22 May and 10 Aug.
- **Size:** 102 commits on `main`, 185 across all refs; tags v0.2.0–v0.5.0.
- **Convention:** Conventional Commits with scopes (ui, wifi, ble, bsp, deps, release…), squash-merged PR titles ending in `(#N)`, versions driven by release-please. 85 commits are co-authored by Claude.
