# Keep time with NTP about four times a day, without an external RTC

The board cannot take a 32 kHz crystal (its pins are GPIO15 and GPIO16, both used), so deep-sleep time runs on an internal RC oscillator that drifts with temperature. Gogadget accepts ±1 minute: a screen-off timer wake syncs with NTP about four times a day (about 0.4 mAh/day), the firmware learns the drift between syncs and corrects for it, an "unsynced" badge appears after 24 hours without a sync, and Wi-Fi is not started below about 3.6 V VSYS because the 500 mA LDO browns out under Wi-Fi peaks. Time also syncs on every USB session. The time zone is fixed UTC+7 (Vietnam has no DST), so core converts epoch to local time itself instead of relying on TZ strings.

## Considered options

- **External TCXO RTC** (for example RV-3032-C7 on I2C 17/18, about 160 nA): ±1 s, but extra hardware; kept as the upgrade path if drift becomes annoying.
- **Sync only on demand**: no background Wi-Fi, but the clock would drift unnoticed.

## Consequences

- NTP servers are fixed (`1.ntp.vnix.vn`, `pool.ntp.org`); DHCP-supplied servers are ignored.
- The RTC clock source (default RC vs the 17.5 MHz/256 source, about +5 µA) is chosen after a 24-hour drift measurement against NTP.
