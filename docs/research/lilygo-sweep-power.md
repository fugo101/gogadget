# LilyGO sweep: power, sleep and battery

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 4 of 33 items were verified in lilygo-verification.md (4 confirmed / 0 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): the owner keeps the stock ~525 mA charge current for the 250 mAh cell (risk accepted, no R13 rework); GPIO38 needs no deep-sleep hold (AW9364 internal 150 kOhm pull-down); a USB plug-in during sleep is noticed at the next wake (no ULP polling for now); PSRAM stays enabled (octal, memtest off).

Superseded by decision: CMD_1's leading SLPOUT + 120 ms is kept (ADR-0006), so K16's 'drop the duplicate SLPOUT' is not adopted; the sleep order (K13) also parks DC and D0-D7 LOW and waits until both buttons read HIGH for at least 50 ms (K12 says 30-50 ms); time syncs run about four times a day on battery, not about daily (K25, ADR-0011); the hardware reference uses a USB-test threshold of about 1 V at GPIO4 (K02 says 0.5 V).

Scope: the power tree, sleep sequence, battery and USB sensing, charging, buttons and the ULP examples. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**Sources read:** Sleep, ULP_ADC, ULP_Count, GetBatteryVoltage, factory, touch_test (SerialExample has no power content), README/README_CN, both schematic revisions (2023-03-13 and 2024-04-29), `datasheet/`, about 90 power-related issues, and the TP4065 datasheet linked in #230. A local copy of that datasheet was used for reading only; it is not stored in this repo.

### Hardware facts that change the design

- **GPIO4 measures VSYS, not the cell.**
  - The divider sits on net `BAT`, which is the system rail VSYS switched by Q2.
  - Q1 turns on Q2 and the V3V switch Q3 when GPIO15 is HIGH **or** VBUS is present.
  - On battery with GPIO15 LOW, GPIO4 reads 0 V. With USB connected it reads about VSYS/2, whatever GPIO15 is.
  - So "GPIO15 LOW, then any voltage on GPIO4" is a clean USB test. The official 4.2 V and 4.3 V thresholds are fragile.
- **Fixed battery drains.** R12 (100 kΩ) sits permanently across the cell, about 40 µA. Adding the LDO's own draw, the panel in sleep (about 20 µA) and the ESP32-S3 (about 8 µA) predicts 90–125 µA. The measured figure is 263 µA, so better pin handling in sleep should recover about 100 µA.
- **Charge current is too high for a 250 mAh cell.** The TP4065 sets current as RPROG = 1050/I, so R13 = 2 kΩ gives about 525 mA, roughly 2C (spec spread up to about 575 mA). The schematic note says 580 mA and the README says 500 mA. Fit R13 = 4.3 kΩ (about 244 mA) or 10 kΩ (about 90 mA) unless the cell is rated for 2C.
- **Battery fuse R10 ("5V/2A") wears out.** Users report it degrading to 45 Ω and 350 Ω, which pulls the rails down to 2.6–2.8 V on battery. The maintainer says it is safe to bypass with a protected cell.
- **LDO and brownout.**
  - The maintainer says the LDO is an RT9013 (500 mA), while the ESP32's peak draw is about 700 mA. Wi-Fi averages about 150 mA.
  - Unplugging USB "will restart" the board (maintainer, #174). The likely cause is a short window where the battery feeds through Q4's body diode and the LDO drops out.
  - Plan for a reset on unplug.
- **The "5V" header pin is VSYS** (maintainer). Never feed 5 V into it, or into the battery connector, while a cell is fitted.
- **Buttons.** GPIO0 has a 10 kΩ pull-up. GPIO14 has 100 kΩ plus 100 nF, a 10 ms time constant. Both pull to the always-on rail. ext1 ANY_LOW works without internal pull-ups; wait for the button to be released before sleeping.
- **USB-C.** The schematic shows the CC pins unconnected, so C-to-C chargers may not supply power. Test the actual charger.

### Sleep recipe

**Official sequence:** SLPIN, GPIO15 LOW, backlight LOW, wake on GPIO14 via ext0. There is no 5 ms wait, no pin hold, and a full re-initialisation on wake.

**Recommended for Gogadget:**
1. Backlight LOW for at least 3 ms.
2. SLPIN, then wait at least 5 ms.
3. GPIO15 LOW and hold it.
4. Hold CS, RD, WR and RST HIGH.
5. Hold GPIO38 LOW. It is a digital pad, so it needs `gpio_deep_sleep_hold_en` as well as `gpio_hold_en`.
6. Leave GPIO16–18 floating; they have on-board pull-ups.
7. `esp_wifi_stop`, set up ext1, then sleep.

**Faster wake.** The panel keeps its frame memory and interface working in sleep, so a wake without reset is possible: draw while the panel is asleep, SLPOUT, wait 5 ms, DISPON, backlight last. This replaces LilyGo-IDF's 220 ms or more of initialisation, which includes reset, SLPOUT and a duplicate 0x11 with a 120 ms delay. The energy gain is tiny; the response-time gain is real. Issue #300 shows ghosting when the panel is never put to sleep.

### Is the ULP useful?

- **Timekeeping: no.** The ULP runs on the fast RC oscillator, and the RTC timer already keeps time in deep sleep. Accuracy is limited by the RC slow clock, which drifts on the order of 500 ppm or more. The 10 ppm crystal only applies while awake. Consider the 8MD256 RTC clock source, more calibration cycles, and an NTP sync on every USB session.
- **Buttons: no.** ext1 handles them.
- **USB insertion: yes.** Poll ADC1 channel 3 from the ULP. An ext0 level-HIGH wake on GPIO4 is cheaper but marginal (2.3–2.4 V against an input threshold of about 2.48 V).
- **Battery: measure at each wake instead.** The ULP would have to raise GPIO15, which lights the green LED.
- **The repo's ULP examples should not be ported.** They use legacy macros, keep the CPU awake, read GPIO1 rather than GPIO4, assume an 8 MHz clock, and carry GPL-3.0 headers.

### Other

- **LEDs.** Red is the charger's status pin and is not wired to the MCU. Green sits on V3V: always on with USB, about 0.6 mA when GPIO15 is HIGH.
- **Battery connector.** JST 1.25 mm, 2-pin. The maintainer and the TP4065 datasheet say reverse polarity causes no damage.
- **`datasheet/` folder.** Only the ST7789V datasheet is useful; the other two cover touch controllers. There is nothing for the LDO, charger, AW9364, flash or PSRAM.
- **LilyGo-Display-IDF** has no power management, sleep, ADC, brownout or RTC-clock configuration. It also enables PSRAM, which adds boot time on every wake.

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| K01 | Battery ADC topology (GPIO4) | high | refines | confirmed |
| K02 | USB-presence detection | high | new |  |
| K03 | Official battery-reading code quality | medium | new |  |
| K04 | Deep-sleep floor composition | high | new |  |
| K05 | LDO limits and brownout risk | high | new |  |
| K06 | Battery fuse R10 | high | new |  |
| K07 | '5V' header pin is VSYS | medium | new |  |
| K08 | USB unplug may reset the board | high | new |  |
| K09 | Charge current vs 250 mAh cell | high | refines | confirmed |
| K10 | Schematic revisions and charger naming | low | contradicts |  |
| K11 | Charge and power LEDs | medium | refines |  |
| K12 | Wake-button hardware | high | new |  |
| K13 | Official sleep sequences and a hardened order | high | refines | confirmed |
| K14 | Ghosting after deep sleep without SLPIN | high | new |  |
| K15 | ST7789V power and timing data | high | new |  |
| K16 | Panel wake latency | medium | refines |  |
| K17 | LilyGo-Display-IDF power configuration | medium | new |  |
| K18 | AW9364 pulse algorithm details | high | refines | confirmed |
| K19 | Backlight supply and V3V behaviour | high | new |  |
| K20 | LCD RD pin handling | medium | new |  |
| K21 | GPIO15 pull-ups can keep V3V on | medium | new |  |
| K22 | Board pull-ups on GPIO16/17/18 | medium | new |  |
| K23 | Battery connector, polarity, protection | high | new |  |
| K24 | Documented currents | high | new |  |
| K25 | Timekeeping in deep sleep | high | new |  |
| K26 | Repo ULP examples | medium | new |  |
| K27 | ULP opportunities for the glance clock | high | new |  |
| K28 | Datasheet and schematic inventory | medium | new |  |
| K29 | USB-C CC pins | medium | new |  |
| K30 | No external supply into the battery connector | medium | contradicts |  |
| K31 | Always-on USB mode and the cell | medium | new |  |
| K32 | Console blocking on battery | medium | new |  |
| K33 | factory display bus settings | low | new |  |

### K01: Battery ADC topology (GPIO4)

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `power:K01`).

**Fact:** GPIO4 (ADC1_CH3) reads a 100 kΩ/100 kΩ 1% divider (R2/R4, C13 10 nF) whose top is net BAT, not the cell net VBAT. BAT is the system rail VSYS ('+5V') switched by P-FET Q2 (SI2301). Q2 and the V3V switch Q3 share one gate node (R8 100 kΩ to VSYS, pulled low via R6 1 kΩ by NPN Q1 PMBT3904); Q1's base is diode-ORed from VBUS (D2 1N4148) and PWR_EN/GPIO15 (D1) through R5 10 kΩ with R7 100 kΩ to GND. Result: battery only + GPIO15 HIGH -> (VBAT - I*(R10 fuse + Q4))/2; battery only + GPIO15 LOW -> 0 V; USB present -> (VBUS - D3 drop)/2 regardless of GPIO15.

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd (ADC block R2/R4/C13 BAT->BAT_ADC; Power block Q1,Q2,Q3,R5-R8,D1,D2); #190 user comment 2023-10-13 ('BAT_ADC ... is just measuring the supply voltage'); #284 (GPIO4 reads 0 on battery until GPIO15 HIGH); #190 user code reads GPIO4 as ADC_UNIT_1/ADC_CHANNEL_3

**Action:** Read the cell only with USB absent and GPIO15 HIGH, 3-5 ms after raising GPIO15 (transistor turn-on + 0.5 ms RC), before backlight/Wi-Fi load; average >=8 samples; adc_oneshot on ADC1_CH3, ADC_ATTEN_DB_12, curve-fitting calibration, x2.

### K02: USB-presence detection

Relevance: high. Novelty: new.

**Fact:** VBUS alone switches Q1/Q2 on, so with GPIO15 LOW GPIO4 is ~0 V on battery but ~2.3-2.4 V (VSYS/2) whenever USB power is present: an unambiguous test. The official thresholds ('>4.2V' maintainer #254, '>4300 mV' GetBatteryVoltage) sit only 0.1-0.4 V above a full cell, and VSYS can sag to ~3.9-4.1 V when the TP4065 throttles a weak source at VCC 4.35 V (inference from datasheet 'electrogenic adaptation' + D3 drop). There is no other VBUS sense line and CHRG is not wired to the MCU.

**Evidence:** schematic D2 VBUS->Q1 base@b0a5ecd; examples/GetBatteryVoltage/GetBatteryVoltage.ino:86-93@1cc41eb (4b8e1ee); #254 maintainer 2024-06-03 id 2144090656; #292 maintainer 2024-12-04 id 2515918306 ('This is not possible with current hardware' = USB detect without ADC); TP4065 datasheet REV_2.0 http://toppwr.com/uploadfile/file/20230304/6403023707f41.pdf

**Action:** usb_present(): with backlight off, GPIO15 LOW, wait ~3-5 ms, read GPIO4; > ~0.5 V at the pin = USB. In always-on mode a periodic 5 ms GPIO15 dip is invisible while USB is present (D2 keeps V3V up) and only blinks once on a real unplug; use VSYS > ~4.35 V only as a secondary hint.

### K03: Official battery-reading code quality

Relevance: medium. Novelty: new.

**Fact:** GetBatteryVoltage prints 'No battery connect!' when calibrated GPIO4x2 > 4300 mV and blames the 'TP4056'; since GPIO4 reads VSYS this branch really means 'USB present'. It reads once (no averaging) via legacy esp_adc_cal (ADC_ATTEN_DB_11, 12-bit, Vref 1100), and its init loop has a precedence bug (j < len & 0x7f) that streams ~127 stray bytes after SLPOUT. factory.ino uses uncalibrated analogRead*2*3.3/4096 every 100 ms (the formula LilyGO called 'rough' in #22). Maintainer: the ESP32 ADC reference depends on the supply, 'Voltage data can only be used as a reference'; a user saw 3400 mV vs 3600 mV on a DMM.

**Evidence:** examples/GetBatteryVoltage/GetBatteryVoltage.ino:57,76-93@1cc41eb (4b8e1ee 2023-10-12); examples/factory/factory.ino:424@1cc41eb (84e8604); #22 LilyGO 2022-10-10 id 1272680745; #190 maintainer 2023-10-12 id 1758937039 and user krupis same day

**Action:** Port to adc_oneshot + adc_cali curve fitting with multi-sample averaging and an optional one-point DMM trim stored in NVS; treat >4.3 V as USB/VSYS, never as 'no battery'.

### K04: Deep-sleep floor composition

Relevance: high. Novelty: new.

**Fact:** R12 100 kΩ is hard-wired from VBAT to GND ahead of the power path (same in 2023 and 2024 schematics): ~37-42 µA permanent drain regardless of GPIO15. Other always-on loads: LDO U7 quiescent (RT9013 ~25 µA typ / AP2112K ~55 µA typ; vendor data, not in repo), ST7789V sleep-in 15 µA IDD + 5 µA IDDI typ (30 + 10 max), ESP32-S3 deep sleep ~7-8 µA, TP4065 BAT drain 0..1 µA without VBUS. Expected ~90-125 µA vs 263 µA measured with SLPIN in #289, i.e. ~140 µA unexplained (likely pin states/leakage).

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd Power block (R12 between VBAT and GND); b0a5ecd^:schematic/T_Display_S3.pdf (811c949) same; PCB image in #230 maintainer comment id 1977898759 shows R12 beside C26; datasheet/ST7789V_SPEC_V1.4.pdf §7.3 Table 3 p.40@e662544; TP4065 datasheet electrical characteristics (Sleep Mode VCC=0 V: 0/-1 µA); #289

**Action:** Budget >=40 µA unavoidable from R12; implement pin parking (K13) and measure the floor (target ~100-150 µA); consider removing R12 only after its charger role is understood (open question).

### K05: LDO limits and brownout risk

Relevance: high. Novelty: new.

**Fact:** U7 is labelled 'AP2112K-3.3V(RT9013)'; the maintainer says it is the RT9013, max 500 mA, while ESP32 peaks are ~700 mA, and a replacement needs >700 mA. U7 EN is tied to VSYS via R11 100 kΩ (always on); pin 4 is NC. README limits the header 3V load to 100 mA. On battery the LDO input is VBAT minus the R10 fuse and Q4 drops. A board rebooting at Wi-Fi connect was diagnosed by the maintainer as a PCB power-supply problem.

**Evidence:** schematic U7/R11@b0a5ecd; #291 maintainer 2024-12-07 id 2524990964; #343 maintainer 2026-06-05 id 4627285693; #322 maintainer 2026-03-17 id 4073276027 ('4-pin is a non-connected pin'); README.md:274@1cc41eb (9e7bdc3 2026-08-19); #275 maintainer 2024-09-09 id 2336922839

**Action:** Keep the brownout detector enabled; on battery start Wi-Fi/NTP only above ~3.6-3.7 V under load, cap Wi-Fi TX power, avoid full backlight during Wi-Fi TX, add no external 3V loads; log esp_reset_reason() on every boot.

### K06: Battery fuse R10

Relevance: high. Novelty: new.

**Fact:** Battery connector P3 pin 1 reaches VBAT through R10 labelled '5V/2A保险丝' (fuse). Field reports: R10 degraded to ~45 Ω on four boards after similar numbers of charge cycles (1000 mAh LiPo) and to ~350 Ω on two v1.2 boards, giving 2.6-2.8 V rails and boot loops on battery while USB works. Maintainer: 'Of course, it can be used safely without R10. Generally, lithium-ion batteries have their own protection circuits.'

**Evidence:** schematic Charger block (P3-R10-VBAT)@b0a5ecd; #291 user 2024-12-05..10 and maintainer 2024-12-11 id 2533686412; #223 user 2026-07-07 id 4906786572

**Action:** Incoming check per board: compare cell voltage at P3 with the '5V' header (VSYS) under load; firmware health check: VSYS drop when the backlight turns on (series resistance); bridge a degraded R10 since Gogadget uses a protected cell.

### K07: '5V' header pin is VSYS

Relevance: medium. Novelty: new.

**Fact:** The header pin labelled 5V is the power-path output: VBUS minus D3 (~4.6 V) on USB, ~VBAT on battery. Maintainer: '5V is just a label ... the end point of the path between the battery and USB ... If the battery is connected, this 5V label can directly output the battery voltage ... The correct label should be VSYS.' Feeding 5 V there with a cell attached turns Q4 on (gate = VBUS ~0 V) and back-feeds the cell (user: 'The charging voltage jumps up to 5V'). The earlier maintainer answer '5V = USB 5V' (#205) is contradicted by the D3 Schottky (acknowledged 2023-12-11) and by #270.

**Evidence:** schematic P2 pin 1 '+5V', D3 IN5819, Q4 SI2307, R14@b0a5ecd; #270 maintainer 2024-08-02 id 2264397273; #42 user 2023-11-01 id 1789422028; #205 maintainer 2023-12-01 id 1835690067 and 2023-12-11 id 1849372774; README.md:271-272,275@1cc41eb (9e7bdc3); #295 user measured 4.62 V on USB

**Action:** Document: never power Gogadget through the 5V pin while a cell is connected; use USB-C only.

### K08: USB unplug may reset the board

Relevance: high. Novelty: new.

**Fact:** Maintainer on using the battery as a buffer: 'During the switching process, it will restart. Cannot switch seamlessly.' Power path: D3 1N5819 VBUS->VSYS; Q4 SI2307 VBAT->VSYS with its gate on VBUS (R14 100 kΩ to GND, >=10 µF on VBUS). Inference: after unplug VSYS is carried by Q4's body diode (~VBAT-0.6 V) until VBUS (the gate) decays, pushing the RT9013 into dropout, so heavy load or low VBAT causes a brownout.

**Evidence:** #174 maintainer 2023-07-21 id 1644809119; schematic Power/Charger blocks (D3, Q4, R14, C21/C22)@b0a5ecd

**Action:** Treat ESP_RST_BROWNOUT/POWERON after an always-on session as expected: keep a 'time valid' flag plus last NTP epoch in RTC memory and NVS, show '--:--' and resync if invalid; keep always-on load low (CPU <=160 MHz, Wi-Fi off between syncs, moderate backlight); bench-test unplug at several VBAT values and record reset reason and RTC-time survival.

### K09: Charge current vs 250 mAh cell

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `power:K09`).

**Fact:** TP4065 (SOT-23-5, marking 65b...) programs IBAT via R13 = 2 kΩ: datasheet formula one RPROG = 1050/IBAT (IBAT > 0.1 A) gives ~525 mA; at 2.1 kΩ the spec is 425/500/575 mA min/typ/max; above ~0.4 A thermal regulation lowers the current. The schematic annotation reads 'R13 IBAT (mA) = 580' (a TP4056-table value) and README FAQ 8 says 500 mA. Termination at C/10, float 4.2 V ±1%, auto-recharge at ~80-90% capacity. For 250 mAh that is ~2.1C (up to ~2.3C). R13's location is in the maintainer's PCB image (#230).

**Evidence:** schematic Charger block U6/R13 note@b0a5ecd; TP4065 datasheet REV_2.0 (formulas, table, electrical characteristics) http://toppwr.com/uploadfile/file/20230304/6403023707f41.pdf; README.md:238@1cc41eb (6fe4d51); #230 maintainer 2024-03-05 id 1977898759

**Action:** Unless the cell datasheet allows ~2C charging, replace R13: 4.3 kΩ ≈ 244 mA (1C), 5.1 kΩ ≈ 206 mA, or 10 kΩ ≈ 90 mA (formula two 900/IBAT); termination current scales accordingly.

### K10: Schematic revisions and charger naming

Relevance: low. Novelty: contradicts.

**Fact:** The 2023-03-13 schematic shows U6 'TP4056B', GPIO0 pull-up R1 = 100 kΩ and CHIP_PU pull-up RW = 100 kΩ; the 2024-04-29 v1.2 schematic shows TP4065 with R1 = 10 kΩ and RW = 10 kΩ, although the maintainer said 'There is no other difference except that the charging chip has changed.' The maintainer later calls the charger 'TP4054' (#343) and GetBatteryVoltage comments name 'TP4056'. Power-path parts (D3, Q1-Q4, R5-R8, R12, R14, U7) are unchanged between revisions.

**Evidence:** git show b0a5ecd^:schematic/T_Display_S3.pdf (811c949, 2023-03-13) vs schematic/T_Display_S3.pdf@b0a5ecd; #236 maintainer 2024-04-29 id 2082060198; #343 maintainer 2026-06-04 id 4618208000; examples/GetBatteryVoltage/GetBatteryVoltage.ino:86@1cc41eb

**Action:** Do not depend on the GPIO0 pull-up value (works either way); identify the charger by its '65b' marking if it matters.

### K11: Charge and power LEDs

Relevance: medium. Novelty: refines.

**Fact:** Red LED1: VBUS -> R9 2 kΩ -> TP4065 CHRG, so it is lit only from USB and never drains the cell; README: flashes or dimly lights with no battery, on while charging, off when full. TP4065 datasheet: CHRG pulses with no battery (period ~0.5-3 s with 10 µF on BAT). Maintainer: 'A constantly lit red light is incorrect; it should be flashing when there's no battery' and 'The red LED is meaningless; it's just a charging indicator'. Green LED8: V3V -> N5 2 kΩ (~0.6 mA whenever V3V is on, i.e. USB present or GPIO15 HIGH; always on with USB). CHRG is not routed to any GPIO.

**Evidence:** README.md:240-256@1cc41eb (dbd46ad 2026-06-08, 6fe4d51 2026-07-17); schematic LED1/R9, LED8/N5@b0a5ecd; #31 LilyGO 2022-10-24 id 1288275715; #343 maintainer 2026-06-08 id 4644706509; #322 maintainer 2026-01-13 id 3741261557; TP4065 datasheet 'Charging status indicator'

**Action:** UI can show 'USB connected' but not 'charging/full'; accept the green LED (~0.6 mA) during battery glances and permanently on USB.

### K12: Wake-button hardware

Relevance: high. Novelty: new.

**Fact:** GPIO0 (BOOT): R1 pull-up to VDD3V3 (10 kΩ in the 2024 schematic, 100 kΩ in 2023), no capacitor. GPIO14 (KEY): R3 100 kΩ pull-up to VDD3V3 plus C14 100 nF to GND (τ ≈ 10 ms hardware debounce, slow rising edge). CHIP_PU: RW pull-up + CU 1 µF. VDD3V3 is always on, so both buttons idle HIGH in deep sleep with zero current. factory sleeps on a GPIO0 click and wakes on GPIO14 (ext0 LOW), so the sleep press cannot re-wake.

**Evidence:** schematic Button block@b0a5ecd; 2023 schematic (811c949); examples/factory/factory.ino:395-404@1cc41eb

**Action:** Use ext1 ANY_LOW on (1<<0)|(1<<14) with no internal pulls; debounce >=20 ms; before sleeping require both lines HIGH for >=30-50 ms (GPIO14 rises with τ≈10 ms) to avoid instant re-wake; read esp_sleep_get_ext1_wakeup_status() to know which button woke.

### K13: Official sleep sequences and a hardened order

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `power:K13`).

**Fact:** factory: SLPIN (0x10 via esp_lcd_panel_io_tx_param) -> GPIO15 LOW -> BL LOW -> ext0 GPIO14 LOW -> esp_deep_sleep_start. Sleep example: GPIO15 LOW -> BL LOW -> SLPIN -> ext0 GPIO14 or a 15 s timer -> deep sleep, followed by full re-init in setup() after wake. Neither waits 5 ms after SLPIN, holds any pin, or stops Wi-Fi; touch_test only holds the touch reset pin (gpio_hold_dis at boot, gpio_hold_en before sleep). No example uses light sleep or esp_pm. IDF v6 docs: on ESP32-S3 gpio_hold_en does not keep digital pads (GPIO22+) in deep sleep without gpio_deep_sleep_hold_en; rtc_gpio_isolate minimizes leakage on RTC pads.

**Evidence:** examples/factory/factory.ino:395-404@1cc41eb (c9fd58a 2024-11-27, #289); examples/Sleep/Sleep.ino:77-89 (4b2edc1) and 97-116 (fc466c6 2025-03-19, #300)@1cc41eb; examples/touch_test/touch_test.ino:66,152-154 (9ceb658); https://docs.espressif.com/projects/esp-idf/en/release-v6.0/esp32c6/api-reference/peripherals/gpio.html (gpio_hold_en note lists S3)

**Action:** Gogadget order: BL LOW >=3 ms -> SLPIN -> wait >=5 ms -> GPIO15 LOW + hold -> CS/RD/WR (and RST if keeping registers) HIGH + hold, GPIO38 LOW + hold + gpio_deep_sleep_hold_en -> GPIO16-18 high-Z -> esp_wifi_stop -> wait for button release -> ext1 (+ USB wake) -> esp_deep_sleep_start; on wake reconfigure pins to the same levels before releasing holds.

### K14: Ghosting after deep sleep without SLPIN

Relevance: high. Novelty: new.

**Fact:** Matte-screen units showed white borders and ghosts of the previous text after minutes of deep sleep on battery when the driver never sent SLPIN; it was fixed by driving all LCD pins (38, 39-48, 15, 5-9) LOW before sleep. Maintainer advice: 'After the screen is initialized, refresh the screen to all black, and then turn on the backlight last'; the Sleep example was revised for this issue.

**Evidence:** #300 user 2025-03-14..24; maintainer 2025-03-10 id 2709985067 and 2025-03-19 id 2735459793; commit fc466c6

**Action:** Always SLPIN before deep sleep (both for current and image retention); after wake draw the frame first and turn the backlight on last.

### K15: ST7789V power and timing data

Relevance: high. Novelty: new.

**Fact:** Table 3: normal mode IDD 6.0 mA typ (7.5 max), partial+idle 5.0 mA, sleep-in IDD 15 µA typ (30 max) + IDDI 5 µA (10 max); the 6.42 mA vs 263 µA measured in #289 matches normal vs sleep-in. SLPIN stops DC/DC, oscillator and scanning, but 'MCU interface and memory are still working and the memory keeps its contents'. Wait 5 ms after SLPIN/SLPOUT before the next command, and 120 ms between SLPOUT and a following SLPIN. Reset: RESX >=10 µs (pulses <5 µs rejected); 5 ms after release before commands; SLPOUT not within 120 ms; power-on/SW/HW reset default = sleep-in. Supplies may drop 0 ms after RESX when in sleep-in. DISPOFF only blanks the output.

**Evidence:** datasheet/ST7789V_SPEC_V1.4.pdf@e662544: §7.3 Table 3 (p.40), Table 9 notes 5-7 (p.47-48), §8.16 (p.136), §9.1.11 SLPIN (p.182), §9.1.12 SLPOUT (p.183), §9.1.18 DISPOFF; #289

**Action:** Budget the sleeping panel at ~20 µA; enforce >=120 ms awake between SLPOUT and SLPIN (quick double press); never use DISPOFF as a low-power state.

### K16: Panel wake latency

Relevance: medium. Novelty: refines.

**Fact:** LilyGo-Display-IDF calls esp_lcd_panel_reset + esp_lcd_panel_init (IDF ST7789 driver: ~20 ms reset, SLPOUT + ~100 ms) and then sends the CMD_1 table whose first entry is SLPOUT 0x11 followed by vTaskDelay(120 ms): >=220 ms of fixed delays per boot. factory does the same with Arduino delay(120). Because GRAM and the MCU interface survive sleep-in and the panel stays powered (VDD3V3), a wake without reset/init is possible: write the frame while still in sleep-in, then SLPOUT, 5 ms, DISPON, backlight.

**Evidence:** LilyGo-Display-IDF main/display_s3.c:113-145@b1a1cc5; examples/factory/factory.ino:306-331@1cc41eb; datasheet/ST7789V_SPEC_V1.4.pdf §9.1.11-9.1.12

**Action:** At minimum drop the duplicate SLPOUT/120 ms when esp_lcd_panel_init already sent SLPOUT (pass the rest of CMD_1 as vendor init commands); optionally implement a no-reset wake with RST/CS held HIGH through deep sleep (~20 ms to first frame). The energy saving is negligible (~3 µAh per wake); the gain is glance latency.

### K17: LilyGo-Display-IDF power configuration

Relevance: medium. Novelty: new.

**Fact:** For T-Display-S3, power_driver_init() only drives BOARD_POWERON (GPIO15) HIGH; there is no sleep, battery ADC, esp_pm, brownout, RTC-clock or ULP configuration anywhere. sdkconfig.defaults.t-display-s3 sets only SPIRAM (octal), the USB-Serial-JTAG console, CPU 240 MHz, 16 MB flash and LVGL options.

**Evidence:** LilyGo-Display-IDF main/power_driver.cpp:172-195, main/product_pins.h:202-204, sdkconfig.defaults.t-display-s3:1-13, main/main.cpp:129-143@b1a1cc5

**Action:** Own the power sdkconfig: CONFIG_SPIRAM off (internal DMA buffers; avoids PSRAM init/memtest on every wake), BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP, quiet boot logs, brownout on, explicit RTC clock source, ULP enabled only if used.

### K18: AW9364 pulse algorithm details

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `power:K18`).

**Fact:** factory setBrightness(0..16): 0 = EN LOW + delay(3) ms; from off, EN HIGH + 30 µs and the chip starts at max (16); then (16 + (16-target) - (16-current)) % 16 LOW->HIGH pulses, each one step down with wrap 1->16 (going 1->2 takes 15 pulses). Pulses are back-to-back digitalWrite calls with no explicit delay. It was added on 2024-09-23 for #278 (maintainer: 'The method of ledc controlling the backlight of display-S3 is wrong'), 18 months before the LEDC clean-up in 02f0e9d. The boot ramp steps 0..16 every 50 ms.

**Evidence:** examples/factory/factory.ino:223-248,371-378@1cc41eb (2d3c294 2024-09-23); #278 maintainer 2024-09-23 id 2367079609

**Action:** Port with explicit 1-2 µs high/low phases inside a critical section (a preempted low phase can be miscounted or reach the ~2.5-3 ms shutdown time); invalidate the cached level whenever V3V drops (GPIO15 LOW on battery, or USB removed while GPIO15 is LOW); use a dimmer default level on battery.

### K19: Backlight supply and V3V behaviour

Relevance: high. Novelty: new.

**Fact:** AW9364DNR VIN = V3V (CW1 1 µF), EN = GPIO38 (ESP_LCD_BLK); the LED anode (FPC pin 20) is V3V and panel logic (FPC pin 2) is VDD3V3. V3V is VDD3V3 switched by Q3, on when GPIO15 is HIGH or VBUS is present. Keeping GPIO15 HIGH with the backlight off costs ~0.8-0.9 mA (green LED ~0.6 mA, Q1 base ~0.2 mA via D1/R5, R8 ~40 µA, divider ~20 µA; estimate from schematic values). GPIO38 is a digital-only pad: in deep sleep it floats unless held, and with USB present (or a ULP raising GPIO15) V3V powers the AW9364.

**Evidence:** schematic Display block (CN1 pins 2 and 20, U4, CW1) and Power block (Q3, D2)@b0a5ecd; https://docs.espressif.com/projects/esp-idf/en/release-v6.0/esp32c6/api-reference/peripherals/gpio.html (gpio_deep_sleep_hold_en note)

**Action:** Hold GPIO38 LOW through deep sleep (gpio_hold_en + gpio_deep_sleep_hold_en); keep GPIO15 HIGH on battery only while the backlight is needed.

### K20: LCD RD pin handling

Relevance: medium. Novelty: new.

**Fact:** esp_lcd's i80 bus does not drive RD; both official code bases configure GPIO9 as output HIGH before creating the bus. A floating RD with CS low could let the panel drive D0-D7, which include the strapping pins GPIO45/46.

**Evidence:** examples/factory/factory.ino:257-258@1cc41eb; LilyGo-Display-IDF main/display_s3.c:64-69@b1a1cc5

**Action:** Drive GPIO9 HIGH before esp_lcd_new_i80_bus and hold it (RTC pad) during deep sleep together with CS HIGH.

### K21: GPIO15 pull-ups can keep V3V on

Relevance: medium. Novelty: new.

**Fact:** Q1 needs only tens of µA of base current, so an internal pull-up on GPIO15 (e.g. gpio_reset_pin(15), which enables the pull-up) is enough to switch V3V on, as a user found; in deep sleep that would keep the green LED, divider and AW9364 VIN powered. R7 100 kΩ keeps Q1 off if GPIO15 floats without pulls.

**Evidence:** #51 user comment 2022-12-20 id 1358844938 (gpio_reset_pin(15) powers the display); schematic D1, R5 10 kΩ, R7 100 kΩ, Q1@b0a5ecd

**Action:** Before deep sleep set GPIO15 output LOW + gpio_hold_en (RTC pad), or rtc_gpio_isolate(15); never use gpio_reset_pin(15) as a 'release'.

### K22: Board pull-ups on GPIO16/17/18

Relevance: medium. Novelty: new.

**Fact:** Even on the non-touch board GPIO17/18 (I2C) have 10 kΩ and GPIO16 (touch INT) 100 kΩ pull-ups to VDD3V3; the maintainer says they sit on the back of the screen and the pins float HIGH.

**Evidence:** #313 maintainer 2025-07-29 ids 3131675491 and 3131730031; schematic RIIC/RIID 10 kΩ and RINT 100 kΩ@b0a5ecd

**Action:** Never drive or pull GPIO16-18 LOW in deep sleep (~330 µA per I2C line, ~33 µA on GPIO16); leave them as inputs without pull-down or isolate them.

### K23: Battery connector, polarity, protection

Relevance: high. Novelty: new.

**Fact:** README: battery 3.7-4.2 V, connector 'JST 1.25 mm'. The maintainer's reference image is a '1.25 mm JST 2 Pin Micro Electrical Connector' (PicoBlade-style); a user reported a JST-GH plug did not fit. The PCB has +/- markings at P3. Maintainer: 'Reversing the polarity will not cause any damage'; the TP4065 datasheet claims battery-reverse protection. Besides the R10 fuse there is no protection (known).

**Evidence:** README.md:269-270@1cc41eb (9e7bdc3); #287 maintainer 2024-11-20 id 2487122092; #317 maintainer 2025-09-15 id 3290136564; PCB image in #230 (P3 '-'/'+'); TP4065 datasheet 'Battery reverse connection protection'

**Action:** Order the 250 mAh protected cell with a 2-pin 1.25 mm-pitch (PicoBlade/'MX1.25') plug and check its polarity against the board marking before first connection.

### K24: Documented currents

Relevance: high. Novelty: new.

**Fact:** Awake 147.7 mA vs 6.42 mA / 263 µA asleep (user code comment in #289); Wi-Fi on averages ~150 mA+ with the heat centred on the ESP32 (maintainer #295); ESP32 peaks ~700 mA (maintainer #291); VSYS measured 4.62 V on USB (#295 user). A community measurement found DISPOFF + backlight off still ~30 mA above a never-initialised display (#33).

**Evidence:** #289 issue body 2024-11-26; #295 maintainer 2024-12-19 id 2552567520; #291 maintainer 2024-12-07 id 2524990964; #33 user 2023-06-21 id 1600840120

**Action:** Budget: 10 s glance at ~80-150 mA ≈ 0.2-0.4 mAh; NTP sync ~150 mA for a few seconds; 250 mAh lasts ~40 days at 263 µA standby (~85 days at ~120 µA). Measure backlight current per AW9364 level and pick the battery default.

### K25: Timekeeping in deep sleep

Relevance: high. Novelty: new.

**Fact:** The 40 MHz crystal is 10 ppm (maintainer), but in deep sleep the RTC timer runs from an internal RC because no 32 kHz crystal can be fitted. IDF docs: the default 136 kHz RC drifts with temperature (C6 BLE doc: >500 ppm sleep-clock accuracy); ESP32-S3 also offers the 17.5 MHz RC/256 source with better stability at higher deep-sleep current; CONFIG_RTC_CLK_CAL_CYCLES trades boot time for calibration precision. factory disconnects Wi-Fi in the SNTP sync callback. A user reported 1-2 min/month drift after a one-shot NTP sync.

**Evidence:** #262 maintainer 2024-07-08 id 2212703094; https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/system/system_time.html; https://docs.espressif.com/projects/esp-idf/en/release-v6.0/esp32c6/api-guides/low-power-mode/low-power-mode-ble.html; examples/factory/factory.ino:453-471,716-722@1cc41eb; #253 user 2024-05-27

**Action:** Evaluate CONFIG_RTC_CLK_SRC_INT_8MD256 (measure the added µA) and a larger CONFIG_RTC_CLK_CAL_CYCLES; resync NTP every USB session and about daily on battery when VBAT is healthy; stop Wi-Fi in the sync callback; measure drift over a week.

### K26: Repo ULP examples

Relevance: medium. Novelty: new.

**Fact:** ULP_ADC/ULP_Count (LILYGO_L, 2024-05-25, 'arduino_esp32 3.0.0-rc3') use legacy ULP-FSM macros, run while the main CPU stays awake (no deep sleep), busy-wait with I_DELAY assuming 'ULP runs at 8 MHz' (ESP32-S3 RC_FAST is ~17.5 MHz), and ULP_ADC samples ADC1 channel 0 = GPIO1 (not GPIO4) without any ADC setup. Both files carry '@License: GPL 3.0' headers although the repo is MIT.

**Evidence:** examples/ULP_ADC/ULP_ADC.ino:1-10,17-56,58-69@1cc41eb; examples/ULP_Count/ULP_Count.ino:1-10,28-42@1cc41eb (9615a6b); README.md:57-58@1cc41eb; https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/peripherals/clk_tree.html

**Action:** Do not port; write any ULP code fresh against IDF v6 (ulp_fsm or ulp_riscv, ulp_adc_init, ulp_set_wakeup_period + halt instead of busy loops).

### K27: ULP opportunities for the glance clock

Relevance: high. Novelty: new.

**Fact:** Time: no gain; the ULP clock is RC_FAST and the RTC timer already counts in deep sleep. Buttons: no; both are RTC GPIOs with external pull-ups, so ext1 ANY_LOW suffices. USB insertion: yes; with GPIO15 LOW the GPIO4 divider is unpowered on battery (0 V) but ~2.3-2.4 V whenever VBUS is present, so a 1-2 s ULP ADC1_CH3 poll can wake the CPU into always-on mode. ext0 level-HIGH on GPIO4 is cheaper, but ~2.4 V is at or below the S3 VIH(min) ≈ 0.75·VDD ≈ 2.48 V. Battery watch: the ULP would have to raise GPIO15 (an RTC GPIO), lighting the green LED and V3V on each sample; measuring at each glance is better.

**Evidence:** schematic Power/ADC blocks (D2 VBUS->Q1->Q2->BAT->R2/R4->GPIO4)@b0a5ecd; ESP32-S3 datasheet DC characteristics (external); IDF ULP ADC API ulp_adc_init (external)

**Action:** Implement USB wake with a ULP ADC poll (threshold ~1 V at the pin, with hysteresis); optionally bench-test ext0 HIGH on GPIO4 across boards; keep battery measurement in the app at wake; no ULP for time or buttons.

### K28: Datasheet and schematic inventory

Relevance: medium. Novelty: new.

**Fact:** datasheet/ (added e662544, 2023-11-10) holds only ST7789V_SPEC_V1.4.pdf (Sitronix 2014, 316 pp: power table, sleep/reset timing), CST328数据手册V2.2.pdf (touch controller, 12 pp) and CST816S寄存器说明-20190508.pdf (touch registers, 5 pp). There is no LDO, charger, AW9364, flash or PSRAM datasheet; the ESP32-S3 datasheet/TRM were removed in da406bd (2024-01-31). schematic/ holds T_Display_S3.pdf (v1.2, b0a5ecd) and the MIDI shield; examples/T-Display-S3-Piano/docs/T_Display_S3_Microcontrolador.pdf is a byte-identical copy of the current schematic. The schematic puts the W25Q128JV flash on VDD_SPI, which is powered down in deep sleep by default.

**Evidence:** git log -- datasheet schematic (e662544, da406bd, b0a5ecd, 811c949); pdfinfo of each PDF; cmp of the Piano docs PDF against schematic/T_Display_S3.pdf

**Action:** Obtain external datasheets for the TP4065 (link in #230), RT9013/AP2112K, AW9364 and ESP32-S3; treat the in-repo ST7789V spec as authoritative for the panel.

### K29: USB-C CC pins

Relevance: medium. Novelty: new.

**Fact:** In the schematic the USB-C receptacle's CC (A5) and VCONN/CC2 (B5) pins are unconnected stubs, and the resistor list (R1-R14, RIIC, RIID, RINT, RW, RX, RX1) has no 5.1 kΩ Rd; a user's engineer claimed the board 'lacks any connections to CC1 or VConn'. The maintainer's PCB image (#230) shows resistors R01/R04 that are not in the schematic, so the real board may differ.

**Evidence:** schematic Charger block U5 'TYPEY-C'@b0a5ecd; #65 user 2023-04-24 id 1520473413; #230 PCB image

**Action:** Test always-on mode with the intended charger and a C-to-C cable; if VBUS does not appear, specify an A-to-C cable or a legacy charger.

### K30: No external supply into the battery connector

Relevance: medium. Novelty: contradicts.

**Fact:** Maintainer: feeding 5 V into the battery holder while USB is connected 'most likely burned out the TP4054 charging chip'; the battery-holder voltage should not exceed 4.2 V. This contradicts community advice in #205 ('3V to 6V coming into the battery charge connector').

**Evidence:** #343 maintainer 2026-06-04 id 4618208000 and 2026-06-08 id 4644706509; #205 community comment 2023-11-30

**Action:** Connect only a 1S Li-ion/LiPo (<=4.2 V) to P3; any bench supply goes through USB-C.

### K31: Always-on USB mode and the cell

Relevance: medium. Novelty: new.

**Fact:** With USB present D3 feeds VSYS (~4.6 V) and Q4 is off (gate = VBUS), so the system does not draw from the cell. The TP4065 charges to 4.2 V, terminates at C/10, idles (75 µA from VBUS) and auto-recharges at ~80-90% capacity; R12 (~40 µA) slowly drains the cell between top-ups; thermal regulation reduces the current above ~0.4 A.

**Evidence:** schematic Power/Charger blocks@b0a5ecd; TP4065 datasheet (operation, termination, automatic restart)

**Action:** Always-on mode is cell-friendly and needs no firmware charge control; show only 'USB connected'.

### K32: Console blocking on battery

Relevance: medium. Novelty: new.

**Fact:** README FAQ 5: with Arduino 'USB CDC On Boot' enabled the board waits for USB access at startup, so battery users must disable it; the maintainer gave the same advice for battery boot loops (#291). The IDF analogue is the USB-Serial-JTAG primary console used by LilyGo-Display-IDF.

**Evidence:** README.md:224-232@1cc41eb (6fe4d51); LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:10@b1a1cc5; #291 maintainer 2024-12-03 id 2513302390

**Action:** Verify that the IDF v6 USB-Serial-JTAG console never stalls boot or logging without a host (measure wake-to-display time on battery); keep battery-mode logging minimal.

### K33: factory display bus settings

Relevance: low. Novelty: new.

**Fact:** factory runs the i80 bus at 16 MHz (comment: 'Too low or too high pixel clock may cause screen mosaic') with a full-frame 320x170 LVGL buffer (108,800 B) allocated in internal DMA RAM and max_transfer_bytes = one full frame.

**Evidence:** examples/factory/pin_config.h:24-30@1cc41eb (adc038e 2023-03-03); examples/factory/factory.ino:276,333@1cc41eb

**Action:** Keep 10 MHz; a full-frame internal DMA buffer is proven to fit if partial buffers are too slow (a 16-bit frame takes ~11 ms at 10 MHz vs ~7 ms at 16 MHz, a negligible energy difference).

## Reusable code

### 1. AW9364 16-level pulse-count backlight control (setBrightness): 0 = EN LOW + 3 ms; first enable = EN HIGH + 30 µs (chip starts at level 16); then (16 + (16-target) - (16-current)) % 16 LOW->HIGH pulses, each stepping one level down with wrap 1->16; plus the boot fade 0..16 in 50 ms steps.

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:223-248 and 371-378 @1cc41eb (introduced 2d3c294, 2024-09-23, issue #278)

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO; the file has no separate header)

**Port notes:** Replace digitalWrite with gpio_set_level and add explicit ~1-2 µs esp_rom_delay_us() for each low and high phase (Arduino call overhead provided this implicitly); emit the pulse train inside a critical section so no low phase is stretched by preemption; keep >=3 ms LOW for off; reset the cached level to 0 whenever V3V drops (GPIO15 LOW on battery, or USB unplugged while GPIO15 is LOW); hold GPIO38 LOW with gpio_hold_en + gpio_deep_sleep_hold_en during deep sleep.

### 2. Panel sleep plus deep-sleep entry: SLPIN 0x10 via esp_lcd_panel_io_tx_param, GPIO15 LOW, backlight LOW, ext0 wake on GPIO14 LOW; the Sleep example adds a 15 s timer wake.

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:395-404 @1cc41eb (c9fd58a, #289); examples/Sleep/Sleep.ino:77-89 and 105-116 @1cc41eb (4b2edc1, fc466c6, #300)

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** The factory version already uses esp_lcd. Reorder to backlight off (>=3 ms) -> SLPIN -> vTaskDelay(5 ms) -> GPIO15 LOW + hold; add holds for CS/RD/WR/RST (RTC pads, gpio_hold_en) and GPIO38 (+ gpio_deep_sleep_hold_en), call esp_wifi_stop(), wait until both buttons are released, and use ext1 ANY_LOW on GPIO0|GPIO14 (esp_sleep_enable_ext1_wakeup_io or the API available in IDF v6). On boot, set pins to the same levels, then gpio_hold_dis / gpio_deep_sleep_hold_dis.

### 3. Battery/VSYS voltage read: calibrated ADC on GPIO4 x2 and a '>4300 mV' heuristic, which in reality means USB/VSYS is present.

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/GetBatteryVoltage/GetBatteryVoltage.ino:76-93 @1cc41eb (4b8e1ee, 2023-10-12)

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Replace legacy esp_adc_cal (ADC_ATTEN_DB_11, width 12, Vref 1100) with adc_oneshot_new_unit(ADC_UNIT_1), adc_oneshot_config_channel(ADC_CHANNEL_3, ADC_ATTEN_DB_12) and adc_cali_create_scheme_curve_fitting; multiply by 2 (R2 = R4 = 100 kΩ 1%); average several samples taken >=3-5 ms after GPIO15 HIGH; interpret >4.3 V as USB present, not 'no battery'; do not copy the init loop at line 57 (precedence bug j < len & 0x7f).

### 4. Board power-on: drive GPIO15 (BOARD_POWERON) HIGH at startup through gpio_config.

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/power_driver.cpp:172-195 and main/product_pins.h:204 @b1a1cc5

**Licence:** MIT (file header '@license MIT', Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd; repo LICENSE Copyright (c) 2024 TTGO)

**Port notes:** Trivial. In Gogadget call gpio_hold_dis(15) first on wake, set the output level before enabling the output to avoid a glitch, and treat GPIO15 as the 'backlight rail + ADC divider enable', not panel power.

### 5. Drive LCD RD (GPIO9) HIGH before creating the i80 bus, because esp_lcd does not use RD.

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:64-69 @b1a1cc5; Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:257-258 @1cc41eb

**Licence:** MIT (both repositories)

**Port notes:** Keep RD HIGH and hold it (gpio_hold_en on the RTC pad) through deep sleep together with CS HIGH.

### 6. Pin hold and release pattern across deep sleep: gpio_hold_dis at the very start of setup(), gpio_hold_en right before esp_deep_sleep_start.

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/touch_test/touch_test.ino:66 and 152-154 @1cc41eb (9ceb658)

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Apply to GPIO15/38/5/6/9 (and D0-D7 if parked); digital pads (GPIO38, 39-48) also need gpio_deep_sleep_hold_en() and gpio_deep_sleep_hold_dis() on ESP32-S3 per the IDF v6 GPIO docs.

### 7. ULP-FSM macro programs: a seconds counter and periodic ADC sampling stored in RTC_SLOW_MEM.

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/ULP_ADC/ULP_ADC.ino and examples/ULP_Count/ULP_Count.ino @1cc41eb (9615a6b, 2024-05-25)

**Licence:** File headers say '@License: GPL 3.0', which conflicts with the repo's MIT licence; treat as GPL-3.0 and do not copy into Gogadget

**Port notes:** Reuse only the idea. Write a new ULP (FSM or RISC-V) for IDF v6 using ulp_adc_init(ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12), timer-driven via ulp_set_wakeup_period plus halt instead of busy-wait I_DELAY loops (which assume an 8 MHz clock; the ESP32-S3 RC_FAST is ~17.5 MHz).

## Open questions

- Which LDO is actually fitted on current v1.2 boards (RT9013 per the maintainer, or AP2112K as also printed on the schematic), and what is its quiescent current? This sets part of the deep-sleep floor.
- What makes up the ~140 µA gap between the expected ~90-125 µA and the 263 µA measured in #289? Candidates: floating LCD bus inputs, GPIO15/38 states, D3 1N5819 reverse leakage from VSYS into R14 (worse when warm). What floor does a fully parked v1.2 board reach?
- Does unplugging USB cause a brownout reset in an ESP-IDF v6 build, which reset reason does it report, and do the RTC timer and RTC memory (system time) survive that reset on the ESP32-S3?
- Are 5.1 kΩ Rd resistors fitted on CC1/CC2 on the real PCB? The schematic shows CC and VCONN unconnected, while R01/R04 appear in the #230 PCB image but not in the schematic. Do USB-C chargers with C-to-C cables power the board?
- AW9364 limits are not in the repo: minimum and maximum low-pulse width, shutdown time (factory uses 3 ms), startup delay (factory uses 30 µs), LED current per channel and per level, and whether EN has an internal pull-down.
- Is ext0 level-HIGH wake on GPIO4 (~2.3-2.4 V with USB present during deep sleep) reliable across boards and temperature, given an ESP32-S3 VIH(min) of about 2.48 V? Otherwise the ULP ADC poll is required.
- What is the purpose of R12 (100 kΩ across VBAT), and is it safe to remove it (for example its effect on the TP4065 no-battery detection and LED flashing)?
- Does the ST7789 keep its registers and GRAM reliably through long deep sleeps with RST and CS held HIGH, so that a no-reset wake (draw while in sleep-in, then SLPOUT/DISPON) is stable?
- How large is the RTC drift on this board in a pocket with the default 136 kHz RC versus RC_FAST/256, and how much extra deep-sleep current does RC_FAST/256 cost on the ESP32-S3?
- Confirm that waking from deep sleep while GPIO0 (a strapping pin) is held LOW boots the app normally and does not enter download mode.
- With USB present, V3V is on from the first moment. Does the backlight flash during boot before firmware drives GPIO38 LOW (depends on whether the AW9364 EN pin has an internal pull-down)?
- Does the ESP-IDF v6 USB-Serial-JTAG console block or add latency to boot and logging when no host is connected (the IDF analogue of README FAQ 5)?
- Which exact connector part fits the board's battery socket (PicoBlade-compatible 1.25 mm or another '1.25 mm JST' variant), and does the chosen 250 mAh pack's polarity match the +/- silkscreen?
- How common is R10 fuse degradation on new v1.2 boards (two reports show 45 Ω and 350 Ω)? Should Gogadget bridge R10 preemptively, given that it uses a protected cell?
