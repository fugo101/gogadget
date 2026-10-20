# Research notes

These are point-in-time research notes from the Gogadget design session on 2026-10-20, imported from the agents' reports and lightly reformatted. They record what was found and how, not what was decided: the decisions live in [docs/adr/](../adr/). Where a note disagrees with an ADR, the ADR wins; each note's `Notes:` and `Superseded by decision:` lines list the known differences. Versions, dates and issue states were correct on 2026-10-20 and will drift.

How to read them:

- Every file starts with Date, Method and Status lines. Status gives the verification result where one exists: **confirmed** (primary evidence found), **partially** (true with a correction), **refuted**, or **unverifiable**. UNVERIFIED inside a report marks a claim its author could not confirm.
- Evidence tags in the LilyGO notes: [V] verified against the primary source, [I] inference or estimate (confirm on hardware), [C] correction of an earlier claim or vendor document.
- A bare `#NNN` means an issue or PR in Xinyuan-LilyGO/T-Display-S3 unless stated otherwise.
- Vendor datasheets are cited by title, version and public URL; none are copied here. The `upstream/` folder is maintained separately.

## Early research (single agents)

| File | Question it answers | Method / status | Main conclusion |
|---|---|---|---|
| [framework-choice.md](framework-choice.md) | Arduino or ESP-IDF, and PlatformIO or native idf.py? | single agent; unverified (key claims re-checked later) | Native ESP-IDF v6.1 with idf.py and the Component Manager; skip Arduino and PlatformIO, which lags IDF releases and rebuilds IDF through its own SCons bridge. |
| [ide-tooling.md](ide-tooling.md) | CLion or VS Code, and how can an agent build, flash, log and debug from the command line? | single agent; unverified (key claims re-checked later) | VS Code with Espressif's extension and the Claude Code extension; the agent works through idf.py and esp-idf-monitor's non-interactive mode. |
| [lvgl-ecosystem.md](lvgl-ecosystem.md) | Which LVGL version, simulator, test and design tools suit an agent-built UI? | single agent; unverified (key claims re-checked later) | LVGL 9.6.0; hand-written C plus a desktop simulator and LVGL's test module is the only free loop an agent can run on its own. |
| [example-repos.md](example-repos.md) | Fork an existing T-Display-S3 repo, or start fresh? | single agent; unverified | No repo is worth forking and no BSP exists; start a native ESP-IDF project with one small board component. |
| [previous-project-analysis.md](previous-project-analysis.md) | What made zen-clock (the previous project) hard to maintain, and what is worth reusing? | single agent; unverified | IDF 6 patches in forked Tailscale/WireGuard stacks, PlatformIO glue, a tracked generated sdkconfig and LVGL calls from many tasks; reuse the pure modules, the ADR lessons and the conventions. |

## Stack fact-check workflow

| File | Question it answers | Method / status | Main conclusion |
|---|---|---|---|
| [stack-esp-idf.md](stack-esp-idf.md) | Do the ESP-IDF, esp_lvgl_port, monitor, MCP, QEMU and PlatformIO claims hold? | multi-agent fact-check; 4 confirmed / 3 partially / 0 refuted | Mostly; esp_lvgl_port 2.9.0 with LVGL 9.6 is only build-tested on IDF 6.1, and scripted monitoring works best through `python -m esp_idf_monitor`. |
| [stack-lvgl.md](stack-lvgl.md) | Do the LVGL version, test-module, simulator, tool-licence and esp_lvgl_port claims hold? | multi-agent fact-check; 5 confirmed / 2 partially / 0 refuted | The test-module API is confirmed (set CREATE_REFERENCE_IMAGE=0 in CI); the nav-button helper needs three buttons and rotation never re-applies the panel gap. |
| [stack-ide.md](stack-ide.md) | Do the CLion and VS Code ESP-IDF extension claims hold? | multi-agent fact-check; 1 confirmed / 1 partially / 0 refuted | CLion is free for non-commercial use but its ESP-IDF support is thin; the VS Code extension v2.3.0 features are confirmed. |
| [stack-claude-code.md](stack-claude-code.md) | How do the Claude Code JetBrains and VS Code integrations compare? | multi-agent fact-check; 7 confirmed / 2 partially / 0 refuted / 1 unverifiable | The VS Code extension is GA with per-change review and checkpoints; the JetBrains plugin is a stale beta that wraps the CLI in a terminal. |
| [stack-simulator.md](stack-simulator.md) | How should the repo share UI code between device and host, with headless screenshots? | multi-agent research; unverified | Option S1: a plain host CMake `sim/` that builds `core/` and `ui/`; the host build fails if UI or core code includes esp_* headers. |
| [stack-critic.md](stack-critic.md) | Where do the reports disagree, what are the risks, and why did zen-clock's folklore rules exist? | multi-agent review; not separately verified | An out-of-spec 17 MHz bus with PSRAM DMA and LEDC PWM on a pulse-count AW9364 explain the old problems; it also lists risks, gaps, owner questions and ADR proposals. |

## Power, provisioning, lunar calendar, alerts

| File | Question it answers | Method / status | Main conclusion |
|---|---|---|---|
| [power-budget.md](power-budget.md) | What does the board draw, and how should sleep, wake, backlight and timekeeping work? | research + verifier; 7 confirmed / 2 partially / 0 refuted | Deep sleep is the only standby: SLPIN the panel, dim the AW9364 by pulse counting, boot fast and sync NTP about 4x/day; the only measured sleep floor is about 0.26 mA. |
| [provisioning.md](provisioning.md) | How should Wi-Fi credentials reach a deep-sleep clock? | research + verifier; 9 confirmed / 1 partially / 0 refuted | espressif/network_provisioning over BLE (NimBLE, Security 2, per-session QR password) in a dedicated boot, plus an own multi-network list; deferred, since v1 uses the USB console. |
| [lunar-calendar.md](lunar-calendar.md) | How to compute the Vietnamese lunar calendar on the device, correctly and with a clean licence? | research + verifier; 9 confirmed / 1 partially / 0 refuted | Own integer C code with 1900-2100 tables generated from JPL DE440s using Vietnamese rules; never use Ho Ngoc Duc's non-commercial code or tables. |
| [alert-hardware.md](alert-hardware.md) | Which buzzer or haptic part could signal a Pomodoro alert, and on which pin? | research + verifier; 10 confirmed / 0 partially / 0 refuted | A passive piezo on GPIO21 through 470 Ohm on its own LEDC timer; the example Murata part reaches end of life in 2027. |

## LilyGO deep dive

| File | Question it answers | Method / status | Main conclusion |
|---|---|---|---|
| [lilygo-knowledge-base.md](lilygo-knowledge-base.md) | What are the verified facts about this board? | multi-agent synthesis; top claims verified (19 confirmed / 5 partially / 0 refuted) | Power tree, panel init, AW9364, buttons, console and timekeeping facts, each tagged [V], [I] or [C]. |
| [lilygo-reuse-plan.md](lilygo-reuse-plan.md) | Which LilyGO code can be reused, how, and under which licence? | multi-agent synthesis; not separately verified | Hand-port only small MIT pieces (CMD_1 table, bring-up order, AW9364 step maths, sleep order) with notices; never copy GPL or unlicensed examples. |
| [lilygo-upstream-tracking.md](lilygo-upstream-tracking.md) | How to notice upstream LilyGO changes that matter? | multi-agent synthesis; script tested on 2026-10-20 | Pin both repos by SHA and run a monthly drift check (script plus GitHub Action) that opens an issue. |
| [lilygo-verification.md](lilygo-verification.md) | Do the 24 most important sweep claims hold? | multi-agent verification; 19 confirmed / 5 partially / 0 refuted | None refuted; corrections cover the two undocumented CMD_1 entries, drawing a frame before DISPON and the backlight, and several quote dates. |
| [lilygo-decision-impacts.md](lilygo-decision-impacts.md) | What does the deep dive change in the stack decisions? | multi-agent synthesis; 12 keep / 13 adjust / 1 reconsider | Keep the stack; adjust display bring-up, sleep pin parking, sensing and timekeeping; reconsider the 250 mAh cell at the stock charge current (the owner kept it). |
| [lilygo-sweep-display.md](lilygo-sweep-display.md) | What do LilyGO's display, backlight and LVGL sources add? | multi-agent sweep; 10 of 48 items verified (8 confirmed / 2 partially / 0 refuted) | CMD_1 decoded; IDF 5+ no longer sends DISPON; 10 MHz is LilyGO's only in-spec clock; no TE line; the AW9364 only dims by pulse counting. |
| [lilygo-sweep-power.md](lilygo-sweep-power.md) | Which power, sleep and battery facts change the design? | multi-agent sweep; 4 of 33 items verified (4 confirmed / 0 partially / 0 refuted) | GPIO4 measures switched VSYS (a clean USB test), R12 and the LDO are fixed drains, and the stock charge current is about 2C for a 250 mAh cell. |
| [lilygo-sweep-apps.md](lilygo-sweep-apps.md) | What do the other examples, lib/ and the tooling teach? | multi-agent sweep; 2 of 48 items verified (2 confirmed / 0 partially / 0 refuted) | v1.3 boards with a better antenna exist; OneButton is a good button-FSM reference; erase the flash once; do not copy the MIDI, PCBClock or GPL examples. |
| [lilygo-sweep-idf.md](lilygo-sweep-idf.md) | Is LilyGo-Display-IDF usable as an IDF reference? | multi-agent sweep; 4 of 29 items verified (3 confirmed / 1 partially / 0 refuted) | Only for board facts and the panel table: its code is IDF 5.3 and LVGL 8, unchanged since 2024-01-31, and does not build on IDF 6.x. |
| [lilygo-sweep-issues-a.md](lilygo-sweep-issues-a.md) | What do issues and PRs #1 to #176 reveal? | multi-agent sweep; 3 of 36 items verified (2 confirmed / 1 partially / 0 refuted) | Drive RD high; switching between USB and battery reboots the board; VBUS forces PWR_EN on; there are two panel generations; the antenna is weak. |
| [lilygo-sweep-issues-b.md](lilygo-sweep-issues-b.md) | What do issues and PRs #177 to #352 reveal? | multi-agent sweep; 1 of 45 items verified (0 confirmed / 1 partially / 0 refuted) | The power path and fixed sleep drains, ghosting on matte panels without SLPIN, R10 fuse wear and unterminated USB-C CC pins. |
| [lilygo-sweep-history.md](lilygo-sweep-history.md) | How is the upstream repo maintained, and what changed over time? | multi-agent sweep; unverified | One maintainer, no tags or releases (pin by SHA), fast reactive fixes; v1.3 boards exist without a published schematic. |
