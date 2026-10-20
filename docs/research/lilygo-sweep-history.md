# LilyGO sweep: commit history and maintenance

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; none of its 50 items were in the verified top-24 set

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): RGB565 bytes are swapped in i80 hardware instead of `swap_bytes = 1` (lvgl-byte-swap and the reusable code); the pixel clock stays at 10 MHz with a ceiling of about 15 MHz (pclk-history suggests testing up to 16 MHz); the CPU runs at 160 MHz with PSRAM enabled (board-config); NTP uses 1.ntp.vnix.vn and pool.ntp.org, and local time comes from the fixed UTC+7 offset in core/ rather than a TZ string; in deep sleep CS/RD/WR/RST are parked HIGH with DC and D0-D7 LOW (sleep-ghosting-300 parks every LCD pin LOW); a USB plug-in during sleep is noticed at the next wake (usb-detect-via-adc).

Superseded by decision: the owner keeps R13 = 2 kΩ with the 250 mAh cell and accepts about 2.1C (charge-current-note; ADR-0010); GPIO15 is held LOW through deep sleep (power-path-or-gate says no hold is needed); the USB test is GPIO15 LOW then GPIO4, not ADC x2 > 4.4 V (usb-detect-via-adc); the backlight goes off before SLPIN, not after (sleep-ghosting-300, sleep-reference-code); DISPON follows the first frame (disp-on-idf5); the 180-degree rotation is a setting applied at boot, not a Kconfig choice (orientation-gap); vendor datasheets are cited by link and SHA-256 and never stored in the repository (datasheets-in-repo).

Scope: all 261 commits, the three schematic revisions and how the repo is maintained. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**What I did.** I went through all 261 commits of Xinyuan-LilyGO/T-Display-S3 (full bare clone in scratch, `git log --numstat/-p`, `git show`). I also diffed the three schematic PDFs, read the issues the commits link to, and checked LilyGo-Display-IDF, lilygo.cc, the LILYGO wiki, GitHub releases/tags/wiki and FCC records.

**How it is maintained.** In practice one maintainer, Lewis He, has done the work since 2023-03 (133 of 261 commits including merges). Before him it was Micky (2022-07→2023-02), plus LILYGO_L for MIDI/ULP and the LilyGO org account for 3D files and images.
- Commits per year: 46 / 117 / 59 / 4 / 35 (2022–2026), with an 11-month silence from 2025-03-19 to 2026-02-24.
- There are no tags, releases or wiki, so pin by SHA.
- Fixes are fast and reactive: #352 was reported, answered and fixed on the same day.
- A stale bot closes issues after 30+14 days (128 of 323 issues), so 'closed' does not mean 'fixed'.
- No pin number has changed in any pin_config.h in four years.
- What changes most: examples (94 commits, factory 35), README (60, of which 17 in 2026), platformio.ini, CI, vendored lib/.

**Timeline.**
- 2022-07: first code; GPIO15 HIGH from day one; factory pixel clock 6.5 MHz.
- 2022-10: schematic v1.
- 2022-12: `LCD_MODULE_CMD_1` added ('product now has two screens'). In 2023-01 the maintainer said it is needed for glossy panels, not frosted ones (#87).
- 2023-03: pixel clock 20 → 16 MHz ('mosaic'); schematic v2 (16 MB flash, TP4056B, battery fuse); CMD_1 switched off, then back on in 2023-04.
- 2024-01: restructure; README points ESP-IDF users to LilyGo-Display-IDF.
- 2024-02 and 2024-11: battery size limit ≤7×18×50 mm.
- 2024-04: schematic v3 (TP4065, 'Battery power must set IO15 HIGH', and an out-of-date '580 mA' charge note).
- 2024-09: LEDC backlight declared wrong (#278), replaced by pulse-count setBrightness.
- 2024-11: sleep with SLPIN (#289). 2025-03: backlight on only after the first frame (#300).
- 2025-09: PCB 'V1.3' with a re-matched antenna, on sale by 2026-04 (#340). New FCC grant 2026-09-24. **No V1.3 schematic has been published.**

**2026 (35 commits).**
- 16 commits are eight community MIDI pull requests.
- A toolchain catch-up: TFT_eSPI 2.5.43, IDF≥5 `#error` guards removed, CI board target fixed (it had been building for feathers3), LEDC removed, and the factory example fixed for Arduino core 3.3.11 (this added `esp_lcd_panel_disp_on_off`).
- Mostly documentation, driven by issues #326, #343, #344, #345, #340 and #352: README rewrite plus a Chinese README, pinout table, LED meanings, the battery-ADC-with-USB caveat, the Wi-Fi antenna resistor, electrical limits, GPIO15 also needed for 5V-header power, and removal of the wrong V3V claim.
- No schematic, pin or init-table changes.

**Most important for Gogadget.**
1. V1.3 boards exist; record the revision of every unit.
2. There are two panel variants. Matte panels can show ghost images after long deep sleep unless SLPIN is sent and the LCD pins are driven LOW. Turn the backlight on only after the first frame.
3. The deep-sleep floor is partly fixed by hardware: a 100 kΩ resistor (R12) across the battery plus the LDO quiescent current account for about 60–100 µA of the measured 263 µA.
4. The diode-OR power path lets GPIO4 detect USB while GPIO15 is LOW. There is no VBUS or charge-status GPIO, and no USB-C CC resistors, so users need an A-to-C cable.
5. The 2 kΩ charge-programming resistor gives about 2C on a 250 mAh cell.
6. GPIO14 has 100 kΩ + 100 nF (τ ≈ 10 ms). GPIO17/18 have 10 kΩ pull-ups, so never drive them low in sleep.
7. LVGL 9 needs the byte swap turned on, and IDF 5+ needs `disp_on_off` after init.
8. Use a fixed time zone 'ICT-7': no geo-IP lookup, no pinned certificates.

**What to watch and how.**
- Watch: schematic/, README.md, dimensions/README.MD, image/, datasheet/, the factory, Sleep, GetBatteryVoltage, lv_demos and tft examples, Setup206, CI/platformio, firmware/README, and in the IDF repo display_s3.c, initSequence.*, product_pins.h and the T-Display-S3 sdkconfig.
- Ignore lib/ and the MIDI examples.
- Knowledge item `tracking-approach` holds a tested `upstream-watch.sh` and lock file. It diffs the pinned SHA against HEAD, word-diffs PDFs, flags issues with hardware keywords, and exits 10 on drift. Run it monthly.
- Check the lilygo.cc product page and FCC ID 2ASYE-T-DISPLAYS3 once a quarter.

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| hw-v13-revision | Hardware revisions | high | new |  |
| hw-fcc-2026 | Hardware revisions / certification | low | new |  |
| hw-silkscreen-not-bom | Hardware revisions | medium | refines |  |
| sch-three-versions | Schematic history | medium | new |  |
| charge-current-note | Charger / battery safety | high | refines |  |
| power-path-or-gate | Power architecture | high | refines |  |
| usb-detect-via-adc | USB detection for always-on mode | high | refines |  |
| no-charge-status | Charger status | medium | new |  |
| sleep-floor-board | Battery budget / deep-sleep floor | high | refines |  |
| adc-divider-settle | Battery ADC | high | refines |  |
| buttons-rc | Buttons / wake | high | new |  |
| i2c-pullups-sleep | Deep-sleep pin states | medium | new |  |
| strapping-on-lcd-bus | Deep-sleep pin states / boot | medium | new |  |
| sleep-ghosting-300 | Display / deep sleep | high | new |  |
| panel-variants-cmd1 | Display init / panel variants | high | refines |  |
| cmd1-table-content | Display init | medium | refines |  |
| disp-on-idf5 | Display init on IDF ≥5 | high | refines |  |
| orientation-gap | Display geometry | medium | confirms |  |
| pclk-history | Pixel clock | medium | refines |  |
| pins-stable | Pin map stability | medium | confirms |  |
| headers-jstsh | Connectors / headers | low | new |  |
| five-volt-pin | Connectors / power | low | new |  |
| battery-port-overvoltage | Battery connector / safety | medium | new |  |
| battery-recommendation-connector | Battery selection | high | new |  |
| mechanical-models | Dimensions / enclosure | medium | new |  |
| usb-c-no-cc | USB-C power | high | new |  |
| wifi-antenna | RF | medium | new |  |
| led-semantics | LEDs | medium | new |  |
| electrical-params | Electrical limits | low | new |  |
| gpio15-doc-history | Documentation reliability | medium | refines |  |
| backlight-method-history | Backlight driver | medium | refines |  |
| aw9364-algorithm | Backlight driver | high | refines |  |
| sleep-reference-code | Deep sleep reference | high | refines |  |
| ntp-timezone-lessons | Time / NTP | high | new |  |
| toolchain-support-timeline | Toolchain / version support | medium | refines |  |
| lvgl-byte-swap | LVGL integration | high | new |  |
| draw-buffer-upstream | Display buffers | low | confirms |  |
| maintainers-cadence | Maintenance pattern | medium | new |  |
| churn-areas | Where change happens | medium | new |  |
| changes-2026 | 2026 changes and why | medium | refines |  |
| idf-repo-stale | LilyGo-Display-IDF maintenance | medium | new |  |
| licences | Licensing | medium | new |  |
| firmware-recovery | Recovery / factory images | low | new |  |
| board-config | Flash/PSRAM configuration | low | confirms |  |
| spare-pins | Free GPIOs | low | new |  |
| console-on-battery | USB console behaviour | low | new |  |
| datasheets-in-repo | Datasheets | low | new |  |
| product-page-skus | Product page / ordering | medium | new |  |
| lilygo-wiki | Official docs | low | new |  |
| tracking-approach | Upstream tracking | high | new |  |

### hw-v13-revision: Hardware revisions

Relevance: high. Novelty: new.

**Fact:** A board revision V1.3 exists and is what lilygo.cc now ships. Its PCB silkscreen reads 'T_Display_S3 25-9-12 V1.3' (PCB date 2025-09-12). Maintainer quotes: 'The latest version is specifically optimized for the antenna'; 'the antenna has been specially matched'; 'The only difference is the optimized matching of the built-in antenna on the PCB' (said while comparing external-antenna use); 'When placing your order at lilygo.cc, simply specify that you need the latest v1.3 version ... we can't guarantee the version if you purchase from a lower-level distributor'. Older boards marked '22-8-2 v1.2 XX2' have weaker onboard-antenna Wi-Fi (one customer reported this on about 100 units). The repo contains no V1.3 schematic and no README text about V1.3; the schematic last changed on 2024-04-29.

**Evidence:** Issue #340 comments 4233316310 (2026-04-13), 4243130220 (2026-04-14), 4485639960 (2026-05-19; attached photo shows silkscreen '25-9-12 V1.3') and 4538655808 (2026-05-26). image/WiFi.png (added 3c048f7, 2026-08-10) is a crop of the same board photo (same ESP32-S3 marking). schematic/T_Display_S3.pdf last touched in b0a5ecd (2024-04-29).

**Action:** Treat V1.3 as the expected board for new purchases and keep supporting v1.2. Log the silkscreen revision and date code of every Gogadget unit. Buy directly from lilygo.cc and ask for v1.3. Re-measure sleep current and backlight behaviour on a V1.3 unit before assuming parity. Make Wi-Fi/NTP retries tolerant of weak v1.2 antennas. Watch schematic/ for a V1.3 PDF.

### hw-fcc-2026: Hardware revisions / certification

Relevance: low. Novelty: new.

**Fact:** A new FCC grant for 'T-Display-S3' is dated 2026-09-24 under FCC ID 2ASYE-T-DISPLAYS3 (no hyphen; model T-DISPLAYS3). It includes a new Antenna Specification, separate BLE and Wi-Fi test reports, internal and external photos, and a user manual. The original grant, ID 2ASYE-T-DISPLAY-S3 (the one printed on board labels), is dated 2022-12-12. The timing fits a re-certification of the V1.3 antenna rework; that link is an inference.

**Evidence:** https://fccid.io/2ASYE-T-DISPLAYS3 and https://fccid.io/2ASYE-T-DISPLAY-S3 (grantee Shenzhen Xin Yuan Electronic Technology Co., Ltd.). The label 'FCC ID: 2ASYE-T-DISPLAY-S3' is visible in image/WiFi.png@1cc41eb.

**Action:** Download the 2026 internal photos and user manual manually (fccid.io returned HTTP 403 to automated fetches). Compare them with v1.2 to look for V1.3 changes beyond the antenna: charger, LDO, R12, power path.

### hw-silkscreen-not-bom: Hardware revisions

Relevance: medium. Novelty: refines.

**Fact:** The silkscreen version does not reliably identify the bill of materials. The official 2022 pinmap photo shows 'T_Display_S3 22-5-18 V1.0'. Users had PCBs marked V1.2 by October 2022. The charger later changed from TP4056B to TP4065 under the same V1.2 marking; the maintainer said 'There is no other difference except that the charging chip has changed'. In 2026 the maintainer said the '22-8-2 v1.2 XX2' marks 'are just the silkscreen on the PCB. Newer PCBs would also have a production date.'

**Evidence:** image/T-DISPLAY-S3.jpg@758b2a3 (2022-07-01). Issue #24 body (2022-10-12): 'My PCBs are marked V1.2 on the silk screen'. #236 lewisxhe 2024-04-29. #340 comment 4235508372.

**Action:** Identify each unit by inspection, not by the 'V1.2' text: charger package (SOT-23-5 TP4065 vs SOP-8 TP4056) near USB-C, the antenna matching network next to the IPEX connector, and any production date code.

### sch-three-versions: Schematic history

Relevance: medium. Novelty: new.

**Fact:** The main-board schematic has had three content versions, none with a revision field.

(a) 2022-10-10, first committed in 'shcematic/' and fixed to 'schematic/' by PR #27. Shows W25Q32 flash, a TP4054 charger with 2 kΩ PROG, a red V3V LED, a greyed-out WS2812B block, and strapping notes.

(b) 2023-03-13. W25Q128JV (16 MB) flash, TP4056B charger, green V3V LED, a fuse R10 labelled '5V/2A保险丝' in series with the battery connector, crystal load caps changed from 22 pF to 12/15 pF, antenna matching parts (2.0 nH, 22 nH, 3 pF), and the WS2812B block and strapping notes removed. Page 2 is an empty BOM table.

(c) 2024-04-29. TP4065 charger, the note 'Battery power must set IO15 to HIGH', a dashed note 'R13 IBAT (mA) = 580', and section titles (Button/ADC/SOC/Display/GPIO pin header/Charger/Power).

**Evidence:** Commits 410f030 (2022-10-10), c19f3ad / PR #27, 811c949 (2023-03-13) and b0a5ecd (2024-04-29). Word-set diffs of `pdftotext -raw` output for `git show <commit>:schematic/T_Display_S3.pdf`. PDF CreationDate metadata matches the dates.

**Action:** Archive the 2024-04-29 PDF (blob at b0a5ecd) in Gogadget docs as the v1.2 hardware baseline, because upstream overwrites the same filename. Detect future changes with a word diff (the tracking script does this).

### charge-current-note: Charger / battery safety

Relevance: high. Novelty: refines.

**Fact:** The v1.2 schematic note 'R13 IBAT (mA) = 580' (R13 = 2 kΩ on PROG) matches the TP4056 datasheet table and was kept after the IC changed to TP4065. The README says 'The default charging current is 500mA'. An LTC4065-style PROG law (about 1000 V / RPROG) also gives about 500 mA at 2 kΩ. Either way, a 250 mAh cell charges at about 2C. In #230 a user calculated 580 mA from the TP4056 datasheet; the maintainer pointed to R13 and said 'I asked and learned that TP4065 is used instead of TP4056'.

**Evidence:** Schematic b0a5ecd, Charger section (U6 TP4065, R13 2K, dashed note). README.md:238@1cc41eb (FAQ added c60ce87, 2024-03-05). Issue #230, lewisxhe 2024-03-05.

**Action:** Choose one: a 250 mAh protected cell explicitly rated for 2C charging, or replace R13 with about 4 kΩ (~250 mA) to 8 kΩ (~125 mA). Confirm the formula against the TP4065 datasheet, then measure the actual charge current. Record the R13 value in the Gogadget build docs.

### power-path-or-gate: Power architecture

Relevance: high. Novelty: refines.

**Fact:** Power path per the 2024-04-29 schematic:
- Battery to system rail: VBAT reaches the +5V rail through P-MOSFET Q4 (SI2307), whose gate is tied to VBUS with R14 100 kΩ to GND.
- USB to system rail: VBUS reaches +5V through Schottky D3 (1N5819).
- Always-on 3.3 V: +5V feeds U7 'AP2112K-3.3V(RT9013)' (EN pulled up by R11 100 kΩ), giving VDD3V3.
- Switched rails: NPN Q1 (PMBT3904) is driven by a diode-OR of VBUS (D2 1N4148) and PWR_EN/GPIO15 (D1 1N4148), with R7 100 kΩ to GND. Q1 turns on Q2 (SI2301), which gives 'BAT' (switched +5V, feeding the battery ADC divider), and Q3 (SI2301), which gives 'V3V' (switched 3V3, feeding AW9364 VIN and the green LED).

Consequences: with USB present the switched rails are on whatever GPIO15 does. This explains the README lines 'In USB mode, the green indicator stays on' and 'powered by battery or 5V Pin Header, GPIO15 must be set to HIGH'. In deep sleep, R7 keeps the switch off even if GPIO15 floats.

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd: Power section (Q1–Q4, D1–D3, R5–R8, R11, R14, U7) and Display section (U4 AW9364DNR, VIN = V3V, EN = ESP_LCD_BLK). README.md:253-256@1cc41eb (dbd46ad, 1cc41eb). README.md:163,192 (ec889e7, 2026-08-31).

**Action:** In the power manager, set GPIO15 HIGH on every wake before using the backlight or ADC, and LOW before sleep. No gpio_hold is needed for GPIO15 because R7 pulls Q1 off. Do not expect savings from GPIO15 LOW while on USB. Do not treat GPIO15 as panel power: the panel and header 3V stay powered (known, #352).

### usb-detect-via-adc: USB detection for always-on mode

Relevance: high. Novelty: refines.

**Fact:** There is no GPIO for VBUS or charger status. The diode-OR above still makes GPIO4 (BAT_ADC = RTC_GPIO4 / ADC1_CH3) a USB detector:
- On battery with GPIO15 LOW, the divider is unpowered and the pin reads about 0 V.
- On USB the divider is fed from +5V ≈ VBUS − Vf(1N5819) ≈ 4.6–4.7 V, so the pin reads about 2.3 V.
- On battery with GPIO15 HIGH, the pin reads VBAT/2.

The README (2026-07-31) adds that the battery voltage cannot be read while USB-C is plugged in.

**Evidence:** Schematic @b0a5ecd: ADC section (R2/R4 100K 1%, C13 10nF; divider top is the switched net 'BAT') and Power section. README.md:170@1cc41eb (a68b5b3). Known #190 / #254 (4.6 V reading with USB).

**Action:** Implement usb_present() as ADC(GPIO4) × 2 > ~4.4 V. On battery, sample with GPIO15 LOW for presence and HIGH for VBAT. Plugging in USB cannot wake the chip from deep sleep, because there is no VBUS GPIO. Either add an occasional timer wake, or a ULP poll of ADC1_CH3 that wakes the CPU when the reading jumps, so Gogadget can enter always-on mode without a button press. Note that usb_serial_jtag_is_connected() detects a USB host, not a charger.

### no-charge-status: Charger status

Relevance: medium. Novelty: new.

**Fact:** The TP4065 CHRG pin drives only the red LED (LED1, with R9 2 kΩ to VBUS); no GPIO sees charge or standby state. Red LED meanings per the README: 'Flashes or dimly lights when no battery is connected', 'Remains on while charging', 'Turns off when fully charged'.

**Evidence:** Schematic @b0a5ecd, Charger section. README.md:240-251@1cc41eb (dbd46ad, 2026-06-08).

**Action:** The UI can show 'USB power' (from usb_present()) but not 'charging' or 'full'. Showing those would need a hardware mod, for example wiring CHRG to a free GPIO such as GPIO21 with a pull-up.

### sleep-floor-board: Battery budget / deep-sleep floor

Relevance: high. Novelty: refines.

**Fact:** Two board loads drain the battery permanently. R12 (100 kΩ from VBAT to GND) draws about 37–42 µA. The always-on LDO U7 adds its quiescent current: the schematic allows AP2112K-3.3 (about 55 µA typical) or RT9013 (about 25 µA typical); these are vendor datasheet figures, not measured here. Together that is roughly 60–100 µA of the 263 µA deep-sleep figure (#289) that firmware cannot remove. At 263 µA the floor is about 6.3 mAh per day, roughly 40 days on 250 mAh with no wakes.

**Evidence:** Schematic @b0a5ecd, Power section: 'R12 100K' on the VBAT (3.7V~4.2V) node; 'U7 AP2112K-3.3V(RT9013)'. Known #289 measurement.

**Action:** Use about 260 µA as the sleep floor in the battery model and spend optimisation effort on wake energy: time to first pixels, backlight level, the 10 s on-time, and Wi-Fi/NTP frequency. Measure your own unit, since the LDO part can differ. Consider removing R12 only after asking LilyGO what it is for (probably battery detection or a bleeder).

### adc-divider-settle: Battery ADC

Relevance: high. Novelty: refines.

**Fact:** The battery divider is R2 = R4 = 100 kΩ 1% with C13 = 10 nF on BAT_ADC (GPIO4). Source impedance is 50 kΩ, so τ ≈ 0.5 ms. On battery the divider top is switched by GPIO15. Upstream reads it two ways:
- GetBatteryVoltage uses esp_adc_cal at 11 dB, multiplies by 2, and shows 'No battery connect!' above 4300 mV (comment: 'the ADC detects the charging voltage of TP4056').
- factory uses an uncalibrated (analogRead*2*3.3*1000)/4096.

**Evidence:** Schematic @b0a5ecd, ADC section. examples/GetBatteryVoltage/GetBatteryVoltage.ino:75-95@1cc41eb (added 4b8e1ee, 2023-10-12). examples/factory/factory.ino:424@1cc41eb.

**Action:** In ESP-IDF 6.1, use adc_oneshot plus adc_cali curve fitting with ADC_ATTEN_DB_12 on ADC_CHANNEL_3. After setting GPIO15 HIGH, wait at least 3–5 ms (about 7τ) before sampling. Average 8–16 samples and multiply by 2. Treat readings above about 4.3–4.4 V as 'USB present', not as a battery level.

### buttons-rc: Buttons / wake

Relevance: high. Novelty: new.

**Fact:** GPIO0 (BOOT, S2) has R1 10 kΩ pull-up to VDD3V3 and no capacitor. GPIO14 (KEY, S3) has R3 100 kΩ pull-up and C14 100 nF to GND, giving τ ≈ 10 ms: slow edges and a built-in RC debounce. RST (CHIP_PU) has 10 kΩ and 1 µF. Both button pull-ups go to the always-on VDD3V3, so ext0/ext1 wake works without internal pull-ups. Holding a button costs about 330 µA (GPIO0) or about 33 µA (GPIO14).

**Evidence:** Schematic @b0a5ecd, Button section (R1 10K, R3 100K, C14 100nF, RW 10K, CU 1uF/6.3V).

**Action:** In the two-button driver, wake with ext1 on GPIO0 and GPIO14 (any-low); ext0 takes only one pin, and upstream wakes on GPIO14 only. Disable internal pulls. Debounce GPIO14 for at least 20–30 ms after release, since the rise takes about 10–20 ms. Do not enter sleep while a button is held, and detect stuck buttons.

### i2c-pullups-sleep: Deep-sleep pin states

Relevance: medium. Novelty: new.

**Fact:** GPIO17 (SCL) and GPIO18 (SDA) have 10 kΩ pull-ups (RIIC, RIID), and GPIO16 (TP_INT) has 100 kΩ (RINT), all to the always-on VDD3V3. They are fitted on non-touch boards too. Maintainer: 'Pull-up resistors are already added to the 16 and 17 ... pulled high by default' and 'If you're not using the touch version, you can remove them.' Driving 17 or 18 LOW during sleep costs about 330 µA each.

**Evidence:** Schematic @b0a5ecd, Display section (RIIC 10K, RIID 10K, RINT 100K to VDD3V3). Issue #313, lewisxhe 2025-07-29 and 2025-11-05. #346, lewisxhe 2026-08-07: 'GPIO 21 or 17 ... not limited in the non-touch version'.

**Action:** In deep sleep, leave GPIO16, 17 and 18 as inputs without pull-downs and never hold them low. An I2C add-on can rely on the existing 10 kΩ pull-ups.

### strapping-on-lcd-bus: Deep-sleep pin states / boot

Relevance: medium. Novelty: new.

**Fact:** The 2022 schematic notes, translated from Chinese: 'GPIO0 ---- default pull-up; GPIO45 ---- default pull-down; GPIO46 ---- default pull-down; GPIO46(1) ----- GPIO0(0) cannot be used'. GPIO45 and GPIO46 are LCD D4 and D5, so two strapping pins sit on the parallel bus.

**Evidence:** Schematic 410f030 (2022-10-10), top-right note (removed in 811c949). Pinout at README.md:140-141@1cc41eb.

**Action:** When parking LCD pins before deep sleep, park them LOW. Never hold or pull up GPIO45 or GPIO46: a chip reset with GPIO46 = 1 while BOOT/GPIO0 is pressed is an invalid strapping combination.

### sleep-ghosting-300: Display / deep sleep

Relevance: high. Novelty: new.

**Fact:** On matte-panel units, after a few minutes of deep sleep on battery (the panel stays powered), the screen showed white borders or smeared remains of the last image after wake. The reporter said glossy-panel units did not show this.
- Maintainer's first advice: 'After the screen is initialized, refresh the screen to all black, and then turn on the backlight last.'
- The maintainer then reworked examples/Sleep: SLPIN 0x10 before sleep, a 15 s timer wake, and the backlight turned on only after pushImage.
- The reporter's working fix (MicroPython, which had no SLPIN): drive BL, D0–D7, GPIO15, RST, CS, DC, WR and RD LOW before deep sleep.

**Evidence:** Issue #300 comments 2709985067, 2723764308, 2733096179, 2735459793, 2748842066. Commit fc466c6 (2025-03-19): examples/Sleep/Sleep.ino:73-74,97-116@1cc41eb.

**Action:** Sleep sequence: optionally DISPOFF (0x28), then SLPIN (esp_lcd_panel_disp_sleep, or tx_param 0x10), wait at least 5 ms, set backlight EN low, set GPIO15 LOW, drive LCD pins 5,6,7,8,9,39–42,45–48 LOW, then gpio_hold_en plus gpio_deep_sleep_hold_en, then deep sleep. On wake: gpio_hold_dis, re-init the panel, render the first LVGL frame, then ramp the backlight. Add an overnight soak test.

### panel-variants-cmd1: Display init / panel variants

Relevance: high. Novelty: refines.

**Fact:** There are two ST7789 panel variants.
- CMD_1 was introduced 2022-12-20 with the comment 'The product now has two screens, and the initialization code needs a small change in the new version'.
- In #87, asked which boards need it, maintainer mmMicky replied: 'LCD_MODULE_CMD_1 is suitable for optimizing screen display... Theoretically observe the screen surface, if it is smooth, it needs to be defined. If it is frosted, no definition is required.'
- Examples switched CMD_1 off on 2023-03-21 ('default display version') and back on: factory 2023-04-17, lv_demos and tft 2023-05-22, nes and touch_test 2024-02-01. It is always on today.
- #300 reports glossy panels (K204 shell) and matte panels (H569 bought on AliExpress).

**Evidence:** Commits 772b8d8 (2022-12-20), 84e8604 and 2d23043 (2023-03-21), 5af4f76 (2023-04-17), fafcb0b (2023-05-22), 76ca887 (2024-02-01). Issue #87, mmMicky 2023-01-30. #300 body and comment 2733096179.

**Action:** Keep CMD_1 on by default, as upstream and the IDF repo do, but make it a Kconfig option. Compare each unit with CMD_1 on and off (gamma, porch, flicker) and log the panel finish (glossy or matte) next to the board revision.

### cmd1-table-content: Display init

Relevance: medium. Novelty: refines.

**Fact:** CMD_1 table, in order:
- 0x11 (SLPOUT), then 120 ms; the delay is flagged by len|0x80.
- 0x3A = 05
- 0xB2 = 0B 0B 00 33 33
- 0xB7 = 75, 0xBB = 28, 0xC0 = 2C, 0xC2 = 01, 0xC3 = 1F, 0xC6 = 13
- 0xD0 = A7, then 0xD0 = A4 A1
- 0xD6 = A1
- 0xE0 and 0xE1: 14-byte gamma tables.

COLMOD 0x3A = 05 was added 2023-01-14; the first version had '// {0x3A,{0X06},1}'. The table has no MADCTL and no DISPON. Factory sends it after esp_lcd init, invert, swap, mirror, gap and disp_on_off.

Four examples (GetBatteryVoltage, nes, touch_test, usb_hid_pad) still loop with `j < lcd_st7789v[i].len & 0x7f`. Because of operator precedence this sends 128 data bytes after SLPOUT. Sleep and tft use the correct '(len & 0x7f)'.

**Evidence:** examples/factory/factory.ino:126-150 and 325-331@1cc41eb. Commit 65db476 (2023-01-14). examples/GetBatteryVoltage/GetBatteryVoltage.ino:57, examples/nes/nes.ino:80, examples/touch_test/touch_test.ino:83, examples/usb_hid_pad/usb_hid_pad.ino:94, compared with examples/Sleep/Sleep.ino:59@1cc41eb.

**Action:** Port the table verbatim as a const array with an explicit delay field instead of the 0x80 flag. Send it after esp_lcd_panel_init and before the first flush, and issue DISPON (esp_lcd_panel_disp_on_off) last. Do not copy the buggy loop.

### disp-on-idf5: Display init on IDF ≥5

Relevance: high. Novelty: refines.

**Fact:** Commit c9b939f (2026-07-28, 'Fixed compilation errors and display issues in Arduino esp32-core v3.3.11', which runs on ESP-IDF 5.x) added esp_lcd_panel_disp_on_off(panel_handle, true) after set_gap. Under IDF 5+, esp_lcd_panel_init() no longer turns the ST7789 on. The same commit moved SNTP to esp_sntp_servermode_dhcp(). examples/lv_demos still lacks this call and still has '#error ... use a version below Arduino ESP32 3.0'.

**Evidence:** c9b939f. examples/factory/factory.ino:324,462-468@1cc41eb. examples/lv_demos/lv_demos.ino:3-5@1cc41eb.

**Action:** In the IDF 6.1 display bring-up, call esp_lcd_panel_disp_on_off(panel, true) explicitly after init and CMD_1. Keep the backlight off until the first LVGL flush completes. Do not use lv_demos as a reference (IDF 4.4 only).

### orientation-gap: Display geometry

Relevance: medium. Novelty: confirms.

**Fact:** Upstream landscape setup: esp_lcd_panel_invert_color(true), swap_xy(true), mirror(false, true) for 'The screen faces you, and the USB is on the left', mirror(true, false) for USB on the right, and set_gap(0, 35). The 2026 LovyanGFX examples use offset_x = 35, offset_rotation = 1 and invert = true.

**Evidence:** examples/factory/factory.ino:310-322@1cc41eb. examples/T-Display-S3-Piano/PianoDisplay.h:96-110@1cc41eb.

**Action:** Add a Kconfig choice for USB-left or USB-right that maps to the two mirror settings, with gap (0, 35) in both. Use the same orientation in the simulator screenshot tests.

### pclk-history: Pixel clock

Relevance: medium. Novelty: refines.

**Fact:** Factory pixel clock: 6.528 MHz in the first upload (2022-07-01, '(6528000) //(10 * 1000 * 1000)'); 20 MHz from 58bd641 (2022-12-08); 16 MHz from adc038e (2023-03-03), with the comment 'Too low or too high pixel clock may cause screen mosaic'. lv_demos still uses 6.528 MHz and LilyGo-Display-IDF uses 10 MHz. The 20 MHz setting was abandoned.

**Evidence:** examples/factory/pin_config.h:24-25@1cc41eb. 758b2a3:example/arduino/factory/pin_config.h. Commits 58bd641 and adc038e. examples/lv_demos/pin_config.h:14@1cc41eb.

**Action:** Keep 10 MHz. If more frame rate is needed, test up to 16 MHz with full-screen gradients and a soak test; never use 20 MHz. Keep the value in Kconfig.

### pins-stable: Pin map stability

Relevance: medium. Novelty: confirms.

**Fact:** Across all 261 commits, no base-board GPIO number changed in any pin_config.h. The only value changes were EXAMPLE_LCD_PIXEL_CLOCK_HZ and GMT_OFFSET_SEC, and PIN_SD_CLK differs between the MIDI-shield config (21) and the SD shield (11).

**Evidence:** `git log -p -- '*pin_config.h'` over the full history (26 commits, 2022-07-01 to 2026-07-28), parsed for #define value changes.

**Action:** Hardcode the pin map in one board header; the risk of upstream pin changes is negligible. Recheck only when a new revision schematic appears.

### headers-jstsh: Connectors / headers

Relevance: low. Novelty: new.

**Fact:** The 4-pin JST-SH 1.0 mm connector next to USB-C carries G, 3V, GPIO43 and GPIO44 (UART0 TX/RX), not the I2C pins 17/18, although lilygo.cc calls it 'STEMMA QT / Qwiic'.
- Header P2: +5V, GND, NC, NC, GPIO13, 12, 11, 10, 3, 2, 1, 3V3.
- Header P1: 3V3, GND, GND, NC, GPIO16, 21, 17, 18, 44, 43, GND, GND.

**Evidence:** Schematic @b0a5ecd, GPIO pin header section (CNC WAFER-100W4A; P1 and P2 Header12). image/T-DISPLAY-S3.jpg ('G 3V 43 44'). lilygo.cc product page.

**Action:** Use the JST-SH port as a spare UART debug console, or remap I2C onto GPIO43/44 if a Qwiic sensor is ever added.

### five-volt-pin: Connectors / power

Relevance: low. Novelty: new.

**Fact:** The header pin labelled '5V' is the +5V system rail after the power path: VBUS minus the diode drop on USB, and VBAT (3.7–4.2 V) on battery. The README electrical table ('VBUS (pin header) 5V'; 'The 5V pin header power source comes from USB-C') is therefore incomplete. In #205 the maintainer first said '5V = USB 5V ... the same line without any isolation', then agreed with a user who found the Schottky diode between VBUS and VIN ('Oh! Yes ... it can be used without an isolator'). Powering through the 5V pin requires GPIO15 HIGH (ec889e7).

**Evidence:** Schematic @b0a5ecd (P2 pin 1 '+5V'; D3 1N5819; Q4 SI2307). README.md:264-275@1cc41eb (ca3263c, 9e7bdc3). Issue #205, lewisxhe 2023-12-01 and 2023-12-11. Commit ec889e7.

**Action:** Do not expect 5 V on that pin when running on battery. Treat external 5 V via the 5V pin as equivalent to USB, with GPIO15 HIGH.

### battery-port-overvoltage: Battery connector / safety

Relevance: medium. Novelty: new.

**Fact:** In 2026 the maintainer said that feeding 5 V into the battery connector while USB-C was connected 'most likely burned out the TP4054 charging chip, but shouldn't have damaged the LDO'. He also suggested replacing a damaged U7 with any LDO 'with an output current greater than 700mA'. This contradicts a community claim in #205 (2023) that 3–6 V on the battery connector is fine. It also shows the maintainer's charger part naming is inconsistent (TP4054 vs TP4065).

**Evidence:** Issue #343, lewisxhe 2026-06-04, 2026-06-05 and 2026-06-08. Issue #205, teastainGit 2023-11-30.

**Action:** Gogadget user docs should state: only a single protected 1S LiPo on the battery connector, never a bench supply or a 5 V source. Check cable polarity before plugging in; the only protection is the R10 fuse.

### battery-recommendation-connector: Battery selection

Relevance: high. Novelty: new.

**Fact:** Official cell size limit: 'less than thick: 7mm, wide: 18mm and long: 50mm'.
- LilyGO added it to the README FAQ on 2024-02-26; lewisxhe replaced it with the charge-current FAQ on 2024-03-05.
- It returned on 2024-11-25 in dimensions/README.MD as 'Battery Recommendation:3.7V Li-Po battery, 7mm wide 18mm long 50mm'.
- Example cells pictured: CL 501646 (380 mAh), 701439 (350 mAh) and 601245P (400 mAh), all with protection boards.
- Connector: 'JST 1.25 mm' in the README (2026-08-19); 'JST-GH 1.25mm 2PIN' on lilygo.cc and the wiki.
- Shell variants K204-01 and K206-01 came with a built-in 700 mAh battery (now listed as unavailable).

**Evidence:** Commits 3a54349, 8651ce0 and 4c198a9 (2024-02-26), c60ce87 (2024-03-05), 5573a38 and 075b109 (2024-11-25). dimensions/README.MD:3@1cc41eb. image/image1.jpg, image2.jpg, image3.png. README.md:270@1cc41eb (9e7bdc3). https://lilygo.cc/products/t-display-s3 and https://wiki.lilygo.cc/products/t-display-series/t-display-s3/.

**Action:** Gogadget's 250 mAh protected cell fits inside 7×18×50 mm. Measure the board connector to tell genuine latching JST-GH from MX1.25 ('JST 1.25'), and fix the lead polarity on the cell side before first use.

### mechanical-models: Dimensions / enclosure

Relevance: medium. Novelty: new.

**Fact:** Model dimensions:
- PCB STEP vertex bounding box: 61.98 × 26.09 × 7.86 mm (dimensions/t-display-s3-pcb.stp; Creo product 'T_DISPLAY_S3', 2022-09-07).
- Non-touch STL (dimensions/t-display-s3-full.stl, formerly 3D_file/noTouch.stl): 61.98 × 26.31 × 9.88 mm.
- Official shell STLs (cova, covb): 32 × 70 mm footprint, about 20 mm total height if the parts share one coordinate frame.
- The shell STEP assembly names the LCD 'LCD-NFP190B', and the touch-screen model is 'NFP190B-01A', so the panel module is NFP190B.
- PCB.dwg (formerly 3D_file/Dimensions.dwg) exists only as DWG.
- The 2024-01-31 rename dropped LCD_3D.stp, MainBoard_3D.stp and the community 'LILYGO T-Display-S3 H569 Not Soldered.step' (PR #165) from the tree; they can be recovered from 899de7a^.

**Evidence:** dimensions/*@1cc41eb (bounding boxes computed from STEP VERTEX_POINT coordinates and binary STL vertices). Commit 899de7a (2024-01-31) rename. `git ls-tree 899de7a^ -- 3D_file`.

**Action:** Base the Gogadget enclosure on t-display-s3-pcb.stp plus the LCD model. Recover the H569 STEP with `git show '899de7a^:3D_file/LILYGO T-Display-S3 H569 Not Soldered.step'`. Take mounting-hole positions from PCB.dwg in a CAD viewer.

### usb-c-no-cc: USB-C power

Relevance: high. Novelty: new.

**Fact:** USB-C pins A5 (CC) and B5 (VCONN/CC2) are unconnected in all three schematics, so there are no 5.1 kΩ Rd pull-downs. Compliant USB-C sources and C-to-C cables may therefore not supply VBUS. User reports:
- #65: the board 'lacks any connections to CC1 or VConn', and compliant sources have 'a hard time supplying the board'.
- #166: C-to-C cable gives an unrecognized device.
- #233: 'If you use a usb-c to usb-c cable it doesn't work. Needs to be usb-c to usb-a'.

lilygo.cc lists the included cable as 'USB2.0 (USB A-USB C)'.

**Evidence:** Schematic @b0a5ecd, Charger section (U5 TYPE-C, A5/B5 stubs unconnected). Issues #65 (2023-04-24), #166 (2023-06-28), #233 (2024-03-16). lilygo.cc product page.

**Action:** Tell Gogadget users to use a USB-A to USB-C cable or a charger with an A port for always-on mode. Test once with a C-only charger and document the failure mode.

### wifi-antenna: RF

Relevance: medium. Novelty: new.

**Fact:** README Wi-Fi antenna section (2026-08-10): 'The default is onboard antenna. If you need to use an external antenna, you need to rotate the resistor ... to switch to external antenna mode.' The maintainer calls it 'the 0201 resistor' (#340). The board has an IPEX/U.FL connector. The v1.2 onboard antenna performs worse than the v1.3 one (#340).

**Evidence:** README.md:258-262@1cc41eb (3c048f7). image/WiFi.png. Issue #340 comment 4485639960.

**Action:** Keep the onboard antenna for NTP-only use. Give the Wi-Fi connect step RSSI logging, backoff and a hard time budget so a weak v1.2 antenna cannot drain the battery. Log any antenna mod in the board log.

### led-semantics: LEDs

Relevance: medium. Novelty: new.

**Fact:** Red LED = charge indicator (TP4065 CHRG). Green LED = 'V3V indicator', fed through 2 kΩ from V3V (about 0.6 mA). The green LED 'stays on' in USB mode; on battery it follows GPIO15 ('lights up when high and turns off when low').

**Evidence:** README.md:240-256@1cc41eb (dbd46ad, 2026-06-08, added right after issue #343). Schematic @b0a5ecd: LED8 Green, 2K, on V3V.

**Action:** Use the green LED as a free power-state probe during development: it is on when GPIO15 is HIGH or USB is present. Budget about 0.6 mA for it in every awake period on battery.

### electrical-params: Electrical limits

Relevance: low. Novelty: new.

**Fact:** README 'Electrical parameters' (added 2026-08-19):
- USB-C input 5 V; battery 3.7–4.2 V; battery connector 'JST 1.25 mm'.
- '3V pin header load does not exceed 100mA'.
- 'The 5V pin header power source comes from USB-C, and the load capacity depends on the USB-C adapter'.

**Evidence:** README.md:264-275@1cc41eb (ca3263c, 9e7bdc3).

**Action:** Keep external loads on the 3V pin under 100 mA; the same LDO also carries ESP32-S3 Wi-Fi peaks and the backlight driver.

### gpio15-doc-history: Documentation reliability

Relevance: medium. Novelty: refines.

**Fact:** GPIO15 history:
- GPIO15 HIGH is in setup() from the first upload (758b2a3, 2022-07-01).
- README FAQ 'The screen does not light up when using battery' added 2024-02-01 (bb3701b).
- IMPORTANT note added 2026-07-17 (6fe4d51) and extended to the 5V header on 2026-08-31 (ec889e7).
- A wrong line, 'V3V controls the V3V output of the header pins and provides power to the screen', was added 2026-06-08 (dbd46ad) and removed 2026-10-13 (1cc41eb) after #352. The maintainer: 'There was an error in the documentation—that was my mistake'.

**Evidence:** Commits listed above. Issue #352, lewisxhe 2026-10-13.

**Action:** Treat README power statements as secondary to the schematic. Read README diffs in every upstream review, because hardware facts are corrected there, often within days of an issue.

### backlight-method-history: Backlight driver

Relevance: medium. Novelty: refines.

**Fact:** Examples drove the backlight with LEDC PWM on GPIO38 from 2022. In #278 (2024-09-23) the maintainer said 'The method of ledc controlling the backlight of display-S3 is wrong. I have modified it to the correct control method' and added the pulse-count setBrightness() to factory (2d3c294). Commit 02f0e9d (2026-03-09, known) cleaned up the remaining examples. The eight community MIDI examples merged in Feb–Mar 2026 still configure LovyanGFX Light_PWM (freq 22000, pwm_channel 7) and call setBrightness(255) before forcing the pin HIGH.

**Evidence:** Issue #278, lewisxhe 2024-09-23. Commits 2d3c294 and 02f0e9d. examples/T-Display-S3-Piano/PianoDisplay.h:113 and PianoDisplay.cpp:17-20@1cc41eb.

**Action:** Never port backlight code from the community LovyanGFX examples. Base the AW9364 driver on the factory setBrightness() timing only (see reusable_code).

### aw9364-algorithm: Backlight driver

Relevance: high. Novelty: refines.

**Fact:** Upstream AW9364 handling uses 16 levels:
- value 0: EN low, then wait 3 ms (shutdown).
- Turning on from 0: EN high, wait 30 µs, and assume level 16 (maximum).
- Each low→high pulse steps one level down, wrapping from 1 to 16, so pulses = (16 + current − target) mod 16.
- The Arduino pulse loop has no explicit timing between edges.
- Factory ramps 0→16 in 50 ms steps after LVGL shows the first frame.

**Evidence:** examples/factory/factory.ino:223-247 and 371-380@1cc41eb (2d3c294).

**Action:** In the AW9364 driver, generate pulses with fixed 1–2 µs low and high phases inside a critical section (or with dedicated GPIO / RMT), keeping each low phase far below the chip's off-detect time. Track the level in RTC memory and reset it to 16 after every enable. Ramp only after the first frame.

### sleep-reference-code: Deep sleep reference

Relevance: high. Novelty: refines.

**Fact:** Upstream sleep paths:
- Factory (button 1 click): esp_lcd_panel_io_tx_param(io, 0x10, NULL, 0), then GPIO15 LOW, BL LOW, esp_sleep_enable_ext0_wakeup(GPIO14, 0), then deep sleep. This is the #289 fix (c9fd58a, 2024-11-27).
- examples/Sleep (fc466c6): adds a 15 s timer wake and turns the backlight on after pushImage.
- Neither uses GPIO holds, pin parking or DISPOFF.

**Evidence:** examples/factory/factory.ino:395-404@1cc41eb. examples/Sleep/Sleep.ino:69-89,93-117@1cc41eb.

**Action:** Treat this as the minimum. Gogadget should add ext1 wake on both buttons, pin parking and holds (see sleep-ghosting-300), and the explicit order SLPIN → wait → backlight off → GPIO15 off. Measure the result against the 263 µA reference.

### ntp-timezone-lessons: Time / NTP

Relevance: high. Novelty: new.

**Fact:** Factory time handling:
- The time zone came from https://ipapi.co, checked against a hard-coded certificate chain whose leaf expired 'Mar 29 22:40:08 2025 GMT'. Factory now forces CUSTOM_TIMEZONE 'CST-8' (kept in c9b939f).
- SNTP enables DHCP-provided servers (esp_sntp_servermode_dhcp(true) when LWIP_DHCP_GET_NTP_SRV).
- The time-sync callback calls WiFi.disconnect().

In #297, NTP never synced on a DHCP-assigned address but worked with a static IP or a phone hotspot. The root cause was not established; a DHCP-provided NTP server is a plausible suspect.

**Evidence:** examples/factory/factory.ino:40-45,453-470,717-722@1cc41eb. Commits 4b942a3 (2023-04-17), 7581703 (2025-01-13, #297) and c9b939f. Issue #297, roblatour 2025-01-14.

**Action:** Use a fixed POSIX TZ 'ICT-7' for Vietnam (UTC+7, no DST); no geo-IP lookup and no pinned certificates. Set esp_netif_sntp server_from_dhcp deliberately, preferring or falling back to vn.pool.ntp.org and pool.ntp.org. Bound the sync time and retries, and stop Wi-Fi right after the first good sync.

### toolchain-support-timeline: Toolchain / version support

Relevance: medium. Novelty: refines.

**Fact:** README guidance over time:
- 'SDK 2.0.3 or above' (2022).
- 'Please use Arduino ESP32 version below 3.0' (734abff, 2023-10-13).
- 'Using IDF version 4.4' (2023-10 to 2024-01).
- 'The test phase uses version 2.0.14 ... TFT_eSPI does not work on versions higher than 2.0.14 (TFT_eSPI#3329)' (a9d0f45, 2024-09-09; still at README.md:86).

Build setup:
- PlatformIO pinned to espressif32@6.5.0 since 0e7131a (2024-08-21).
- Arduino CI: 2.0.14 (2024-02), unpinned latest from da301bc (2026-03-09), back to 2.0.17 in 395fa46 (2026-07-16).
- The CI board target (FQBN) was esp32:esp32:feathers3 until 216a520 (2026-03-09).

2026 changes: IDF≥5 '#error' guards removed from 14 examples (a1e3c39) and factory fixed for core 3.3.11 (c9b939f). lv_demos and Arduino_GFXDemo.ino.cpp keep the guard. No upstream example targets ESP-IDF 5.x/6.x directly.

**Evidence:** Commits listed above. README.md:86@1cc41eb. .github/workflows/arduino_ci.yml:53,61@1cc41eb. platformio.ini:39@1cc41eb.

**Action:** Assume LilyGO does not validate anything on IDF 6.1; keep Gogadget's own hardware-in-the-loop smoke test. Watch arduino_ci.yml and the factory example: a move to IDF 5+ APIs there signals behaviour worth re-checking.

### lvgl-byte-swap: LVGL integration

Relevance: high. Novelty: new.

**Fact:** The repo vendors LVGL 8.3.0-dev, untouched since 2022, with lib/lv_conf.h LV_COLOR_DEPTH 16 and LV_COLOR_16_SWAP 1. LilyGo-Display-IDF's sdkconfig.defaults.t-display-s3 sets CONFIG_LV_COLOR_16_SWAP=y. So RGB565 data must be byte-swapped for this 8-bit i80 ST7789 setup, and LVGL 9 no longer has LV_COLOR_16_SWAP.

**Evidence:** lib/lv_conf.h:27,30@1cc41eb. lib/lvgl/library.json (version 8.3.0-dev). LilyGo-Display-IDF sdkconfig.defaults.t-display-s3@b1a1cc5.

**Action:** With LVGL 9.6 and esp_lvgl_port 2.9, set lvgl_port_display_cfg_t.flags.swap_bytes = 1, or call lv_draw_sw_rgb565_swap() in a custom flush. Keep the desktop simulator unswapped so screenshots still match.

### draw-buffer-upstream: Display buffers

Relevance: low. Novelty: confirms.

**Fact:** Factory display setup:
- One full-frame draw buffer (320×170×2 = 108,800 B) with MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL.
- i80 max_transfer_bytes = full frame, trans_queue_depth 20, clk_src LCD_CLK_SRC_PLL160M.
- RD (GPIO9) held HIGH as a plain GPIO.

**Evidence:** examples/factory/factory.ino:257-298,333@1cc41eb. examples/factory/pin_config.h:29@1cc41eb.

**Action:** Gogadget's internal DMA buffers match upstream practice. Consider two partial buffers (for example 320×34 lines) to leave SRAM for Wi-Fi and later BLE. Keep GPIO9 driven HIGH while awake.

### maintainers-cadence: Maintenance pattern

Relevance: medium. Novelty: new.

**Fact:** Repository activity:
- 261 commits, 29 of them merges. By year: 2022 46, 2023 117, 2024 59, 2025 4, 2026 35 (to 2026-10-13). There was an 11-month gap between fc466c6 (2025-03-19) and 1b113d1 (2026-02-24).

Authors:
- Lewis He (lewishe/lewisxhe): 133 commits including merges; sole active maintainer since 2023-03-03.
- Micky/Michy: 44 commits, 2022-07 to 2023-02.
- LILYGO_L (Llgok): 24 commits on the MIDI shield, ULP and audio, 2023-07 to 2024-05.
- LilyGO org account: 30 web uploads (3D files, images, README battery notes).
- About 15 outside contributors.

Repository setup:
- No tags or releases. has_wiki is true but no wiki repo exists. Only one branch (main).
- A stale bot (.github/workflows/issues.yaml, since 2024-03-05) marks issues stale after 30 days and closes them 14 days later; for PRs it is 1 + 1 days. 128 of 323 issues are labelled 'stale'. Labels are otherwise almost unused (wifi-antenna once).

Responsiveness:
- Median first maintainer reply: 33 h (2022), 57 h (2023), 34 h (2024), 4.3 h (2025), 14 h (2026).
- Documentation fixes can land the same day (#352).

**Evidence:** git log on a full clone. gh api contributors, branches, releases, tags and labels. .github/workflows/issues.yaml@1cc41eb. Issue cache of 323 issues with first maintainer comment times.

**Action:** Pin upstream by full SHA. Read a closed issue's thread before trusting it, since 'closed' often just means stale. File short, reproducible issues and expect a fast first reply. Comment every few weeks or the issue auto-closes.

### churn-areas: Where change happens

Relevance: medium. Novelty: new.

**Fact:** Non-merge commits touching each area:
- examples: 94 (factory 35, touch_test 24, tft 19, T-Display-S3-MIDI 15, lv_demos 14, usb_hid_pad 13).
- README: 60 (17 in 2026).
- platformio.ini: 27. 3D_file/dimensions: 26. Vendored lib/: 24. .github: 20. firmware: 17. image: 10.
- schematic: 7 commits but only 3 content versions. boards: 3. datasheet: 1.

Most line churn comes from vendored LVGL 8.3.0-dev and TFT_eSPI (updated to 2.5.43 in f15652b, 2026-03-02).

**Evidence:** `git log --numstat --no-merges` on the full clone.

**Action:** Watch README, schematic, dimensions and image, plus the factory, Sleep, GetBatteryVoltage, lv_demos and tft examples, CI and platformio. Ignore lib/ and the MIDI or community examples.

### changes-2026: 2026 changes and why

Relevance: medium. Novelty: refines.

**Fact:** The 35 commits of 2026, by period:

(1) 02-24 to 03-06: eight community PRs (#328–#336, sauloverissimo) with USB/BLE MIDI examples plus the ESP32_Host_MIDI and Gingoduino libraries; 16 commits including merges, each merged within about a day.

(2) 03-02 to 03-09: TFT_eSPI 2.5.43 after user confusion in #326; IDF≥5 '#error' guards removed (#326: 'This error message can be commented out'); Arduino CI moved to the latest core with the correct board target (FQBN); LEDC backlight code removed.

(3) 06-08 to 07-31:
- LED description, added right after #343.
- Board JSON renamed to the PlatformIO name (#344); boards_dir dropped, so PlatformIO now uses its own lilygo-t-display-s3 board.
- CI re-pinned to 2.0.17.
- README rewrite (#345): Chinese README, pinout table, GPIO15 IMPORTANT note, and a 'Clean before build' tip.
- firmware/ reorganised into dated merged images, plus LILYGO Spark.
- Factory fixed for core 3.3.11 (display-on, esp_sntp, WiFiMulti, SmartConfig timeout).
- Note that the battery ADC can't read with USB plugged in.

(4) 08-10 to 10-13: Wi-Fi antenna section with a V1.3 photo (after #340); electrical parameters; GPIO15 also needed for 5V-header power; wrong V3V/header claim removed (#352).

No 2026 commit touched the schematic, pin definitions, datasheets or the init table.

**Evidence:** `git log --since=2026-01-01` (1b113d1..1cc41eb) and the issue threads cited.

**Action:** In each review, re-read the README sections Pinout, FAQ, LED, WiFi antenna and Electrical. For v1.2/v1.3 behaviour the 2026 documentation changes matter more than the 2026 code changes.

### idf-repo-stale: LilyGo-Display-IDF maintenance

Relevance: medium. Novelty: new.

**Fact:** In LilyGo-Display-IDF, the T-Display-S3 sources (main/display_s3.c, main/initSequence.c/.h, sdkconfig.defaults.t-display-s3, main/main.cpp) have not changed since the first commit c465092 (2024-01-31). Only product_pins.h and Kconfig.projbuild changed since, for other boards (a716e0f 2024-08-12, 6fe03b1 2025-10-02). The latest commit is b1a1cc5 (2025-11-03). display_s3.c already calls esp_lcd_panel_disp_on_off(panel, true).

**Evidence:** Per-file `git log` in the LilyGo-Display-IDF clone. main/display_s3.c:23,145@b1a1cc5.

**Action:** Use it as a cross-check only. A monthly watch is enough; any change to display_s3.c or initSequence.c deserves a full read.

### licences: Licensing

Relevance: medium. Novelty: new.

**Fact:** The repo LICENSE is MIT, 'Copyright (c) 2022 Xinyuan-LilyGO'. factory.ino, Sleep.ino, tft.ino and GetBatteryVoltage.ino have no per-file header, so the repo licence applies. SerialExample, I2CScan and CapacitiveTouch say MIT. examples/ULP_ADC/ULP_ADC.ino, examples/ULP_Count/ULP_Count.ino and examples/WIFI_Audio_I2S/WIFI_Audio_I2S.cpp carry '@License: GPL 3.0'. Vendored libraries keep their own licences, e.g. arduino-nofrendo COPYING.LGPL and ESP32-audioI2S LICENSE.

**Evidence:** LICENSE@1cc41eb. Grep of licence headers in examples/ and lib/*/LICENSE*.

**Action:** Port only from MIT-covered files and keep a NOTICE line: 'portions derived from Xinyuan-LilyGO/T-Display-S3, Copyright (c) 2022 Xinyuan-LilyGO, MIT'. Do not copy the GPL ULP examples; base any ULP ADC poller on Espressif's ESP-IDF ULP examples instead.

### firmware-recovery: Recovery / factory images

Relevance: low. Novelty: new.

**Fact:** firmware/ (reorganised in c22de50, 2026-07-17) holds merged images to flash at 0x0: t-display-s3-no-touch-20231013_0x0.bin, -touch-20230417, -lvgl-demo-20250109, -tft-espi-20231113 and -screen-detect-20230315. These are old builds, not rebuilt from the 2026 source. The README recommends four ways to flash: LILYGO Spark (desktop app for Windows/macOS/Linux with a firmware center that includes community uploads), the ESP Flash Download Tool, the esptool-js web flasher, or `esptool --chip esp32s3 ... write_flash -z --flash_mode dio --flash_freq 80m 0x0 firmware.bin`. The ScreenDetection image only prints the TFT_eSPI pin configuration; it does not probe the panel.

**Evidence:** firmware/README.MD@1cc41eb. Commit c22de50. https://lilygo.cc/pages/lilygo-spark. firmware/images/log.png.

**Action:** Document a recovery path for Gogadget users: flash the factory image to rule out hardware faults. Consider publishing Gogadget releases as merged 0x0 images, the format LILYGO Spark's firmware center already uses.

### board-config: Flash/PSRAM configuration

Relevance: low. Novelty: confirms.

**Fact:** The board JSON specifies memory_type qio_opi (QIO flash at 80 MHz plus OPI PSRAM), default_16MB.csv, 240 MHz, USB VID:PID 303A:1001 (native USB-Serial-JTAG), and openocd 'esp32s3-builtin.cfg' (#210). Since 81e5d5c (2026-07-15) PlatformIO uses the platform's own lilygo-t-display-s3 board and the repo's JSON is unused. The README marks OPI PSRAM as required only for its Arduino examples.

**Evidence:** boards/lilygo-t-display-s3.json@1cc41eb. Commits e53beab (2024-03-05) and 81e5d5c. README.md:94-112@1cc41eb.

**Action:** sdkconfig: 16 MB flash, QIO at 80 MHz, 240 MHz CPU on USB (lower is possible on battery). Leave CONFIG_SPIRAM off unless needed; internal DMA buffers suffice and skipping PSRAM init shortens wake-to-pixels.

### spare-pins: Free GPIOs

Relevance: low. Novelty: new.

**Fact:** GPIO16 (TP_INT) and GPIO21 (TP_RST) are used only on touch boards. SD pins 11/12/13 are used only with the SD shield ('The standard T-Display-S3 board does not have an onboard SD card slot'). Maintainer: 'GPIO 21 or 17 ... are not limited in the non-touch version, but they cannot be used in the touch version' (#346). GPIO21 has no board pull-up; GPIO16, 17 and 18 do (see i2c-pullups-sleep).

**Evidence:** README.md:155-168@1cc41eb (6fe4d51). Issue #346, lewisxhe 2026-08-07. Schematic @b0a5ecd.

**Action:** Reserve GPIO21 (no pull-up), GPIO1–3 and GPIO10–13 for future Gogadget hardware, for example a CHRG sense line or a buzzer. Avoid GPIO16–18 for anything that must sit LOW during sleep.

### console-on-battery: USB console behaviour

Relevance: low. Novelty: new.

**Fact:** README FAQ 5: 'If you use an external power supply instead of USB-C, please disable the CDC option. The board waits for USB access on startup when CDC is enabled.' This is specific to Arduino (ARDUINO_USB_CDC_ON_BOOT). LilyGo-Display-IDF uses CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y.

**Evidence:** README.md:224-232@1cc41eb. LilyGo-Display-IDF sdkconfig.defaults.t-display-s3@b1a1cc5.

**Action:** Check on hardware that Gogadget's USB-Serial-JTAG console never stalls the 10 s battery wake when no host is attached: log timestamps and wake-to-pixel time on battery.

### datasheets-in-repo: Datasheets

Relevance: low. Novelty: new.

**Fact:** datasheet/ holds ST7789V_SPEC_V1.4.pdf, CST328 V2.2 and the CST816S register notes, all added in e662544 (2023-11-10). There are no datasheets for AW9364, TP4065, AP2112K or RT9013. The ESP32-S3 PDFs in doc/ (bumped to TRM v1.3 and datasheet v1.7 by PR #172) were deleted as outdated in da406bd (2024-01-31).

**Evidence:** datasheet/@1cc41eb. Commits e662544, 618b5a0 / cb1a2db and da406bd.

**Action:** Store vendor datasheets for AW9364, TP4065 and AP2112K in Gogadget docs/ and cite them in the backlight timing and charge-current decisions.

### product-page-skus: Product page / ordering

Relevance: medium. Novelty: new.

**Fact:** lilygo.cc SKUs:
- Non-touch: Non-Soldered Pin H569, Soldered Pin H577.
- Touch: Non-Soldered H587, Soldered H589.
- Shells: Black K204, Grey K206; with built-in 700 mAh battery K204-01 / K206-01 (unavailable).
- MIDI board only: H656.

The page says 'the shell is only available for the soldered and non-soldered versions, not for the touch version'. Connectors: 'STEMMA QT / Qwiic: JST-SH 1.0mm 4-PIN' and 'JST-GH 1.25mm 2PIN'. Cable: 'USB2.0 (USB A-USB C)'. The page gives no revision, dimensions, current consumption or charging specs. A screenshot posted in #340 (2026-04) shows a V1.3 board as the H569 listing photo.

**Evidence:** https://lilygo.cc/products/t-display-s3 (fetched 2026-10-20). Issue #340 comment 4240932758 (screenshot).

**Action:** Order H569 or H577 directly from lilygo.cc and ask for v1.3. The official shell STLs are in dimensions/LilyGo-Shell if an off-the-shelf case is acceptable.

### lilygo-wiki: Official docs

Relevance: low. Novelty: new.

**Fact:** The LILYGO wiki page (wiki.lilygo.cc/products/t-display-series/t-display-s3/) mirrors the README as it was before July 2026, including 'The default charging current is set at 500mA per hour' and the 2.0.14 note. Its 'Version History' table is empty, it has no ESP-IDF instructions (only a link to LilyGo-Display-IDF), and 'Last updated' shows no date. It links schematic/T_Display_S3.pdf and dimensions/PCB.dwg.

**Evidence:** https://wiki.lilygo.cc/products/t-display-series/t-display-s3/ (fetched 2026-10-20).

**Action:** Treat the GitHub README and schematic as primary and the wiki as a lagging copy. Check its Version History quarterly in case LilyGO starts documenting v1.3 there.

### tracking-approach: Upstream tracking

Relevance: high. Novelty: new.

**Fact:** Watch list:
- Hardware truth: schematic/; README.md sections Pinout, IMPORTANT, FAQ, LED, WiFi antenna and Electrical; dimensions/README.MD; image/; datasheet/.
- Reference code: examples/factory/{factory.ino, pin_config.h}, examples/Sleep/Sleep.ino, examples/GetBatteryVoltage, examples/lv_demos, examples/tft, lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h.
- Toolchain signals: .github/workflows, platformio.ini, boards/.
- Recovery: firmware/README.MD.
- LilyGo-Display-IDF: main/display_s3.c, main/initSequence.*, main/product_pins.h, main/Kconfig.projbuild, sdkconfig.defaults.t-display-s3.
- Ignore: lib/ (vendored), the MIDI and community examples, and the shells.

The script in the action field diffs a pinned SHA against upstream HEAD for these paths, word-diffs PDFs, and lists issues and PRs updated since the last run with hardware keywords flagged.

**Evidence:** The script was run on 2026-10-20 against T-Display-S3 pins 02f0e9d and 9642869 and LilyGo-Display-IDF c465092 and b1a1cc5; it exited 10 and printed the expected diffs. A scratch copy of the script was kept outside this repo.

**Action:** Add tools/upstream.lock + tools/upstream-watch.sh to the Gogadget repo; run monthly (cron or a GitHub Actions schedule such as cron "0 3 1 * *"), open an issue when it exits 10, review the diff, then bump the pinned SHA. Tested here against pins 02f0e9d and 9642869 (it printed the 2026 README/factory diffs, the schematic word diff TP4056B->TP4065/IBAT 580, and flagged #352). Requires git, gh (optional, for the issue scan) and pdftotext (optional, for schematic/datasheet word diffs). Since there are no tags/releases, always pin by full SHA. Quarterly, also check manually: the lilygo.cc product page version selector, fccid.io/2ASYE-T-DISPLAYS3, and the LILYGO wiki page.

`tools/upstream.lock`:

```text
# tools/upstream.lock  (name|git-url|pinned-sha|watched paths)
T-Display-S3|https://github.com/Xinyuan-LilyGO/T-Display-S3.git|1cc41eb1c76b2f3215130a3dd64e7138ce00b482|README.md schematic datasheet dimensions/README.MD image examples/factory/factory.ino examples/factory/pin_config.h examples/Sleep/Sleep.ino examples/GetBatteryVoltage/GetBatteryVoltage.ino examples/lv_demos/lv_demos.ino examples/lv_demos/pin_config.h examples/tft/tft.ino lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h firmware/README.MD boards platformio.ini .github/workflows
LilyGo-Display-IDF|https://github.com/Xinyuan-LilyGO/LilyGo-Display-IDF.git|b1a1cc54994bf1b417e3bb30c437bbe1036bff7f|main/display_s3.c main/initSequence.c main/initSequence.h main/product_pins.h main/Kconfig.projbuild sdkconfig.defaults.t-display-s3 README.md
```

`tools/upstream-watch.sh`:

```bash
#!/usr/bin/env bash
# upstream-watch.sh - report what changed upstream (LilyGO) in the paths Gogadget depends on.
# Usage: tools/upstream-watch.sh [lockfile]   exit 0 = no drift, exit 10 = drift (review, then bump the pin)
# Lock file lines: name|git-url|pinned-sha|space-separated paths   (lines starting with # are comments)
set -euo pipefail
LOCK=${1:-tools/upstream.lock}
CACHE=${UPSTREAM_CACHE:-$HOME/.cache/gogadget-upstream}
mkdir -p "$CACHE"
since=$(cat "$CACHE/last-run" 2>/dev/null || date -u -v-31d +%F 2>/dev/null || date -u -d '31 days ago' +%F)
status=0
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
words() { pdftotext -raw "$1" - 2>/dev/null | awk '{for(i=1;i<=NF;i++) print $i}' | sort -u; }

while IFS='|' read -r name url pin paths; do
  [[ -z "${name// }" || "${name:0:1}" == "#" ]] && continue
  repo="$CACHE/$name.git"
  [[ -d "$repo" ]] || git clone --quiet --bare --filter=blob:none "$url" "$repo"
  git -C "$repo" fetch --quiet --prune origin '+refs/heads/*:refs/heads/*'
  head=$(git -C "$repo" rev-parse HEAD)
  read -r -a P <<<"$paths"
  echo "== $name  pinned ${pin:0:7}  upstream HEAD ${head:0:7} ($(git -C "$repo" log -1 --format=%cs HEAD))"
  if [[ -z "$(git -C "$repo" log --format=%h "$pin..$head" -- "${P[@]}")" ]]; then
    echo "   no changes in watched paths"
  else
    status=10
    git -C "$repo" log --date=short --format='   %h %ad %an: %s' "$pin..$head" -- "${P[@]}"
    git -C "$repo" diff --stat=110 "$pin" "$head" -- "${P[@]}" | sed 's/^/   /'
    git -C "$repo" diff --name-status --no-renames "$pin" "$head" -- "${P[@]}" | while read -r st file; do
      case "$file" in
        *.pdf|*.PDF)   # schematic/datasheet: compare extracted words, not bytes
          git -C "$repo" show "$pin:$file" > "$tmp/old.pdf" 2>/dev/null || : > "$tmp/old.pdf"
          git -C "$repo" show "$head:$file" > "$tmp/new.pdf" 2>/dev/null || : > "$tmp/new.pdf"
          echo "   --- $file: words removed(-) / added(+)"
          { diff <(words "$tmp/old.pdf") <(words "$tmp/new.pdf") | grep '^[<>]' | grep -v -E 'PI[A-Z0-9]+$|^. (CO|NL)' |
            sed 's/^</-/; s/^>/+/' | paste -sd' ' - | fold -w 110 | sed 's/^/      /'; } || true ;;
        *.h|*.c|*.cpp|*.ino|*.md|*.MD|*.ini|*.json|*.yml|*.yaml|*.txt|*.projbuild|sdkconfig*)
          echo "   --- diff $file"
          git -C "$repo" diff "$pin" "$head" -- "$file" | sed -n '5,400p' | sed 's/^/      /' ;;
        *) echo "   --- $st (binary): $file" ;;
      esac
    done
  fi
  if command -v gh >/dev/null; then   # issues/PRs touched since the last run; hardware keywords flagged with !
    slug=${url#https://github.com/}; slug=${slug%.git}
    echo "   --- issues/PRs updated since $since"
    jq_fmt='.items[] | "      #" + (.number|tostring) + " [" + .state + "] " + .updated_at[:10] + " " + .title'
    { gh api -X GET search/issues -f q="repo:$slug updated:>=$since" -f per_page=100 --jq "$jq_fmt" 2>/dev/null |
      awk 'tolower($0) ~ /sleep|backlight|gpio ?15|battery|charg|schematic|v1[.][0-9]|revision|st7789|screen|power|current|adc|lcd_module|antenna/ {print "   !" $0; next} {print}'; } || true
  fi
done < "$LOCK"

date -u +%F > "$CACHE/last-run"
exit $status
```

## Reusable code

### 1. AW9364 pulse-count backlight control (setBrightness). 16 levels; 0 means EN low for at least 3 ms; enable means EN high, wait 30 µs, start at level 16; each low→high pulse steps one level down with wrap-around; pulse count = (16 + current − target) mod 16.

**Source:** examples/factory/factory.ino:223-247 @1cc41eb1c76b2f3215130a3dd64e7138ce00b482 (introduced in 2d3c294, 2024-09-23, issue #278). Ramp usage: factory.ino:371-380.

**Licence:** MIT, from the repo LICENSE ('Copyright (c) 2022 Xinyuan-LilyGO'); the file has no per-file header.

**Port notes:** Replace digitalWrite and delay with gpio_set_level, esp_rom_delay_us and vTaskDelay. Emit the pulse train inside portENTER_CRITICAL (or with dedicated GPIO bundles or RMT) so each low and high phase is a fixed 1–2 µs and preemption cannot stretch a low phase into the chip's shutdown window. Keep the current level in RTC_DATA_ATTR, and reset it to 16 whenever EN goes from off to on, including every deep-sleep wake. Map Gogadget's UI brightness steps onto 1..16. Confirm minimum and maximum pulse widths and shutdown time against the AW9364 datasheet, which is not in the repo.

### 2. ST7789 'LCD_MODULE_CMD_1' init table (porch, gate, VCOM, power and gamma; COLMOD 0x05) and the init order used with esp_lcd: reset, init, invert, swap_xy, mirror, set_gap(0,35), disp_on_off, then the table.

**Source:** examples/factory/factory.ino:126-150 (table) and 304-331 (order) @1cc41eb1c76b2f3215130a3dd64e7138ce00b482. History: 772b8d8 (2022-12-20), 65db476 (2023-01-14).

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO).

**Port notes:** Store the table as a static const array of {cmd, data[14], len, delay_ms} and send each entry with esp_lcd_panel_io_tx_param(io, cmd, data, len); wait 120 ms after 0x11. As far as I know the built-in IDF ST7789 driver has no hook for a custom init table, so send it after esp_lcd_panel_init, then call esp_lcd_panel_disp_on_off(panel, true) last. The table has no MADCTL, so orientation stays with swap_xy/mirror. Put it behind a Kconfig option (default y), because the maintainer said frosted panels do not need it (#87). Do not copy the 'j < len & 0x7f' loop from the other examples.

### 3. Deep-sleep entry and wake ordering for this board: SLPIN, then GPIO15 and backlight LOW, then ext0 wake on GPIO14. The Sleep example also turns the backlight on only after the first frame is drawn and uses a 15 s timer wake.

**Source:** examples/factory/factory.ino:395-404 (c9fd58a, #289) and examples/Sleep/Sleep.ino:69-89 and 93-117 (4b2edc1, fc466c6, #300) @1cc41eb1c76b2f3215130a3dd64e7138ce00b482

**Licence:** MIT (repo LICENSE).

**Port notes:** In IDF, call esp_lcd_panel_disp_sleep(panel, true) or esp_lcd_panel_io_tx_param(io, 0x10, NULL, 0) and wait at least 5 ms. Then drive GPIO38 and GPIO15 LOW. Park GPIO5,6,7,8,9,39–42,45–48 LOW and latch them with gpio_hold_en plus gpio_deep_sleep_hold_en; this is the #300 ghosting fix, and GPIO45/46 must be held LOW, never HIGH. Wake with ext1 on GPIO0 and GPIO14 (ESP_EXT1_WAKEUP_ANY_LOW, via esp_sleep_enable_ext1_wakeup_io or esp_sleep_enable_ext1_wakeup), with no internal pulls since the board has 10k/100k pull-ups. On wake: gpio_deep_sleep_hold_dis and gpio_hold_dis, re-init the panel, render with lv_refr_now(), then ramp the backlight. Leave GPIO16–18 floating.

### 4. Battery voltage reading (ADC1 on GPIO4, eFuse-calibrated, ×2 divider) with the '>4300 mV means no battery or USB' heuristic.

**Source:** examples/GetBatteryVoltage/GetBatteryVoltage.ino:75-95 @1cc41eb1c76b2f3215130a3dd64e7138ce00b482 (4b8e1ee, 2023-10-12)

**Licence:** MIT (repo LICENSE).

**Port notes:** esp_adc_cal is legacy. Use adc_oneshot_new_unit(ADC_UNIT_1), channel ADC_CHANNEL_3 (GPIO4), ADC_ATTEN_DB_12, and adc_cali_create_scheme_curve_fitting, then multiply mV by 2. After setting GPIO15 HIGH, wait 3–5 ms (C13 10 nF behind a 50 kΩ source). Average 8–16 samples and avoid Wi-Fi TX bursts. Use readings with GPIO15 LOW as a USB-present detector (about 2.3 V at the pin on USB, about 0 V on battery).

### 5. SNTP start-up and the battery-saving pattern of dropping Wi-Fi as soon as the first time sync arrives.

**Source:** examples/factory/factory.ino:453-470 (init_sntp_time) and 717-722 (timeavailable calls WiFi.disconnect()) @1cc41eb1c76b2f3215130a3dd64e7138ce00b482 (c9b939f, 2026-07-28)

**Licence:** MIT (repo LICENSE).

**Port notes:** Use esp_netif_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2, ESP_SNTP_SERVER_LIST("vn.pool.ntp.org", "pool.ntp.org")); set cfg.sync_cb and cfg.server_from_dhcp explicitly (#297 shows DHCP networks failing); call esp_netif_sntp_init(&cfg), then esp_netif_sntp_sync_wait() with a hard timeout. On success: esp_wifi_disconnect() and esp_wifi_stop(), setenv("TZ", "ICT-7", 1), tzset(). Do not port the ipapi.co lookup or the pinned certificate chain; its leaf expired 2025-03-29.

### 6. i80 bus and panel IO configuration for this board: 8-bit bus, DC levels, CS on GPIO6, RD held HIGH by GPIO, full-frame transfers, on_color_trans_done signalling flush-ready.

**Source:** examples/factory/factory.ino:257-299 @1cc41eb1c76b2f3215130a3dd64e7138ce00b482. Cross-check: LilyGo-Display-IDF main/display_s3.c@b1a1cc5 (10 MHz, disp_on_off at line 145).

**Licence:** MIT for both repos (LilyGo-Display-IDF LICENSE; T-Display-S3 LICENSE).

**Port notes:** IDF 6.1: esp_lcd_i80_bus_config_t {.dc_gpio_num=7, .wr_gpio_num=8, .clk_src=LCD_CLK_SRC_DEFAULT, .data_gpio_nums={39,40,41,42,45,46,47,48}, .bus_width=8, .max_transfer_bytes=draw_buf_bytes}. Use dma_burst_size where the IDF 6.1 headers drop psram/sram_trans_align. Panel IO: {.cs_gpio_num=6, .pclk_hz=10*1000*1000, .trans_queue_depth=10, .lcd_cmd_bits=8, .lcd_param_bits=8, .dc_levels={0,0,0,1}}. Drive GPIO9 (RD) HIGH as a plain output. Let esp_lvgl_port own on_color_trans_done, and set flags.swap_bytes=1 for LVGL 9. Verify field names against the IDF 6.1 headers.

## Open questions

- What changed in V1.3 besides the antenna matching? The maintainer only described the antenna; no V1.3 schematic is published, and the 2026-09-24 FCC exhibits (internal photos, user manual) for 2ASYE-T-DISPLAYS3 could not be fetched automatically (HTTP 403). Possible differences: R12, LDO, power path, charger, panel.
- Which panel variant (glossy or matte) do the Gogadget units have? Does CMD_1 help or hurt on matte panels, and does the #300 ghosting after deep sleep still occur under IDF 6.1 with SLPIN plus pin parking and holds?
- What is the real charge current with R13 = 2 kΩ on TP4065? The schematic note says 580 mA (TP4056 table), the README says 500 mA, and the known measurement is ~525 mA. The TP4065 datasheet formula was not verified.
- AW9364 timing limits are not documented upstream: maximum low-pulse width before shutdown, shutdown detection time, power-on default level, and whether EN has an internal pull-down. The datasheet is not in the repo.
- Breakdown of the 263 µA deep-sleep current: R12 (100 kΩ) and the LDO quiescent current explain about 60–100 µA. Is the rest D3/Q4 leakage, TP4065 reverse current, panel SLPIN current, or pin leakage? What is R12 for, and is it safe to remove?
- Battery connector family and polarity: 'JST 1.25 mm' (README) vs 'JST-GH 1.25mm 2PIN' (product page and wiki). This needs a physical check.
- Mounting-hole coordinates and outline tolerances exist only in dimensions/PCB.dwg, which was not parsed here.
- Root cause of #297 (NTP failing on a DHCP-assigned address, working with a static IP): a DHCP-provided NTP server (option 42) is suspected, not confirmed.
- Does IDF 6.1's USB-Serial-JTAG console ever stall output or boot when running on battery with no host (the Arduino FAQ 5 analogue)?
- Can a ULP poll of ADC1_CH3 (GPIO4) during deep sleep detect USB insertion cheaply enough (detection latency vs. µA cost) to trigger Gogadget's always-on mode without a button press?
- Will LilyGO publish ESP-IDF 5/6 examples for T-Display-S3? LilyGo-Display-IDF's T-Display-S3 code is unchanged since 2024-01-31, and the main repo's lv_demos still refuses IDF ≥5.
