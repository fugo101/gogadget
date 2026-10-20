# Power and sleep budget (T-Display-S3 v1.2)

Date: 2026-10-20

Method: multi-agent research workflow: one research agent, then an adversarial verifier that re-checked each critical claim and the recommendation against primary sources

Status: verified: 7 confirmed / 2 partially / 0 refuted; the verifier says the recommendation holds

Notes: the settled decisions use a 250 mAh cell (target >= 1 week; their model gives about 11-13 days at 50 glances/day, 4 syncs/day and backlight 6/16), turn the backlight off before SLPIN (the recommendation lists SLPIN first), and skip Wi-Fi below about 3.6 V VSYS (the verifier suggested about 3.5 V).

Superseded by decision: the USB test is GPIO15 LOW, then GPIO4 at about 0 V on battery or about 2.3 V on USB, with a VSYS threshold only as a secondary hint (not the 4.4 V heuristic in section 3); the sleep sequence sends SLPIN without DISPOFF and parks every LCD pin, not only RST and CS; the wake-to-time baseline is 0.4-0.5 s with a full panel init, not 250-400 ms ([power-budget.md](../hardware/power-budget.md)).

## Research report

### 1. Power tree and current draw

From the [schematic, 2024-04-29](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf). LilyGO says v1.2 changed only the charger ([#236](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/236#issuecomment-2082060198)).
- **Always on:** VBAT → Q4 → AP2112K/RT9013 LDO (EN tied high) → VDD3V3. This rail feeds the S3, the **ST7789 supply (FPC pin 2)**, the header 3V pins ([#352](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/352)) and the pull-ups (GPIO0 10k, GPIO14 100k, GPIO17/18 10k).
- **Switched V3V (Q1/Q3, turned on by GPIO15 OR VBUS):** backlight LED anodes, AW9364 VIN and the green LED (2 kΩ). Q2 switches the GPIO4 divider. GPIO15 does **not** power the panel.
- **Fixed floor:** LDO Iq 55 µA (80 max, [AP2112](https://www.diodes.com/assets/Datasheets/AP2112.pdf)) + R12 100 kΩ across VBAT (~40 µA) + ST7789 sleep-in 20 µA ([spec §7.3](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/datasheet/ST7789V_SPEC_V1.4.pdf)) + S3 7–8 µA ([datasheet Tab. 5-10](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)) ≈ **0.12 mA**.

| State | Current | Basis |
|---|---|---|
| Deep sleep, GPIO15/BL low, panel not in SLPIN | 6.42 mA | measured, [#289](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/289) |
| …with panel SLPIN | 0.263 mA | measured, #289 |
| Awake, full backlight | 147.7 mA | measured, #289 |
| Deep sleep, GPIO15 held high | floor + ~0.2–0.7 mA | derived |
| Light sleep, display off | ~0.5–0.6 mA (240 µA + 140 µA PSRAM + floor) | datasheet |
| Display on | +6 mA panel + 0.33 mA AW9364 + 5 mA × level | datasheets |
| Wi-Fi | RX 88–91 mA, TX peaks 283–340 mA | datasheet |

UNVERIFIED: no one has published measurements of this board at each backlight level or with Wi-Fi on.

**Pitfalls and fixes:**
- Send DISPOFF+SLPIN before sleep. Skipping it costs ~6 mA and causes ghosting ([#300](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/300)).
- Drive BL low ≥2.5 ms before GPIO15 goes low. Never hold GPIO15 or GPIO38 high.
- Hold RST and CS high instead of leaving the bus floating.
- Don't drive or pull down GPIO0/14/16–18 (they have external pull-ups).
- Use EXT1, not ext0.
- Measure on battery with USB unplugged, because VBUS forces V3V on.
- Set `SPIRAM_MEMTEST=n`.
- Call `esp_deep_sleep_disable_rom_logging()`, which saves ~10 ms per wake ([esp-idf#15921](https://github.com/espressif/esp-idf/issues/15921)).
- Never select the external 32k crystal option, because that pin is GPIO15.

### 2. Backlight

**Verified.** Commit [02f0e9d](https://github.com/Xinyuan-LilyGO/T-Display-S3/commit/02f0e9dc4b) (2026-03-09, 'Remove invalid backlight control methods') replaced ledc PWM with `digitalWrite(38,HIGH)`. The maintainer had already called ledc 'wrong' ([#278](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/278#issuecomment-2367079609)).

The [AW9364 datasheet](https://doc.awinic.com/doc/202402/d1a46d65-a45d-494f-bfc9-3d6a6a93a860.pdf) explains why:
- The n-th EN rising edge sets 20−1.25(n−1) mA per channel. There are 16 steps and the count wraps.
- Pulses need a low time of 0.5–500 µs and a high time of ≥0.5 µs.
- EN low for more than 2.5 ms shuts the chip down.
- kHz PWM therefore steps the counter instead of dimming.

**Correct method:** EN high, wait 30 µs, then n−1 low pulses of ~2 µs inside a critical section (or with RMT). LilyGO's [factory setBrightness](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/examples/factory/factory.ino) does this. With four LEDs the current is ≈ **5 mA × level** (level 16 → 80 mA, level 1 → 5 mA). These numbers come from the datasheet.

### 3. Charging

- The charger is a TP4065 with R13 = 2 kΩ, which gives ≈500–525 mA ([datasheet](http://toppwr.com/uploadfile/file/20230304/6403023707f41.pdf)).
- CHRG drives only the red LED and VBUS reaches no GPIO. There is no charge or USB-present input and no wake on plug-in.
- With USB connected, GPIO4 reads the 5 V rail (~4.6 V), not the cell ([#190](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/190), [#254](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/254)). Treat a reading above 4.4 V as a USB heuristic only.
- On battery, the reading needs GPIO15 high. Use calibrated, averaged reads with the backlight off.
- There is no battery protection IC on the board, so use a protected cell.

### 4. Deep vs light sleep

**Deep-sleep wake:**
- ROM to stub takes 2.75 ms (#15921).
- Turn on `BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP`, set logs to WARN and turn memtest off. Memtest costs ~1 s per 4 MB ([IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html#improving-startup-time)).
- The stock ST7789 driver waits 100 ms after SLPOUT ([source](https://github.com/espressif/esp-idf/blob/master/components/esp_lcd/src/esp_lcd_panel_st7789.c)). The spec requires 120 ms after a reset, but only 5 ms when waking from a retained sleep-in.
- Estimated time to the first LVGL frame (UNVERIFIED): ~250–400 ms with a full re-init, ~120–180 ms if RST and CS are held.

**Light sleep:**
- Standby current is ~2–5× the deep-sleep floor.
- Auto light sleep works with i80. The driver holds `ESP_PM_NO_LIGHT_SLEEP` only during transfers ([v5.5](https://github.com/espressif/esp-idf/blob/release/v5.5/components/esp_lcd/i80/esp_lcd_panel_io_i80.c)), and the panel refreshes from its own GRAM.
- It needs `lvgl_port_stop()/resume()`, because the port's 5 ms tick otherwise wakes the chip ([esp_lvgl_port](https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port#power-saving)).
- Keep `PM_SLP_DISABLE_GPIO` off.

### 5. Timekeeping

- **No 32 kHz crystal:** the XTAL_32K_P/N pins are GPIO15 (PWR_EN) and GPIO16.
- The RC clock drifts with temperature and has no ppm spec ([System Time](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/system_time.html)).
- Each deep-sleep interval is timed with a 10-cycle calibration taken when the chip enters sleep ([sleep_modes.c](https://github.com/espressif/esp-idf/blob/master/components/esp_hw_support/sleep_modes.c)).
- An ESP32-C6 RC was measured at −1540 ppm/°C ([#19248](https://github.com/espressif/esp-idf/issues/19248)). Expect ~10²–10³ ppm, about 9–90 s/day (UNVERIFIED on S3).
- The 8MD256 clock option costs +5 µA; test it on the bench before relying on it ([#9455](https://github.com/espressif/esp-idf/issues/9455)).

**Sync cadence:**
- ±1 min: a sync every 6 h tolerates up to 2,800 ppm.
- ±1 s: needs syncs hourly or more often, so add an I2C TCXO RTC instead.

### 6. Wake sources

- GPIO0 and GPIO14 are both RTC IOs, so one EXT1 `ANY_LOW` mask covers both buttons with RTC_PERIPH off ([sleep modes](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html)).
- A deep-sleep wake is a Core Reset, and strapping pins are latched only at Chip Reset ([TRM Tab. 7.1-1](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)). BOOT is therefore safe as a wake key.
- Use the RTC timer for alarms.
- RTC memory is 8 KB SLOW + 8 KB FAST. It is lost on power-on or RST.

### 7. Battery model

Assumptions:
- 85% of capacity is usable.
- Deep-sleep floor is 0.20 mA.
- A glance draws 40 mA (MCU) + 6.7 mA + backlight, plus a 0.15 s boot.
- Each Wi-Fi sync is 3 s at 120 mA.

| Mode | mAh/day | 500 | 1000 | 1500 mAh |
|---|---|---|---|---|
| (a) Glance, BL 10 mA | 13.2 | 32 d | 64 d | 97 d |
| (a) Glance, BL 40 mA | 17.4 | 25 d | 49 d | 74 d |
| (a) Glance, BL 80 mA | 22.9 | 19 d | 37 d | 56 d |
| (a) BL 40 mA, light-sleep standby | 25.8 | 16 d | 33 d | 50 d |
| (b) Always-on, BL 5 mA | 295 | 1.4 d | 2.9 d | 4.3 d |

## Recommendation

Use deep sleep as the only standby state. Wake on EXT1 ANY_LOW for GPIO0|GPIO14, plus an RTC-timer wake for the next alarm, Pomodoro or NTP slot. Before each sleep send DISPOFF+SLPIN, drive the backlight enable low and wait ≥3 ms, drive GPIO15 low and hold RST/CS high. Boot fast (skip validation on deep-sleep wake, logs at WARN or lower, SPIRAM memtest off, ROM log off) and keep the 10 s glance as a plain active window. Add esp_pm DFS or auto light sleep only if measurements show it is worth it. Dim with AW9364 pulse counting, never PWM, and default to a low level, because the backlight (about 5 mA per step) dominates glance energy. Keep the internal RC clock (or bench-test 8MD256) and sync NTP 4×/day with a stored drift correction for ±1 min; if ±1 s is required, add an I2C TCXO RTC, since the 32 kHz pins are already in use. Offer always-on only as a USB-powered desk mode (light sleep; about 1.5–4 days on battery). After an OTA update, reboot with esp_restart() and mark the app valid before the first deep sleep, because skip-validate deep-sleep wakes bypass the rollback check.

## Open questions

- No published board-level measurements exist for current at each AW9364 level, energy per Wi-Fi/NTP sync, or light sleep with the display on or off. All of these are datasheet-derived and need a battery-side measurement (e.g. PPK2) with USB unplugged.
- The measured deep-sleep current (263 µA in #289) is about twice the bottom-up estimate (~120 µA). Still unknown: whether R12 (100 kΩ across VBAT) is populated on v1.2, which LDO is fitted (AP2112K or RT9013), and whether floating LCD inputs or the ext0/RTC_PERIPH setup inflated that measurement.
- The orientation of Q2's body diode is unclear, so it is unknown whether the GPIO4 divider leaks ~17 µA from the 5 V rail while GPIO15 is low. The current of the green LED8 is also unknown because its part and Vf are not published.
- The real deep-sleep drift of the ESP32-S3's RC and 8MD256 clocks (ppm, and ppm/°C) has not been characterized. It is also unknown whether the 8MD256 deep-sleep timing problem (esp-idf #9455) is fixed for S3 in v5.5.x or v6.x.
- The actual wake-to-first-frame time on this board is unmeasured. It is also unknown whether holding RST/CS with gpio_hold across deep sleep reliably preserves ST7789 registers and GRAM (which would skip the 120 ms reset path), and whether releasing the hold causes glitches.
- The panel part number and FPC pinout are not published. The roles of pin 2 (VDD3V3 supply) and pin 20 (V3V LED anode) are inferred from the schematic and #289.
- Product gap: the board has no buzzer or vibration motor, so a Pomodoro or alarm firing while the device is in a pocket can only light the screen unless extra hardware is added.

## Critical claims and verification

### C1: confirmed

**Claim:** GPIO15 (diode-ORed with VBUS, via Q1/Q3) switches only V3V: the backlight LED anodes, AW9364 VIN and green LED8. Through Q2 it also switches the GPIO4 divider. The ST7789 supply (FPC pin 2) and the header 3V pins are on the always-on LDO rail VDD3V3, so the panel must be put to sleep with commands (SLPIN), not by power-gating.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf ; maintainer confirmation https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/352

**Why it matters:** Sets the required pre-sleep sequence and explains why GPIO15 LOW alone leaves ~6 mA and causes ghosting.

**Correction:** Minor: the README pinout table still labels GPIO15 'LCD power control'. It actually gates only backlight power, LED8 and the ADC divider.

**Evidence:** Schematic T_Display_S3.pdf (Altium, dated 2024-04-29, repo commit b0a5ecd796), read at high zoom. PWR_EN (SoC pin 21, XTAL_32K_P = GPIO15) and VBUS are ORed through 1N4148 diodes D1/D2 into R5 10k, which drives the base of the PMBT3904 Q1 (R7 100k to GND). Q1's collector, through R6 1k, pulls down the shared gate node of two SI2301s, Q2 and Q3 (R8 100k pull-up to the VIN/'+5V' rail). Q3 switches VDD3V3 to V3V. V3V feeds only three things: CN1 pin 20 (LED anode, with C20), AW9364 VIN (with CW1) and green LED8 through 2k. Q2 connects the VIN rail to net BAT, which is the top of the R2/R4 100k/100k 1% divider (BAT_ADC = GPIO4). CN1 pin 2, the panel's only supply pin, is VDD3V3 (C17 1uF, C18 100nF). P1-1, P2-12 and CNC-2 are VDD3V3; P2-1 is +5V/VIN. The LDO U7 has EN pulled up to VIN through R11 100k, so it is always on. In #352 the maintainer wrote (comment 6137349539, 2026-10-13): 'The pin header is connected directly to the LDO output and cannot be controlled'. README commit 1cc41eb1c7 on the same day removed 'V3V controls the V3V output of the header pins and provides power to the screen'. #289 (6.4 mA with GPIO15 and BL low) and #300 (image retention after deep sleep) fit a panel controller that stays powered.

### C2: partially

**Claim:** Measured on this board: 6.42 mA in deep sleep with GPIO15 and the backlight low but the panel not in sleep-in; 263 µA after adding ST7789 SLPIN (0x10); 147.7 mA awake.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/289

**Why it matters:** This is the only board-level measurement, and it anchors the standby floor in the battery model.

**Correction:** The figures are quoted correctly, but they come from one anecdotal user report, not a measurement known to be on a v1.2 non-touch board. #289 does not give the board revision, touch or non-touch, the supply path (battery or USB) or the meter. The test used ext0, which keeps RTC_PERIPH powered, and left the i80 bus pins unheld. That SLPIN (0x10) was sent is inferred from the linked example; the user never says so. Treat 0.263 mA as the only empirical floor and re-measure it on the target board.

**Evidence:** #289 (opened 2024-11-26) contains the code comment 'draws 6.42mA asleep and 147.7mA awake'. That test used ext0 on GPIO14, drove GPIO15 and GPIO38 low, did not send SLPIN and did not hold any LCD pin. The maintainer replied 'the screen needs to be set to sleep', two minutes after commit 4b2edc1e5d (2024-11-27) added examples/Sleep, which sends tft.writecommand(0x10). The user then reported 'now drawing 263 uA in deep sleep'.

### C3: confirmed

**Claim:** Floor components: AP2112 Iq 55 µA typ / 80 µA max; R12 100 kΩ from VBAT to GND (~40 µA); ST7789V sleep-in ~20 µA typ; ESP32-S3 deep sleep 7–8 µA. ESP32-S3 light sleep is 240 µA plus 140 µA for 8 MB octal PSRAM.

**Source:** https://www.diodes.com/assets/Datasheets/AP2112.pdf ; https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf (Table 5-10) ; https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/datasheet/ST7789V_SPEC_V1.4.pdf (§7.3)

**Why it matters:** Puts deep sleep at ~0.12–0.26 mA versus ~0.5–0.6 mA for light sleep, which is why deep sleep is the standby choice.

**Correction:** Caveats: (1) The schematic part is 'AP2112K-3.3V(RT9013)'. The RT9013 alternate is 25 uA typ (Richtek product page), so which LDO is fitted is UNVERIFIED. (2) Datasheet section 5.6.2 says these low-power figures apply to ESP32-S3/S3FH8 and that PSRAM variants such as the S3R8 'might be higher'. (3) Sleep-in typical figures are at VDDI 1.8 V / VDD 2.75 V; at 3.3 V the current can approach the 40 uA max. (4) The ~0.12 mA sum leaves about 0.14 mA of the 0.263 mA measurement unexplained.

**Evidence:** AP2112 (DS39724 Rev. 2-2): IQ 55 uA typ, 80 uA max at IOUT = 0. Schematic: R12 100k from VBAT to GND, about 37-42 uA at 3.7-4.2 V. ST7789V_SPEC_V1.4 section 7.3 Table 3, sleep-in: IDDI 0.005 mA + IDD 0.015 mA typ (max 0.01 + 0.03 mA); normal mode IDD is 6.0 mA typ. ESP32-S3 datasheet v2.2 Table 5-10: Light-sleep 240 uA (VDD_SPI and Wi-Fi off, GPIOs high-impedance), with footnote 1 adding 140 uA for 8 MB 8-line 3.3 V PSRAM. Deep-sleep is 8 uA with RTC peripherals on and 7 uA with them off. The TP4065 BAT-pin current with VCC = 0 is 0 to -1 uA, which is negligible.

### C4: confirmed

**Claim:** The AW9364 dims by counting EN pulses in 16 steps (20 mA down to 1.25 mA per channel; low pulse 0.5–500 µs, high ≥0.5 µs). EN low for more than 2.5 ms shuts it down, and its Iq is 330 µA. LilyGO removed its ledc PWM backlight code as invalid on 2026-03-09.

**Source:** https://doc.awinic.com/doc/202402/d1a46d65-a45d-494f-bfc9-3d6a6a93a860.pdf ; https://github.com/Xinyuan-LilyGO/T-Display-S3/commit/02f0e9dc4b

**Why it matters:** Dimming must use pulse counting, and the backlight level (≈5 mA per step across 4 LEDs) dominates energy per glance.

**Correction:** Shutdown is guaranteed only after more than 2.5 ms low but can start from 0.8 ms; low pulses of 500-800 us are undefined. 5 mA per step is typical (plus or minus 17.5 percent). Because of the internal 150k pull-down, a floating GPIO38 in deep sleep is safe. Holding GPIO38 high would back-feed an unpowered AW9364.

**Evidence:** AW9364 datasheet V2.4 (Feb 2024), p.5: ILED 16.5/20/23.5 mA, IQ 330 uA typ (ILED = 0), ISD 0.1/1 uA, TON 20 us, TLO 0.5-500 us, THI at least 0.5 us, TOFF 800-2500 us. Table 1 maps EN rising edge 1..16 to 20 mA down to 1.25 mA in 1.25 mA steps, and Fig. 7 shows the count wrapping to 16/16. The pin table says EN has an internal 150 kOhm pull-down. LilyGO commit 02f0e9dc4b (2026-03-09T02:57:44Z), 'Remove invalid backlight control methods', replaced ledc PWM with digitalWrite(PIN_LCD_BL, HIGH) in 11 examples. factory.ino setBrightness counts pulses: EN high, wait 30 us, send n pulses; to turn off, EN low then delay(3). The maintainer called ledc 'wrong' in #278 (comment 2367079609).

### C5: partially

**Claim:** The charger is a TP4065 with R13 = 2 kΩ (~500–525 mA). CHRG drives only the red LED and VBUS reaches no GPIO. With USB connected, GPIO4 reads the ~4.6 V system rail instead of the battery.

**Source:** http://toppwr.com/uploadfile/file/20230304/6403023707f41.pdf ; https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/190 ; https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/254

**Why it matters:** Firmware can read neither charge state nor battery voltage while on USB, and plugging in USB cannot wake the chip.

**Correction:** No GPIO is wired to VBUS, but VBUS still reaches GPIO4. VBUS turns on Q1 through D2 regardless of GPIO15, which turns on Q2, so in deep sleep GPIO4 (RTC_GPIO4) steps from 0 V on battery to about 2.3-2.4 V when USB is plugged in. 'Plugging in USB cannot wake the chip' is therefore too strong. An ext0 level-high wake on GPIO4 is physically possible, but 2.3-2.4 V is below the guaranteed VIH (0.75 x VDD, about 2.475 V, datasheet v2.2), so it is UNVERIFIED and unreliable. The schematic's 'R13 IBAT (mA) = 580' note is left over from the TP4056 (#230); about 525 mA is correct for the TP4065.

**Evidence:** Schematic: U6 is a TP4065 with PROG = R13 2k. CHRG drives only red LED1, through R9 2k to VBUS. Q4 SI2307 (gate tied to VBUS, R14 100k to GND) disconnects VBAT from the VIN rail when USB is present, and D3 IN5819 then feeds VIN from VBUS. TP4065 datasheet REV_2.0: RPROG = 1050/IBAT for IBAT above 0.1 A, giving 525 mA at 2.0k; the table lists 2.1k as 500 mA. In #190 a user reports about 4.6 V on USB. #254 (maintainer): above 4.2 V means USB is connected, but charging state cannot be read. #230 (maintainer, 2025-02-06): no built-in battery protection.

### C6: confirmed

**Claim:** There is no 32.768 kHz crystal, because XTAL_32K_P/N (GPIO15/16) are used as PWR_EN and TP_INT. The RTC runs on the internal RC, whose stability Espressif describes as temperature-dependent with no ppm figure, and each deep-sleep interval is converted using a 10-cycle calibration taken at sleep entry.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf ; https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/system_time.html ; https://github.com/espressif/esp-idf/blob/master/components/esp_hw_support/sleep_modes.c

**Why it matters:** Drives the NTP cadence and the need for drift correction, or for an external RTC if ±1 s is required.

**Correction:** Derived: 10 RC cycles is about 73 us, or about 2,940 XTAL counts, so each interval carries roughly plus or minus 340 ppm quantization. The sample is also taken while the die is still warm from the active window, the same mechanism as esp-idf #19248.

**Evidence:** Schematic: SoC pin 21 XTAL_32K_P = PWR_EN, pin 22 XTAL_32K_N = ESP_TP_INT (RINT 100k pull-up); no 32 kHz crystal. ESP-IDF v6.1 System Time page (esp32s3): the internal 90-150 kHz RC has 'frequency stability ... affected by temperature fluctuations' and no ppm figure is given; the 8.5-17.5 MHz/256 source gives 'better frequency stability' at +5 uA in deep sleep. Datasheet v2.2 says only 'typically about 136 kHz'. sleep_modes.c: RTC_CLK_SRC_CAL_CYCLES = 10 (v5.5.6 line 171, master line 146). esp_deep_sleep_start calls sleep_low_power_clock_calibration(true) (master line 1290), which runs rtc_clk_cal(RTC_SLOW, 10) and esp_clk_slowclk_cal_set() (master lines 788-800).

### C7: confirmed

**Claim:** A deep-sleep wake is a Core Reset and strapping pins are latched only at Chip Reset. ESP32-S3 EXT1 supports ANY_LOW on RTC GPIOs, which include GPIO0 and GPIO14, even with RTC_PERIPH powered down.

**Source:** https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf (Table 7.1-1) ; https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/sleep_modes.html

**Why it matters:** Both buttons can wake the chip from one EXT1 mask at the lowest-power configuration, and the BOOT button is safe to use as a wake key.

**Correction:** Notes: the esp_sleep.h header says esp_sleep_enable_ext1_wakeup will be deprecated in favour of esp_sleep_enable_ext1_wakeup_io. EXT1 is level-triggered, so the firmware must wait for both buttons to be released before sleeping, or it will wake immediately.

**Evidence:** ESP32-S3 TRM (PDF dated 2026-03) Table 7.1-1: reset 0x05 'Deep-sleep reset' is a Core Reset, which 'resets the whole digital system except RTC'. TRM chapter 8: 'During Chip Reset ... hardware captures samples ... and holds these bits until the chip is powered down'. Sleep Modes page (v6.1, esp32s3): ext1 offers ANY_HIGH/ANY_LOW and 'supports wakeup even when the RTC peripheral is powered down'; wake pins are held automatically. esp_sleep.h (v5.5.6, v6.0.3, v6.1): ANY_LOW is valid on S3, ALL_LOW is a deprecated alias, and one level applies to all pins (ESP_ERR_NOT_ALLOWED otherwise). Datasheet: GPIO0 = RTC_GPIO0 and GPIO14 = RTC_GPIO14. Schematic: external pull-ups of 10k (GPIO0) and 100k (GPIO14).

### C8: confirmed

**Claim:** SPIRAM memtest adds ~1 s per 4 MB at every boot. BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP takes a fast path before partition selection, so it skips validation and OTA/rollback handling on deep-sleep wakes.

**Source:** https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/performance/speed.html#improving-startup-time ; https://github.com/espressif/esp-idf/blob/master/components/bootloader/Kconfig.projbuild ; https://github.com/espressif/esp-idf/blob/master/components/bootloader/subproject/main/bootloader_start.c

**Why it matters:** These settings decide glance latency, and the OTA flow must reboot with esp_restart and confirm the app before the first deep sleep.

**Evidence:** Speed Optimization page (v6.1, esp32s3): the external RAM memory test costs 'approximately 1 second per 4 MB of memory tested'. The same page names log levels and SKIP_VALIDATE_IN_DEEP_SLEEP as startup-time levers. bootloader_start.c (v5.5.6 and master) calls bootloader_utility_load_boot_image_from_deep_sleep() before select_partition_number(). bootloader_utility.c lines 515-525: on RESET_REASON_CORE_DEEP_SLEEP it loads the partition stored in RTC retain memory with bootloader_load_image_no_verify(), so bootloader_utility_get_selected_boot_partition() (otadata and rollback) never runs. The Kconfig help warns that the option 'should not be used in conjunction with using deep_sleep() entry and changing the active OTA partition'.

### C9: confirmed

**Claim:** The esp_lcd i80 bus holds ESP_PM_NO_LIGHT_SLEEP only during transfers (v5.5/v6.0/master). The stock ST7789 driver delays 100 ms after SLPOUT/SLPIN. esp_lvgl_port ticks LVGL from a 5 ms esp_timer and provides lvgl_port_stop()/resume() for light sleep.

**Source:** https://github.com/espressif/esp-idf/blob/release/v5.5/components/esp_lcd/i80/esp_lcd_panel_io_i80.c ; https://github.com/espressif/esp-idf/blob/master/components/esp_lcd/src/esp_lcd_panel_st7789.c ; https://github.com/espressif/esp-bsp/tree/master/components/esp_lvgl_port#power-saving

**Why it matters:** Shows auto light sleep is possible with LVGL and i80, and where the wake latency goes.

**Evidence:** esp_lcd_panel_io_i80.c (v5.5.6, v6.0.3, v6.1, master): creates an ESP_PM_NO_LIGHT_SLEEP lock (CPU_FREQ_MAX only on P4), acquires it before each tx_param or color transaction and releases it when the transfer finishes. esp_lcd_panel_st7789.c (v5.5.6 and master): init sends SLPOUT then vTaskDelay(100 ms); disp_sleep sends SLPIN or SLPOUT then waits 100 ms; hardware reset waits 10 + 10 ms. esp_lvgl_port 2.9.0 (registry, 2026-08-07): ESP_LVGL_PORT_INIT_CONFIG sets timer_period_ms = 5 and task_max_sleep_ms = 500; a periodic esp_timer calls lv_tick_inc; lvgl_port_stop()/resume() stop and restart that timer, and the README says to stop the timer before light sleep. ST7789V spec 9.1.11/9.1.12: wait 5 ms after SLPIN/SLPOUT, and 120 ms from SLPOUT to SLPIN. After a hardware reset, SLPOUT is not allowed for 120 ms.

## Verifier's assessment of the recommendation

The architecture holds, and each part of it rests on facts verified above: deep sleep as the only standby state, EXT1 ANY_LOW on GPIO0|14 plus an RTC timer, panel SLPIN with the panel kept powered, AW9364 pulse dimming, the fast-boot settings, a USB desk mode, and esp_restart() plus mark-valid for OTA. Deep sleep at about 0.12-0.26 mA beats light sleep at about 0.5-0.65 mA by about 7-9 mAh/day, far more than the extra boot energy. Five corrections:

(1) The stated reason for dimming is wrong at the proposed default. With the report's own numbers at BL 10 mA, the 40 mA MCU is about 70 percent of glance current: about 5.6 mAh/day against 1.4 for the backlight, out of 13.2. #289's 147.7 mA awake figure implies about 60 mA of non-backlight current. DFS, a lower fixed clock, or auto light sleep between the 1 Hz redraws of the 10 s window (possible because i80 blocks light sleep only during transfers and lvgl_port_stop() exists) is likely the biggest software lever. Plan it rather than treat it as optional. The gain is UNVERIFIED, roughly 1.5x runtime at low backlight.

(2) The model's 0.20 mA floor is unsupported. The only measurement is 0.263 mA, against a datasheet sum of about 0.12 mA. At 0.263 mA the BL-10 case drops from 32/64/97 to about 29/58/87 days. If the MCU really draws about 60 mA, a 500 mAh cell gives about 24 days.

(3) Hold every i80 line at a defined level in deep sleep, not only RST/CS. D0-D7 on GPIO39-48 need gpio_hold_en plus gpio_deep_sleep_hold_en on S3. RTC-pad holds persist after wake, so set RST/CS high before calling gpio_hold_dis, or the panel resets and loses GRAM.

(4) The target of plus or minus 1 min with NTP 4x/day is unproven. The 10-cycle calibration is taken warm at each sleep entry, and an ESP32-C6 RC measured -1540 ppm/C (#19248). The S3 is UNVERIFIED, but a few degrees of difference could exceed the 2,800 ppm budget, and a constant stored correction will not track temperature. Bench-test RC (and 8MD256) drift under real glance duty before committing.

(5) 'USB cannot wake' is too strong (see C5); just do not depend on GPIO4 as a wake source.

Smaller points:
- The stock disp_sleep adds 100 ms where the spec needs 5 ms. Sending 0x10/0x11 through esp_lcd_panel_io_tx_param avoids it without a fork, but keep the 120 ms SLPOUT-to-SLPIN rule.
- Turn the ROM log off with the API: CONFIG_BOOT_ROM_LOG_ALWAYS_OFF silences only UART, while esp_deep_sleep_disable_rom_logging() also covers USB-Serial-JTAG (#15921).
- Mark-valid before the first deep sleep is the safe rule. Otherwise an image in PENDING_VERIFY can run for days on fast-path wakes and then roll back at the next non-deep-sleep reset.

## What the research missed (verifier)

1) The floor is mostly fixed hardware. LDO Iq (55 uA, or 25 uA if an RT9013 is fitted) plus R12 (about 40 uA) is about 95 uA, and it drains the cell continuously even with the SoC off. There is no protection IC (maintainer, #230), so there is no true 'off'. The firmware needs a low-battery policy: skip Wi-Fi below about 3.5 V, because AP2112 dropout during 283-340 mA TX peaks can cause brownouts, then stop timer wakes; and use a protected cell. Only a hardware change (removing R12 or swapping the LDO) lowers the floor further.
2) The external-RTC advice needs a power constraint. A DS3231 powered from VCC draws up to 110 uA standby (ICCS at 3.63 V), close to the whole controllable floor, and common modules add LEDs and charging circuits. Use a nano-power TCXO such as the RV-3032-C7 (160 nA, plus or minus 2.5 ppm) on the always-on VDD3V3 I2C header (GPIO17/18 already have 10k pull-ups).
3) Possible cause of the 0.14 mA gap (UNVERIFIED): unheld i80 lines floating into a panel that is powered from VDD3V3. A floating CS could also clock garbage into the panel, for example an accidental SLPOUT. Bench-measure with all lines held.
4) The warm-calibration bias (C6/#19248) is systematic, not random. It is correlated with glance frequency and backlight self-heating, so an NTP-learned constant only partly absorbs it. A short post-glance recalibration wake is an option to evaluate.
5) Light sleep (desk mode or inside the glance): ESP32-S3 erratum RTC-126 advises keeping RTC peripherals powered in light sleep; enabling GPIO wake does this automatically. GPIO38 must stay driven high (PM_SLP_DISABLE_GPIO off or gpio_sleep_sel_dis), otherwise the AW9364 shuts down and re-enables at 20 mA per channel.
6) The battery model never states its usage assumption. Back-calculation shows 50 glances/day of 10 s each; it should be explicit. At 525 mA charge, a 500 mAh cell charges at above 1C (#230).
7) Minor: later v1.2 builds have a re-matched antenna (#340). Older boards have weak Wi-Fi, which can lengthen NTP syncs beyond the modelled 3 s.
