# Tracking LilyGO upstream

Gogadget learns about its board from two LilyGO repositories but builds none of their code. This page explains why, what is pinned and watched, how to triage a change, and what may be copied from upstream. Bare issue numbers (#NNN) refer to Xinyuan-LilyGO/T-Display-S3.

## A knowledge source, not a dependency

- Nothing upstream builds on ESP-IDF 6.x. LilyGo-Display-IDF targets IDF v5.3 with LVGL ^8.3.11, and its T-Display-S3 path is unchanged since 2024-01-31. T-Display-S3 is Arduino (CI pins core 2.0.17; only the factory example builds on core 3.3.11, c9b939f) plus PlatformIO. So there is nothing to depend on: no submodule, no component, no vendored copy.
- The value is knowledge: the pinout, the power tree, the vendor init bytes, the maintainer's answers in issues. A fact is checked against the schematic, a datasheet or the board before it shapes code, and is written down in [docs/hardware/t-display-s3.md](hardware/t-display-s3.md) or an ADR, with the upstream commit or issue as evidence.
- The code is not a quality reference: pixel clocks of 16, 20 and 40 MHz against a limit of about 15 MHz, PWM backlight code in examples (wrong for the AW9364, ADR-0007), unchecked error codes, and the `j < len & 0x7f` precedence bug in four examples.
- Upstream edits in place. There are no tags or releases, and the schematic PDF has had three versions under one file name (2022-10-10, 2023-03-13, 2024-04-29) while the charger changed twice (TP4054, TP4056B, TP4065). Hence pins by full SHA, a local copy of the schematic, and a monthly watch.

## Pins

| Repository | Pinned commit | Date | Branch | Note |
|---|---|---|---|---|
| [Xinyuan-LilyGO/T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3) | `1cc41eb1c76b2f3215130a3dd64e7138ce00b482` | 2026-10-13 | main | "Fix docs" (#352) |
| [Xinyuan-LilyGO/LilyGo-Display-IDF](https://github.com/Xinyuan-LilyGO/LilyGo-Display-IDF) | `b1a1cc54994bf1b417e3bb30c437bbe1036bff7f` | 2025-11-03 | master | Merge of its PR #18; its T-Display-S3 files are unchanged since c465092 (2024-01-31) |

- [tools/upstream.lock](../tools/upstream.lock) is the source of truth; the table shows the state on 2026-10-20.
- Archive: only LilyGO's MIT schematic, as [docs/research/upstream/T_Display_S3-schematic-b0a5ecd.pdf](research/upstream/T_Display_S3-schematic-b0a5ecd.pdf) (blob `318cc4f`, last changed in b0a5ecd on 2024-04-29). Vendor datasheets are recorded as title, version, URL and SHA-256 in [datasheets.md](research/upstream/datasheets.md).
- Gogadget's own toolchain pins (ESP-IDF v6.1, LVGL 9.6.0, esp_lvgl_port 2.9.0) live in the firmware's `idf_component.yml` and `dependencies.lock`, not here (ADR-0001, ADR-0002).

## What is watched, and why

**T-Display-S3, hardware truth**
- `schematic/`, `datasheet/`, `image/`, `dimensions/README.MD`.
- `README.md` and `README_CN.md`: the pinout, the GPIO15 note, the FAQ, and the LED, Wi-Fi antenna and electrical sections. Corrections land here, often within days of an issue (#343, #352).

**T-Display-S3, reference code**
- `examples/factory/factory.ino` and its `pin_config.h`: the init table (CMD_1), `setBrightness()`, sleep, SNTP.
- `examples/Sleep/Sleep.ino` and `examples/GetBatteryVoltage/GetBatteryVoltage.ino`.
- `examples/lv_demos/lv_demos.ino` and its `pin_config.h`; `examples/tft/tft.ino`.
- `lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h`, the only watched file under `lib/` (TFT_eSPI's pins, colour order, inversion and init-sequence choice for this board).

**T-Display-S3, toolchain and recovery signals**
- `.github/workflows/`, `platformio.ini`, `boards/`, `firmware/README.MD` (the factory images and how to flash them).

**LilyGo-Display-IDF**
- `main/display_s3.c` (bus setup and the init table), `main/main.cpp`, `main/power_driver.cpp`, `main/product_pins.h` (the T-Display-S3 block), `main/Kconfig.projbuild`.
- `sdkconfig.defaults.t-display-s3`, `setup.py` (the IDF and LVGL targets), `README.md`.
- Not `main/initSequence.*`: it serves the AMOLED boards, and `display_s3.c` does not include it.

**Ignored**
- `lib/` (vendored libraries), the community MIDI and LovyanGFX examples (they still drive the backlight with PWM and use wrong pins), the 3D shells, `nes`, `PCBClock` and `PokerS3`.

**Issues and pull requests in both repositories**
- Each run lists those updated since the previous run and flags hardware keywords with `!`: sleep, backlight, GPIO15, battery, charg*, schematic, v1.x, revision, ST7789, screen, power, current, ADC, antenna. Issue activity alone is not drift.
- Follow LilyGo-Display-IDF #16 (a Component Registry BSP request) and LilyGo-Display-IDF #8 (slow LVGL): either could signal an IDF 6 / LVGL 9 port.

## Running the watch

**In CI.** [.github/workflows/upstream-watch.yml](../.github/workflows/upstream-watch.yml) runs at 04:23 UTC on the 1st of each month and on demand (Actions, upstream-watch, Run workflow). It installs `poppler-utils` for PDF word diffs and runs the script with a token limited to `contents: read` and `issues: write`.
- Exit 10: it opens an issue "Upstream drift YYYY-MM-DD" labelled `upstream-drift`, or comments on the open issue with that label instead of opening a duplicate. The report goes in a fenced block (so upstream text cannot mention people or link issues here), cut at 60,000 characters on a line boundary; the summary comes first, so a cut only drops per-file diffs, and the full report stays in the run log.
- Exit 0: no issue; the report is in the job summary.
- Any other exit code fails the job, and GitHub notifies the repository owner.
- GitHub disables scheduled workflows in a public repository after 60 days without repository activity. A disabled watch looks like "no drift", so re-enable it on the Actions tab when that happens.

**Locally.** Run `tools/upstream-watch.sh` (optionally with a lock file path). It needs git; `gh` adds the issue scan and `pdftotext` the PDF word diffs. Blob-less bare clones are kept in `$UPSTREAM_CACHE` (default `${XDG_CACHE_HOME:-$HOME/.cache}/gogadget-upstream`), together with the date of the last complete run, so each issue scan starts where the previous one ended. CI starts with an empty cache and scans the last 31 days. `UPSTREAM_SINCE=YYYY-MM-DD` sets the start date by hand.

**Output and exit codes.** 0 means no drift, 10 drift, 2 an error (the report is incomplete). The report gives a summary per repository (both commits, the commits that touch watched paths, a diffstat, the issue scan), then per-file details: text diffs capped at 400 lines per file, word diffs for PDFs with Altium object names filtered out, one line per binary file.

**Tested on 2026-10-20 [V].** With the pins as given: exit 0. With T-Display-S3 pinned at 02f0e9d: exit 10, with 14 commits (the README rework, the new `README_CN.md`, the factory changes of c9b939f, the board-file rename and `image/WiFi.png`). Exit codes and drift details were identical on macOS (bash 3.2, BSD tools) and Ubuntu 24.04 (bash 5.2, GNU date, mawk).

## Triage

Put each change in the report into one class:
- **A. Hardware truth**: the schematic; the README power, pinout, LED or electrical sections; a new board revision. Update [docs/hardware/t-display-s3.md](hardware/t-display-s3.md) and any affected ADR, then schedule a re-check on the board.
- **B. Init, backlight, sleep or pin code**: the CMD_1 bytes, `setBrightness()`, the sleep order, `pin_config.h`, `product_pins.h`. Diff it against Gogadget's port (the files listed in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)), then rerun the display, backlight and sleep acceptance tests on the board.
- **C. Toolchain or docs only**: workflows, `platformio.ini`, `boards/`, `firmware/README.MD`, wording. Say so in the commit message and bump the pin.
- **D. Ignore**: community examples, vendored libraries. Bump the pin.

Bump the pins in `tools/upstream.lock` in the same commit as the notes they affect, for example `docs(hardware): record the new README FAQ, bump T-Display-S3 pin to 1a2b3c4`. Then close the drift issue.

## Out-of-cycle checks

Run the script, or dispatch the workflow, as soon as one of these shows up (for example in a notification from watching the repositories, or in a followed issue thread):
- a new schematic PDF, or a v1.3 schematic;
- any change to the CMD_1 table, `pin_config.h`, `setBrightness()` or the sleep sequence;
- a LilyGo-Display-IDF commit that touches `display_s3.c` or adds IDF 6 / LVGL 9 support.

## Quarterly manual checks

The script cannot see these. Check them in January, April, July and October:
- The [lilygo.cc T-Display-S3 page](https://lilygo.cc/products/t-display-s3): the version selector and the V1.3 notes (SKUs H569 and H577 are the non-touch boards). V1.3 (silkscreen "25-9-12 V1.3") is described only as a re-matched antenna (#340), and no V1.3 schematic is published.
- The [LILYGO wiki page](https://wiki.lilygo.cc/products/t-display-series/t-display-s3/): its "Version History" table (empty on 2026-10-20; the page lags the README).
- FCC ID [2ASYE-T-DISPLAYS3](https://fccid.io/2ASYE-T-DISPLAYS3), granted 2026-09-24 and probably V1.3 [I] (the 2022 grant is [2ASYE-T-DISPLAY-S3](https://fccid.io/2ASYE-T-DISPLAY-S3)): the internal photos, for R12, the LDO marking and the USB-C CC resistors.
- The [Espressif Component Registry](https://components.espressif.com/): any T-Display-S3 BSP (none on 2026-10-20).
- The job summaries of the last three upstream-watch runs: flagged (`!`) issues that changed no watched path.

## "Closed" is not "fixed"

- T-Display-S3 runs a stale bot (`.github/workflows/issues.yaml` at 1cc41eb, actions/stale) [V]. An issue is marked stale after 30 days without activity and closed 14 days later; a pull request is marked stale after 1 day and closed 1 day later. A closed issue may never have been answered.
- Read the thread. Trust a commit, a README change or a maintainer statement, not the issue state.
- LilyGo-Display-IDF has no workflows at the pin, so its issues stay open until someone closes them; LilyGo-Display-IDF #8 has no answer.

## Licensing policy

Both repositories are MIT: T-Display-S3 "Copyright (c) 2022 Xinyuan-LilyGO"; LilyGo-Display-IDF "Copyright (c) 2024 TTGO", with file headers "Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd". The full texts are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

**May be used, with attribution**
- Small MIT pieces with no better source: the 14-entry CMD_1 table (LilyGo-Display-IDF `main/display_s3.c:32-47`, byte-identical to T-Display-S3 `examples/factory/factory.ino:133-149`) and the AW9364 step maths (T-Display-S3 `examples/factory/factory.ino:223-248`).
- LilyGO's own schematic, archived unmodified; it is the only upstream file stored in this repository.
- Each port gets the per-file header `/* Portions derived from Xinyuan-LilyGO/<repo> <path> @<sha> (MIT). See THIRD_PARTY_NOTICES.md. */`, a row in THIRD_PARTY_NOTICES.md, and the notices in every release archive.
- Facts (pin numbers, part values, timings, maintainer statements) are restated in our own words in the hardware notes, with the commit or issue as evidence.

**Never copied, and why**
- `examples/ULP_ADC`, `examples/ULP_Count`, `examples/WIFI_Audio_I2S`: their headers say GPL 3.0 inside the MIT repository; copying them would put Gogadget under the GPL.
- `examples/PCBClock`, `examples/PokerS3`: taken from VolosR repositories that have no licence, so there is no permission to copy at all.
- `examples/factory/font_Alibaba.c`: a rendered typeface whose licence is unknown.
- `lib/`: vendored third-party code under its own licences, not LilyGO's MIT (for example `lib/ESP32-audioI2S` is GPL-3.0; `lib/arduino-nofrendo` and `lib/DabbleESP32` are LGPL).
- `datasheet/`: the vendors' copyright, not LilyGO's to license; the ST7789V pages are marked "Sitronix Confidential". Datasheets are cited by title, version, URL and SHA-256 only.
- Related repositories: krupis/T-Display-S3-esp-idf (no licence) and llgok/lilygo_device_driver (GPL-3.0, and it only covers P4 boards). zef/t-display-s3-espidf-lvgl (MIT) is for reading only.
- OneButton (`lib/OneButton` 2.0.4, BSD) is not needed: Gogadget writes its own button state machine (ADR-0008).

Notes: the research behind this page also proposed archiving the ST7789V, AW9364 and TP4065 PDFs and adding OneButton's BSD notice. The settled decisions keep only LilyGO's schematic in the repository and use an own button state machine, so neither is adopted.
