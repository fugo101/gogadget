# Gogadget

A pocket gadget on a [LilyGO T-Display S3](https://github.com/Xinyuan-LilyGO/T-Display-S3) (ESP32-S3, 1.9" 170x320 IPS): first a battery-friendly glance clock with the Vietnamese lunar calendar, then timers, a calendar and more, one feature at a time. Built on ESP-IDF v6.1 and LVGL 9.6, written in C, with a host simulator that renders every screen to PNG.

**Status:** early development (v0.1 in progress, see [docs/roadmap.md](docs/roadmap.md)).

> **Tiếng Việt:** Gogadget là firmware cá nhân cho board LilyGO T-Display S3: trước tiên là một đồng hồ xem nhanh tiết kiệm pin (giờ, ngày dương, ngày âm lịch), sau đó thêm hẹn giờ/Pomodoro, lịch và các tiện ích khác. Giao diện hiển thị tiếng Việt; mã nguồn và tài liệu viết bằng tiếng Anh.

## Hardware

- LilyGO T-Display S3 v1.2, non-touch (16 MB flash, 8 MB PSRAM).
- Optional LiPo cell on the 1.25 mm battery connector. Read [ADR-0010](docs/adr/0010-accept-2c-charging.md) before using a small cell: the board charges at about 525 mA.

## Getting started

1. Install ESP-IDF v6.1 with [EIM](https://docs.espressif.com/projects/idf-im-ui/en/latest/) (`brew tap espressif/eim && brew install eim`, then `eim install -i v6.1 -t esp32s3`).
2. Host tools: CMake, Ninja, and SDL2 for the interactive viewer (`brew install cmake ninja sdl2`).
3. Open `gogadget.code-workspace` in VS Code and install the recommended extensions.
4. Build and test:

   ```sh
   make test     # unit tests + golden screenshots, on the host
   make viewer   # interactive simulator window
   make fw       # firmware
   make flash    # flash over USB-C
   ```

   `make help` lists everything. Before the first flash of a board, read [docs/hardware/bring-up.md](docs/hardware/bring-up.md).

## Layout

| Path | What |
|---|---|
| `core/` | Pure C application state and policies (also built on the host) |
| `ui/` | LVGL screens that render the core state |
| `firmware/` | ESP-IDF project: board drivers and the composition root |
| `sim/` | Host build: tests, golden screenshots, SDL viewer |
| `docs/` | Decisions (`adr/`), hardware reference, UI design, research notes |
| `tools/` | Font generator, checks, upstream tracking |

## Documentation

- [GLOSSARY.md](GLOSSARY.md): the project's vocabulary.
- [docs/adr/](docs/adr/): architecture decisions and why they were made.
- [docs/hardware/](docs/hardware/): board reference, power budget, bring-up tests.
- [docs/ui/design.md](docs/ui/design.md): screen layout, fonts, palette.
- [docs/research/](docs/research/): the research behind the decisions.

## License

MIT, see [LICENSE](LICENSE). Third-party material and its licences are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md); fonts keep their own licences in `tools/fonts/src/`.
