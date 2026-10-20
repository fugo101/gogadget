# Roadmap

Features arrive one at a time; each milestone ships with golden screenshots for its screens and an on-device check.

## v0.1: Watchface and foundation

- Repository, CI (firmware build, host tests, golden screenshots, secrets check), simulator.
- Board bring-up: display (ADR-0006), backlight (ADR-0007), buttons (ADR-0008), dev console (ADR-0016).
- Watchface v1 (minimal variant, docs/ui/design.md): 88 px seven-segment HH:MM with a seconds line, Solar date with Vietnamese weekday and Lunar date ("Thứ Ba 20/10 · 11/9 ÂL"), Status bar.
- Power model (ADR-0009): Standby, Glance, Always-on on USB, battery level, fast wake.
- Wi-Fi networks via the console and NTP Sync four times a day (ADR-0011, ADR-0012).
- Lunar calendar core with generated tables and CI checks (ADR-0015).
- Partition table with OTA slots and rollback marking (ADR-0013); the OTA update path itself can follow in v0.1.x.

## v0.2: Pomodoro and timers

Countdown and Pomodoro with a visual alert; a timer wakes the device from Standby when it expires.

## v0.3: Calendar

Month view with Solar and Lunar dates, Can Chi, leap months and Vietnamese holidays.

## v0.4: BLE provisioning

Provisioning boot (hold KEY at power-up), `network_provisioning` with NimBLE and Security 2, a QR code for the iPhone app (ADR-0012).

## Later

- Weather (low priority).
- Settings and System info grow with each milestone (brightness, glance length, orientation, Wi-Fi list, firmware version, battery and reset history).
- Optional hardware: a TCXO RTC on I2C for ±1 s time, a passive piezo on GPIO21 for audible alerts, ULP polling for instant Always-on when USB is plugged in.

## Not planned

Tailscale or any VPN, touch input, Arduino or PlatformIO builds.
