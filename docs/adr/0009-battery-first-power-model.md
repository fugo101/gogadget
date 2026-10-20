# Battery first: deep sleep between glances, always-on only on USB

On battery the device spends its standby time in deep sleep and wakes on a button for a 10-second glance (extended while buttons are used), plus short screen-off wakes about four times a day to sync time (ADR-0011). On USB power it stays always-on. The board cannot do better than about 0.25 mA in deep sleep (about 95 µA is hard-wired: a 100 kΩ resistor across the cell plus the LDO), and the MCU dominates glance energy at low backlight, so anything between deep sleep and fully awake (light-sleep standby, an always-on low-power face) would cut the 250 mAh cell to about a day. Targets: at least a week per charge (model: 11-13 days at 50 glances a day and backlight 6/16) and at most 0.5 s from button press to visible time.

USB presence is only measurable through the GPIO4 divider, and plugging USB in cannot wake the chip (the divider voltage is below the input-high threshold), so a plug-in during sleep takes effect at the next wake, either a button press or the next scheduled sync. A ULP coprocessor poll could make that instant later.

## Consequences

- Every wake is a cold boot of the application: wake-to-time depends on fast boot settings (no PSRAM memtest, no image validation on deep-sleep wake, quiet logs) and on the panel init (about 0.24 s of mandatory delays); a reset-free panel wake is the planned fallback if the measured time exceeds 0.5 s.
- The panel must receive SLPIN before every deep sleep (6 mA otherwise, and ghosting on matte panels), and every LCD pin is parked at a defined level.
- Unplugging USB can reboot the board, so time and state survive in RTC memory and NVS.
- Always-on mode shifts the face by 1-2 px every few minutes against image retention.
