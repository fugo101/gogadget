# Core owns the state, one task owns LVGL, everything else posts messages

All application state (current screen, focus, clock, status, power mode) lives in `core/` and changes only through a reducer that takes a message and returns effects; `ui/` renders that state and never decides anything; `firmware/` and `sim/` are shells that feed messages in and execute effects (sleep, backlight, start a sync). On the device a single task owns LVGL: it drains a message queue from an LVGL timer, runs the reducer and re-renders. zen-clock let several tasks call LVGL under a lock, which produced lock-ordering bugs and made behaviour depend on task timing; with one owner there is nothing to lock, and every behaviour can be replayed on the host from a list of messages.

## Consequences

- Services (buttons, Wi-Fi, time sync, battery) never include LVGL; they post messages such as `INPUT`, `TICK`, `STATUS`.
- Navigation is part of core state, so it is unit-tested in pure C; LVGL input devices and groups are not used.
- Effects are data, so the simulator can log them instead of performing them.
