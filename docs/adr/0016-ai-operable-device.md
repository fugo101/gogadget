# Make the device operable by an AI agent

An AI agent is expected to develop Gogadget end to end, including LVGL screens, so every loop it needs is scriptable: the simulator renders screens to PNG files it can view (ADR-0005); the firmware exposes a development console over USB-Serial-JTAG (`ui.goto`, `ui.key`, `ui.snap`, `power.stay`, `wifi.add`, `sys.info`, `heap`, `log.dump`, `bl`); and the agent talks to the board through esp-idf-monitor's non-interactive mode (stdin is not a TTY under Claude Code), always ending a script with `expect --timeout` or `exit`. Because the USB device disappears in deep sleep, `power.stay` and the always-on USB mode keep the console reachable while developing.

## Consequences

- Console commands are part of the firmware's interface and are kept stable and documented.
- The agent may build, test, flash and read logs, and commit locally; it never pushes, opens pull requests or deletes branches unless asked.
