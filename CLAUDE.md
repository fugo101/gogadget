# Gogadget

Personal ESP-IDF firmware for a LilyGO T-Display S3 (ESP32-S3, 320x170 ST7789V): a battery-first glance clock that grows into a pocket tool. GLOSSARY.md holds the domain language. Decisions and their reasons are in docs/adr/: read the ADR for an area before changing it, and record a new one when a change is hard to reverse, surprising, and a real trade-off.

Where the work stands and what comes next: `docs/status.md`. Read it at the start of a session and update it when a step lands.

## Layout

- `core/`: pure C state, reducer and policies. No `esp_*`, no LVGL; the host build enforces it.
- `ui/`: LVGL screens that render `gg_state_t` and decide nothing.
- `firmware/`: the ESP-IDF project. `components/board` drives the hardware; `main` is the composition root and the shell that runs the reducer.
- `sim/`: host CMake project with the unit tests, golden screenshots and an SDL viewer.
- `config/lvgl.defconfig`: the single LVGL configuration for device and host.

## Loop

`make help` lists every command. For most changes run `make test` (host) and `make fw` (firmware). For UI changes run `make golden`, open each changed PNG in `ui/test/golden/` and judge it before committing; CI compares pixels.

## Rules

- Hardware values (pins, timings, the panel init table) come from `docs/hardware/t-display-s3.md`; cite its section in a comment next to the value.
- Generated files are regenerated, not edited: `ui/fonts/*.c` (tools/fonts/generate.sh) and `firmware/sdkconfig` (edit `firmware/sdkconfig.defaults`).
- LVGL is pinned in `ui/idf_component.yml`, `firmware/components/board/idf_component.yml` and `sim/CMakeLists.txt`; change them together (`make versions`).
- Credentials live only in gitignored `secrets/` and reach the device over the USB console; tracked files hold placeholders (`make secrets`).
- Commits follow Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `build:`, `ci:`, `chore:`), one focused change each; release-please writes the changelog.
- Git stays local: commit freely, but push, open pull requests or delete branches only when the owner asks.

## On the device

- The board enumerates as USB-Serial-JTAG (`/dev/cu.usbmodem*`). It vanishes during deep sleep; on USB power the firmware stays awake.
- stdin is not a TTY here, so esp-idf-monitor runs in its non-interactive mode: pipe it a script that ends with `expect --timeout <s> <regex>` or `exit`.
- Before the first flash of a board, follow `docs/hardware/bring-up.md` (it backs up the flash).
