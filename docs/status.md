# Status

Snapshot for picking the work up in a new session. Decisions live in `docs/adr/`; this file only says where things stand. Update it when a milestone step lands.

Last updated: 2026-10-20.

## Done (v0.1 in progress)

- Foundation: core reducer, button state machine, battery and civil time; host build with unit tests and golden screenshots; SDL viewer (`make viewer`); CI (firmware build in `espressif/idf:v6.1`, host tests on x86_64, secrets/format/version checks); release-please via the org's release app; monthly LilyGO upstream drift check.
- Watchface v1: variant C (minimal seven-segment, spec in `docs/ui/design.md`, proposals in `docs/ui/proposals/watchface-v1/`).
- Lunar calendar (ADR-0015, `docs/lunar.md`): JPL DE440s tables 1900-2100, integer C decoder, Can Chi, solar terms; verified against HKO day by day and three independent engines.
- Firmware written but **not yet run on hardware**: i80 display at 10 MHz with the vendor init table and DISPON after the first frame, AW9364 backlight, buttons, VSYS/USB sensing, deep sleep with pin parking, dev console (`ui.goto`, `ui.key`, `bl`, `time.set`, `power.stay`, `sys.info`, `heap`, `reboot`), console only on USB power.
- Docs: hardware reference, power budget, bring-up tests (`docs/hardware/`), research notes (`docs/research/`).

## Next

1. **Board bring-up** (needs the board on USB-C), following `docs/hardware/bring-up.md`:
   - first flash: `make backup PORT=...` (saves the old zen-clock flash to `backups/`), `make erase`, `make flash`;
   - DSP tests with `ui.goto test` (colour bars, 1-px frame, orientation), cold boot vs deep-sleep wake, backlight levels (`bl 0..16`), buttons, sleep;
   - photo of the panel to approve the palette (colours differ from the simulator);
   - fill in the per-unit log.
2. **Wi-Fi + time**: `wifi add/list/del` console commands (multi-SSID in NVS, credentials from gitignored `secrets/`), NTP Sync four times a day plus on every USB session (servers 1.ntp.vnix.vn, pool.ntp.org), drift learning, Unsynced badge after 24 h; save the epoch to NVS every minute on USB (a brownout resets the RTC on the S3).
3. **Power model on hardware**: measure wake-to-time (target 0.5 s; reset-free panel wake if over), glance timeout, Always-on on USB, battery level, idle-drain test, RC vs 8MD256 drift.
4. OTA image marks itself valid before the first deep sleep.
5. Release v0.1.0 by merging release-please's PR #1 once the above works.

## Open questions (hardware)

GPIO0 wake (untested by LilyGO); whether unplugging USB resets the board and loses the time; the unexplained 120-190 µA of sleep current; USB-C CC resistors (C-to-C chargers); matte-panel ghosting after long sleep; real awake current per backlight level.

## Tooling gotchas

- VS Code: open `gogadget.code-workspace`; the ESP-IDF extension always selects the first workspace folder, so `firmware` stays first. It writes per-machine state to `firmware/.vscode/` and a coarse `firmware/.clangd` (both gitignored); the root `.clangd` is the real one.
- `firmware/CMakePresets.json` is ours on purpose: idf.py uses its `default` preset whenever the file exists, and its `binaryDir` must stay relative (`build`).
- idf.py compares the interpreter path as a string: run it through `venv/bin/python3` (the Makefile does). If the extension or a terminal reports "configured with ... python", run `make fw-clean` once.
- `make golden` regenerates references; review the PNG diff before committing. Golden paths are relative to the repo root (LVGL drops the leading `/` of absolute paths when creating folders).
- `make lunar-check` needs `tools/lunar/.venv` (see `tools/lunar/README.md`).
