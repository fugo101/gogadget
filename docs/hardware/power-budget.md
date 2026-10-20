# Power budget

How Gogadget spends the 250 mAh cell, how it enters and leaves deep sleep, what USB plug and unplug do, and how to check all of it without a multimeter.

- The hardware facts behind the numbers are in [t-display-s3.md](t-display-s3.md); the tests that check them are in [bring-up.md](bring-up.md).
- The design choices are in ADR-0007 (backlight), ADR-0009 (battery-first power model), ADR-0010 (2C charging) and ADR-0011 (NTP without an RTC).

Tags as in the board reference: [V] verified against the primary source, [I] inference or estimate, [C] correction.

## Energy model

The target is at least one week per charge. The model below gives about 12 days for 50 glances and 4 time syncs per day at backlight 6/16: about 11-13 days for the central inputs, 10.7-13.4 days at the extremes. Every input is an estimate until it is measured on the real board.

### Inputs

| Input | Value used | Range | Basis |
|---|---|---|---|
| Usable capacity | 212 mAh (85% of 250 mAh) | 85-90% | [I] The charger stops near C/5 for this cell, a few percent short of full ([t-display-s3.md](t-display-s3.md#battery-and-charging)), and below about 3.35 V VSYS the LDO is in dropout ([t-display-s3.md](t-display-s3.md#ldo-and-supply-limits)) |
| Sleep floor | 0.25 mA | 0.243-0.263 mA measured | Measured on LilyGO's bench (243 µA) and in #289 (263 µA). [I] Pin parking may bring it down to 0.12-0.15 mA ([t-display-s3.md](t-display-s3.md#sleep-current)) |
| Glances | 50 per day, 10 s each | - | Product decision. Activity extends a glance; the model uses the minimum |
| Wake and sleep overhead per glance | 0.6 s at 50 mA | 0.5-0.7 s, 40-60 mA | [I] Wake-to-time estimate 0.4-0.5 s, plus sleep entry |
| Awake current without backlight | 45 mA | 37-52 mA | [I] ESP32-S3 at 160 MHz, mostly idle: 27.6-42.3 mA [V ESP32-S3 DS Table 5-9]. Plus the panel 6.0 mA [V DS Table 3], the V3V overhead 0.5-0.9 mA and the AW9364 quiescent 0.33 mA. The only figure measured on this board, 147.7 mA awake in #289 (CPU clock, Wi-Fi state and backlight level not stated), would leave about 68 mA if the backlight was at 16/16 [I]; see [Notes](#notes) |
| Backlight | 5 mA per level: 30 mA at 6/16 | ±17.5% | [V AW9364 DS]. Battery current ≈ LED current through the linear LDO |
| Time syncs | 4 per day, 0.25 mAh each | 0.1-0.4 mAh each | [I] A Background wake with 3-8 s of Wi-Fi at 120-150 mA (#295). A weak v1.2 antenna lengthens it |

### Daily budget

Nominal inputs:

| Item | mAh/day | Share |
|---|---|---|
| Sleep floor: 0.25 mA x 24 h | 6.00 | 34% |
| Glances: 50 x 0.217 mAh | 10.83 | 61% |
| - MCU, panel and rails: 45 mA x 10 s x 50 | 6.25 | 35% |
| - Backlight at 6/16: 30 mA x 10 s x 50 | 4.17 | 23% |
| - Wake and sleep overhead: 0.6 s x 50 mA x 50 | 0.42 | 2% |
| Time syncs: 4 x 0.25 mAh | 1.00 | 6% |
| **Total** | **17.8** | 100% |

- Nominal: 212 mAh / 17.8 mAh per day ≈ **11.9 days**.
- Optimistic, 13.4 days: 37 mA awake, 0.243 mA floor, 0.1 mAh syncs, 0.5 s overhead at 40 mA.
- Pessimistic, 10.7 days: 52 mA awake, 0.263 mA floor, 0.4 mAh syncs, 0.7 s overhead at 60 mA.
- At 90% usable capacity, each figure grows by about 6%.

The MCU, not the backlight, is the largest awake cost at the default level. The glance length and the awake current matter most, the backlight level second, and the pixel clock hardly at all.

Cross-check: the knowledge base estimated 2.5-3.5 weeks for 20 glances and 1 sync per day. The same model gives 20 days (6.0 + 20 x 0.217 + 0.25 = 10.6 mAh/day), which agrees.

### Glance cost per backlight level

Nominal inputs (45 mA without backlight, 0.6 s overhead at 50 mA, 50 glances, 0.25 mA floor, 4 syncs at 0.25 mAh, 212 mAh usable):

| Level | Backlight | Awake current | mAh per glance | Glances, mAh/day | Total, mAh/day | Days per charge |
|---|---|---|---|---|---|---|
| 0 (off) | 0 mA | 45 mA | 0.133 | 6.7 | 13.7 | 15.5 |
| 1 | 5 mA | 50 mA | 0.147 | 7.4 | 14.4 | 14.8 |
| 2 | 10 mA | 55 mA | 0.161 | 8.1 | 15.1 | 14.1 |
| 4 | 20 mA | 65 mA | 0.189 | 9.4 | 16.4 | 12.9 |
| **6 (default)** | 30 mA | 75 mA | 0.217 | 10.8 | 17.8 | **11.9** |
| 8 | 40 mA | 85 mA | 0.244 | 12.2 | 19.2 | 11.0 |
| 10 | 50 mA | 95 mA | 0.272 | 13.6 | 20.6 | 10.3 |
| 12 | 60 mA | 105 mA | 0.300 | 15.0 | 22.0 | 9.6 |
| 16 | 80 mA | 125 mA | 0.356 | 17.8 | 24.8 | 8.6 |

### Sensitivities

Around the nominal point:

- One backlight level: +0.69 mAh/day, about -0.45 day.
- Ten more glances a day: +2.2 mAh/day, about -1.3 days. 100 glances a day give about 7.4 days.
- One more sync a day: +0.25 mAh/day.
- 10 mA more awake current: +1.4 mAh/day, about 11.0 days.
- A 0.15 mA floor (if pin parking recovers about 100 µA): 13.7 days. A 0.12 mA floor: 14.4 days.
- The 17.5 MHz/256 RTC clock source (+5 µA): +0.12 mAh/day, negligible.
- Pure deep sleep with no wakes: about 35 days at 0.25 mA.

Faults show up as a collapse of the runtime, which makes them easy to spot in a drain test:

| Fault | Extra current | Days per charge |
|---|---|---|
| Panel not in SLPIN | ~6 mA | ~1.3 |
| GPIO15 HIGH (or its pull-up on) in sleep | 0.5-0.9 mA | ~6 |
| BOOT stuck or held | 0.33 mA | ~8.2 |
| GPIO17 or GPIO18 driven LOW in sleep | 0.33 mA each | ~8.2 |

### Levers not in the plan

[I] These are noted for later; none is decided:

- Lower the CPU clock, or allow automatic light sleep between the 1 Hz redraws of a glance. The MCU is about 35% of the budget. This needs `lvgl_port_stop()`/`lvgl_port_resume()`, and the USB-Serial-JTAG console does not work in light sleep (no host is attached on battery anyway).
- Board mods: remove R12 (about 40 µA, purpose unknown) or the green LED (0.25-0.6 mA while V3V is on).

## Deep-sleep entry and exit

The settled sequence (ADR-0009), with the reason for each step. Panel rules are in [t-display-s3.md](t-display-s3.md#panel-timing-rules); backlight rules are in [t-display-s3.md](t-display-s3.md#backlight-aw9364).

### Entry sequence

Before step 1:

- Call from a task whose stack is in internal RAM. Digital-pad holds require it [V].
- Keep LVGL from starting a flush (hold the port lock). The SLPIN `tx_param()` drains any queued colour DMA [V].
- Stop Wi-Fi (`esp_wifi_stop()`) if it was started.
- Make sure a new OTA image has already marked itself valid. Deep-sleep wakes skip image validation and rollback.
- Save the clock state to RTC memory.

Steps:

1. **Backlight off.** GPIO38 LOW, then wait at least 3 ms (the AW9364 shuts down within 0.8-2.5 ms). Invalidate the cached level.
   - Use `esp_rom_delay_us(3000)`. `vTaskDelay()` can return one tick early, and `pdMS_TO_TICKS(3)` is 0 at IDF's default 100 Hz tick ([t-display-s3.md](t-display-s3.md#panel-timing-rules)).
2. **SLPIN.** `esp_lcd_panel_io_tx_param(io, 0x10, NULL, 0)`, then wait at least 5 ms (with the same care about ticks). `esp_lcd_panel_disp_sleep(panel, true)` is equivalent but waits a fixed 100 ms.
   - If the last SLPOUT was less than 120 ms ago, wait until 120 ms have passed first.
   - SLPIN cuts the panel from about 6 mA to about 20 µA and prevents the #300 ghosting.
3. **GPIO15 LOW**, pulls disabled, `gpio_hold_en(GPIO_NUM_15)`. This switches off V3V, the green LED and the divider on battery.
   - Never `gpio_reset_pin(GPIO_NUM_15)`: it enables the pull-up, which turns V3V on [V gpio.h, #51].
4. **Park the LCD pins** (table below). Call `gpio_hold_en()` on each, then `gpio_deep_sleep_hold_en()` once for the digital pads. GPIO38 is not held.
5. **Wait until GPIO0 and GPIO14 both read HIGH for at least 50 ms.** ext1 is level-triggered, and GPIO14's 100 kΩ/100 nF needs about 14 ms after release to reach the input-high level [I].
6. **Arm the wakes.** `esp_sleep_enable_ext1_wakeup_io(BIT64(GPIO_NUM_0) | BIT64(GPIO_NUM_14), ESP_EXT1_WAKEUP_ANY_LOW)`, plus `esp_sleep_enable_timer_wakeup()` for the next scheduled sync.
   - No internal pulls: both buttons have board pull-ups to the always-on rail, so the RTC peripherals can power down [V esp_sleep.h].
7. **Sleep.** `esp_deep_sleep_disable_rom_logging()`, then `esp_deep_sleep_start()`.

### Pin parking

| Pin | Function | Level in sleep | Hold | Why |
|---|---|---|---|---|
| GPIO15 | PWR_EN | LOW, pulls off | `gpio_hold_en` (RTC pad) | V3V, green LED and divider off on battery. A pull-up would turn them on |
| GPIO5 | LCD RST | HIGH | `gpio_hold_en` | Panel stays out of reset, so registers and GRAM survive (needed for a later reset-free wake) |
| GPIO6 | LCD CS | HIGH | `gpio_hold_en` | Panel deselected: nothing on the bus can clock in |
| GPIO7 | LCD DC | LOW | `gpio_hold_en` | Defined level |
| GPIO8 | LCD WR | HIGH | `gpio_hold_en` | Idle level, no strobe edge |
| GPIO9 | LCD RD | HIGH | `gpio_hold_en` | No read cycle: the panel never drives D0-D7 |
| GPIO39-42, 45-48 | LCD D0-D7 | LOW | `gpio_hold_en` + `gpio_deep_sleep_hold_en` (digital pads) | Defined inputs. Never HIGH on GPIO45/46 (strapping) |
| GPIO38 | Backlight EN | LOW, not held | none | AW9364's internal 150 kΩ pull-down keeps it off, and V3V is off on battery |
| GPIO0, GPIO14 | Buttons | inputs, pulls off | ext1 wake | Board pull-ups to VDD3V3 |
| GPIO16, 17, 18 | Board pull-ups | untouched (inputs) | none | Driving them LOW costs 33/330/330 µA (#313) |
| GPIO4 | ADC | analog input | none | Divider unpowered on battery |
| GPIO43, 44 | UART0 | untouched | none | ROM output on deep-sleep wakes is disabled by API |
| GPIO1-3, 10-13, 21 | free | untouched, or as an add-on requires | - | - |

- [I] Every LCD control pin is an RTC pad, so CS stays held HIGH through the wake. Whether D0-D7 stay held between the wake and the release calls is not documented; with CS HIGH it does not matter to the panel.
- [I] The #300 reporter's alternative parks every LCD pin LOW, which keeps the panel in reset. Compare both on a matte unit ([bring-up.md](bring-up.md#display), DSP-6).

### Exit sequence

On every boot, not only after a deep-sleep wake (holds also survive watchdog and software resets [V gpio.h]):

1. **Restore pin levels before releasing any hold.** Configure each parked pin as an output at its parked level first. A pin released while it is still in its default input mode stops driving its held level [V gpio.h, `gpio_hold_dis()`]; RST or CS would then float or glitch.
2. **Read the wake cause.** Use `esp_sleep_get_wakeup_causes()` (a bitmap) and `esp_sleep_get_ext1_wakeup_status()`. The button that woke the board starts in the DOWN state, and its press is consumed (ADR-0008).
3. **USB test, before raising GPIO15.** GPIO15 has been LOW through the whole sleep, so GPIO4 has settled: about 0 V on battery, about 2.3 V on USB [V SCH].
4. **VSYS.** Release GPIO15's hold, drive it HIGH, wait 3-5 ms, then read VSYS (battery only), before any backlight or Wi-Fi load ([t-display-s3.md](t-display-s3.md#gpio4-sensing)).
5. **Display.** Release the LCD holds (`gpio_hold_dis()` per pin, then `gpio_deep_sleep_hold_dis()`) and keep RD HIGH. Then run the full panel bring-up (ADR-0006):
   1. bus, IO and panel;
   2. reset and init;
   3. invert, gap and CMD_1;
   4. first frame (`lv_refr_now()`);
   5. DISPON.
6. **Backlight on, last.** EN HIGH, wait at least 30 µs (level 16), then 10 pulses down to the default level 6.

[I] Expected wake-to-time is 0.4-0.5 s, of which about 0.24 s is the panel's fixed delays. The reset-free wake ([t-display-s3.md](t-display-s3.md#reset-free-wake)) saves about 0.23 s if the measurement exceeds 0.5 s.

### Background wakes

A Background wake (about 4 a day) runs a Sync with the screen dark:

- **The panel stays in Sleep In.** Leave the LCD pins parked and held, and never send SLPOUT; GRAM keeps the last frame.
- **VSYS check before Wi-Fi.** Raise GPIO15 for about 5 ms to read VSYS, then lower and re-hold it; the green LED blinks briefly. Skip Wi-Fi below about 3.6 V VSYS (ADR-0011).
- **Going back to sleep.** Steps 5-7 of the entry sequence, with GPIO15 held LOW again. Steps 1 and 2 are already done.

## USB plug and unplug

What the hardware does in each case ([t-display-s3.md](t-display-s3.md#switched-rails)):

| Event | Hardware | Firmware |
|---|---|---|
| Plug in, awake on battery | VBUS feeds VSYS through D3 (about 4.6 V); Q4 turns off; VBUS forces V3V on (green LED); the TP4065 starts charging (red LED). [I] No reset expected: VSYS rises | The next USB test (GPIO15 LOW, GPIO4 about 2.3 V) sees it; switch to always-on and sync time |
| Plug in, asleep | V3V and the divider power up (green LED lights); the backlight stays off (EN pulled down). The chip stays asleep: GPIO4's 2.3 V is below VIH, and no other VBUS line exists | Noticed at the next wake: a button press or the next scheduled Background wake (product decision; no extra polling). A ULP poll of GPIO4 is a later option |
| On USB, always-on | The load runs from VBUS; the TP4065 charges, then tops up at 80-90%. GPIO15 has no effect; the VSYS reading says nothing about the cell | Show "on USB" only. Save the epoch to NVS every minute. Backlight off only via EN |
| Unplug, awake | VBUS decays. [I] Until Q4 turns on, VSYS is fed through Q4's body diode (about VBAT - 0.6 V), so the LDO can drop out, worse under Wi-Fi peaks. LilyGO: "During the switching process, it will restart" (#174) | Log `esp_reset_reason()` on every boot. After an unexpected reset, show the time from RTC memory or the NVS epoch with the "unsynced" badge, and resync when possible |
| Unplug, asleep | V3V and the divider switch off again | Nothing |

- [I] **Polling in always-on mode.** A periodic USB test needs a short GPIO15-LOW dip, about 5 ms. With USB present, V3V is held up by VBUS and the dip is invisible. After an unplug, the first dip blanks the backlight and the AW9364 loses its level, so invalidate the cached level, as for any V3V loss.
- [I] **Does the time survive a reset?** It depends on the reset type.
  - IDF's RTC timer persists across all resets except power-on resets [V IDF System Time docs].
  - On the S3, IDF v6.1 programs the brownout detector for a hardware System Reset [V esp_hal_pmu/brownout_hal.c], and the TRM says a System Reset also resets the RTC domain. A brownout therefore probably loses the time, and a supply collapse certainly does.
  - So after `ESP_RST_BROWNOUT` or `ESP_RST_POWERON`, treat the clock as lost even if RTC memory says otherwise, and recover from the NVS epoch. The bring-up test records what actually happens ([bring-up.md](bring-up.md#usb-plug-and-unplug)).
- **Cables.** The schematic shows no CC resistors, so a USB-C-to-C charger may not supply VBUS at all. LilyGO ships a USB-A-to-C cable.

## Verifying power without a multimeter

There is no meter, so each check uses a quantity the firmware or a phone can observe. Gross faults (missing SLPIN, GPIO15 left HIGH, a stuck button) show up within a night. Absolute sleep current within ±10% needs a meter; buy one with a µA range, or a power profiler on the battery lead, if a number must be exact.

### Firmware voltage log

Each wake appends one record to a small log that `log.dump` prints over USB. Its storage (an RTC-memory ring flushed to flash) is a firmware decision.

- RTC timestamp and whether the clock is synced.
- Wake cause and `esp_reset_reason()`.
- USB present (GPIO4 with GPIO15 LOW).
- VSYS in mV: no load, 3-5 ms after GPIO15 HIGH, 16-32 samples averaged, x2.
- VSYS just after the backlight reaches its level, and that level (for the R10 check below).
- Time awake in ms.
- On Background wakes: Wi-Fi attempts, RSSI and duration; NTP offset in ms; seconds since the previous Sync.

With a timestamp and VSYS for every wake, a discharge curve falls out of normal use.

### Idle-drain test

Measure on battery only. On USB, Q4 is off and VBUS powers everything, so a drain test on USB measures nothing.

1. **Gross check, one night.**
   - Charge until the red LED goes out, unplug, and let the cell rest about 1 h (it relaxes by tens of mV after charging).
   - Run a test build that stays in deep sleep and wakes once an hour only to log VSYS (no backlight, no Wi-Fi, under 1 s awake).
   - After 10-12 h, the VSYS decline must match well under 1 mA average. At 0.25 mA, 12 h use 3 mAh, about 1% of the cell. A 6 mA fault would use about 70 mAh, a drop of several hundred mV.
   - Also check by eye: on battery in deep sleep, the green LED must be off. If it glows, V3V is on (GPIO15 hold or pull-up wrong; +0.5-0.9 mA) [V SCH].
2. **Endurance run, the real acceptance.**
   - From a full charge, run the normal duty (glances and scheduled syncs) and log every wake.
   - Record the days until VSYS (no load) first reads below 3.6 V, the Wi-Fi cut-off. Compare with the model above.
   - [I] Below 3.6 V there is still roughly 5-10% of the capacity left, so this end point is slightly conservative.
3. **Calibrated decline, optional [I].**
   - Record one discharge curve at a near-constant load: awake, backlight 16/16, Wi-Fi off, about 125 mA nominal, VSYS logged every 30 s. It takes about 1.5-2 h from full to 3.6 V. This gives VSYS as a fraction of the delivered charge for this cell.
   - Later declines in a sleep-only run can then be read as a fraction of the capacity per day.
   - The absolute current is still only as good as the 125 mA estimate, about ±20%. That is enough to tell 0.12, 0.25 and 0.6 mA apart.

Keep the temperature roughly constant across a test: the cell voltage and the RC clock both depend on it.

### RTC drift against NTP

Each sync measures the clock error for free.

- Before setting the clock, log offset = t_ntp - t_rtc, the seconds since the previous sync, and the RTC clock source. Drift in ppm is offset / elapsed x 10^6.
- The same numbers feed the drift learning (ADR-0011) and the 24 h test that chooses between the default RC and the 17.5 MHz/256 source ([bring-up.md](bring-up.md#rtc-drift)).
- NTP over Wi-Fi is accurate to tens of ms, so a 6 h interval resolves drift to a few ppm.
- Budget: ±1 min over a 6 h sync interval is 2,800 ppm of raw drift. Over the 24 h after which the "unsynced" badge appears, it is 690 ppm.

### R10 health from the VSYS sag

On battery, VSYS = Vcell - I x R_path, where R_path is the cell's internal resistance plus its protection FETs, R10 and Q4. A degraded R10 (5-45 Ω in #291, about 350 Ω in #223) raises R_path a lot.

1. On a battery glance, sample VSYS (16 samples) just before the backlight turns on.
2. Sample again 3-5 ms after it reaches its level. The divider settles in about 3 ms, and the cell's slower polarisation is still small.
3. R_path ≈ ΔVSYS / ΔI, with ΔI ≈ 5 mA x level + 0.33 mA (AW9364 quiescent). Keep Wi-Fi off and the CPU load the same for both samples.
4. [I] Expected R_path is about 0.3-0.8 Ω on a healthy board:
   - At level 6 (ΔI ≈ 30 mA), ΔVSYS is 10-25 mV, near the ADC's resolution after averaging.
   - At 16/16 (ΔI ≈ 80 mA), it is 25-65 mV.
   - A degraded R10 at 5 Ω gives 150 mV at level 6, and 45 Ω gives about 1.3 V. Both are unmistakable.
5. Flag the battery path when R_path exceeds about 2 Ω [I], and log the trend.
   - If the level-6 step is too noisy, run a maintenance check that briefly steps to 16/16 while a frame is shown.
6. A first sanity check right after a full charge: the first battery VSYS reading should be about 4.1-4.2 V. A much lower value points to ADC calibration or to R10. Boot loops on battery while USB works also point to R10 (#291).

## Open questions

- What the real floor is with Gogadget's parking, against the 243-263 µA references. Without a meter, only the drain tests above bound it.
- Whether unplugging USB resets the board under IDF v6.1, with which reset reason, and whether the RTC time survives (#174).
- The real per-sync cost on a v1.2 antenna. It may be well above the 0.25 mAh used here.
- Whether the AW9364's top levels compress at V3V ≈ 3.3 V, which would make levels above about 12 cost less than modelled.

## Notes

- **Sync cost.** ADR-0011 quotes about 0.4 mAh/day for time syncs (3 s at 120 mA each). The knowledge base estimates 0.2-0.4 mAh per sync. This model uses 0.25 mAh per sync (1.0 mAh/day); the difference is about half a day of runtime.
- **Awake current.** The model's 45 mA comes from datasheet figures. The knowledge base's only on-board figure is 147.7 mA awake (#289), of which about 80 mA is likely the backlight at 16/16 [I]. If the remaining ~68 mA applied to Gogadget's 160 MHz build, the nominal case would drop to about 10.1 days, still above the one-week target. The [calibrated decline](#idle-drain-test) and the time awake in the [voltage log](#firmware-voltage-log) can bound it without a meter.
- **USB polling.** An earlier proposal added a 15-minute timer check for USB plug-in. The product decision is no extra polling: a plug-in during sleep is noticed at the next button press or Background wake.
- **Backlight default.** Research proposed level 4/16 on battery; the decision is 6/16. The table above shows the cost of each level.
