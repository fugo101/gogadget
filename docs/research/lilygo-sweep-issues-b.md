# LilyGO sweep: issues and PRs #177 to #352

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 1 of 45 items was verified in lilygo-verification.md (0 confirmed / 1 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): RGB565 bytes are swapped in i80 hardware instead of `swap_bytes = true` (K14 and the reusable code); GPIO38 needs no deep-sleep hold (K02 and the reusable code); the owner keeps the stock charge current (K18); Wi-Fi is skipped below about 3.6 V VSYS (K31 says 3.5 V); no ULP polling of GPIO4 for now (K04).

Superseded by decision: DHCP-supplied NTP servers are never used, only 1.ntp.vnix.vn and pool.ntp.org (K30; ADR-0011); the USB test is GPIO15 LOW then GPIO4, with VSYS above about 4.35 V only a hint (K04); the sleep entry (K08 and reusable item 2) sends SLPIN without DISPOFF and parks RST HIGH.

Scope: every issue, PR and comment from #177 to #352. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**Scope.** I read all 174 items in the range, including every comment. That is 155 commented threads; #258 and #339 do not exist. 17 are PRs: 11 merged (mostly community MIDI examples) and 6 closed unmerged. Most threads are Arduino/TFT_eSPI setup help. The hardware facts come from maintainer replies (lewisxhe, Llgok) and from the published v1.2 schematic (`schematic/T_Display_S3.pdf`, dated 2024-04-29). I traced that schematic and checked it against the ST7789V and TP4065 datasheets.

### Power path

- The schematic matches #352, #270 and #190:
  - USB: VBUS → 1N5819 diode → VSYS.
  - Battery: cell → R10 (a 2 A fuse) → Q4 (SI2307) → VSYS. Q4 switches off while USB is present.
  - VSYS → LDO → VDD3V3, which is always on.
- GPIO15 **or** VBUS (through two diodes) switches both:
  - V3V: backlight, AW9364 and the green LED;
  - the BAT_ADC divider. That divider measures **VSYS, not the cell**.
- So on USB, GPIO15 has no effect, and nothing can wake the chip when USB is plugged in during deep sleep (#292).

### Sleep current

These are permanent loads, not listed in the known facts:
- R12, 100 kΩ straight across the cell: about 40 µA.
- The LDO's own idle draw (quiescent current): 2–55 µA. The part varies by batch: AP2112K, RT9013, or RT9080 (#291, #348).
- The panel in sleep mode: 20–40 µA.

Together these come to roughly 70–145 µA. The 263 µA measured in #289 leaves about 120–190 µA unexplained. My guess is the LCD bus pins floating against a panel that stays powered.

GPIO16, 17 and 18 have pull-ups fitted even on non-touch boards (10 k / 10 k / 100 k, #313), so never drive them low while asleep.

### Display

- The panel's frame memory survives a hardware reset and deep sleep, because the panel stays powered. On wake it still holds the old frame, which is the stale time. Keep the backlight off until the first full frame has been sent: #232, and the maintainer in #300: *"refresh the screen to all black, and then turn on the backlight last"*.
- Units with matte screens showed ghosting and white edges after deep sleep (#300). The fix was putting the panel to sleep and parking every LCD pin low. LilyGO's own Sleep example sends sleep-in (SLPIN) but does not latch any pins in a fixed state.
- Without the vendor init table, *"the pixels will be blurred"* (#229).
- The ST7789V needs at least 66 ns per write, so about 15 MHz is its limit. Your 10 MHz is within spec. The factory sketch runs 16 MHz, with LilyGO's comment *"Too low or too high pixel clock may cause screen mosaic"* (commit adc038e).
- The factory example has had a 16-level AW9364 driver since 2024-09 (#278, commit 2d3c294): HIGH for 30 µs to enable, LOW for 3 ms to switch off. The community MIDI examples merged in 2026 still drive the backlight with PWM. Don't copy from them.

### Battery and charging

- The R10 fuse wears out in the field (#223: about 350 Ω; #291: 5–45 Ω after some charge cycles). The maintainer says bridging it is fine with a protected cell.
- R13 (2 kΩ) sets about 500–525 mA charge current, roughly 2C for a 250 mAh cell. Either fit about 4.3 kΩ (1C) or 8.2 kΩ (0.5C), or choose a cell rated for 2C charging. The datasheet formula is R = 1050/I.
- The connector is a JST 1.25 mm 2-pin, not JST-GH (#287). The official case fits a battery up to 47×25×7 mm (#324).
- The maintainer says a reversed battery does no damage (#317), and the TP4065 datasheet lists reverse-battery protection.
- The header pin labelled "5V" is really VSYS (#270). Feeding 5 V into it with a battery attached would charge the cell directly through Q4, bypassing the charger. LilyGO's own docs contradict each other on this pin (#305, README).

### USB

- USB can only be detected through the ADC. LilyGO's example uses a 4300 mV threshold.
- The charging state cannot be read: the charger's status pin only drives the red LED.
- The schematic leaves both USB-C CC pins unconnected. USB-C-to-C chargers may therefore not power the board. This needs testing on a real unit.

### Wi-Fi

- The v1.2 onboard antenna is weak; one user reported this across about 100 boards (#340).
- Since 2026 there is a v1.3 board with a re-tuned antenna. Ask for it when ordering from lilygo.cc.
- An external antenna needs a tiny 0201 resistor rotated; the U.FL socket is not connected by default.
- The maintainer says Wi-Fi averages *"about 150mA+"* (#295). The LDO is rated about 500 mA against ESP32 peaks of about 700 mA (#291). Reboots on Wi-Fi connect were blamed on the board's power supply (#275).

### Still open

The R10 failure cause, which LDO and panel finish your units have, whether the CC resistors are fitted, the best pin parking for deep sleep, and whether waking on GPIO0 works (every official example wakes on GPIO14 only).

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| K01 | power path topology | high | refines | partially |
| K02 | GPIO15 / V3V switching | high | refines |  |
| K03 | BAT_ADC measures VSYS, settling time | high | refines |  |
| K04 | USB / charge detection | high | refines |  |
| K05 | permanent battery drain R12 | high | new |  |
| K06 | LDO part variants and capacity | high | new |  |
| K07 | deep-sleep current budget | high | refines |  |
| K08 | deep-sleep ghosting on matte panels | high | new |  |
| K09 | LilyGO sleep sequence is minimal | medium | refines |  |
| K10 | GRAM retention / boot artifacts | high | new |  |
| K11 | pixel clock limits | medium | refines |  |
| K12 | vendor init table | medium | refines |  |
| K13 | two LCD panel variants | medium | new |  |
| K14 | esp_lcd / LVGL configuration details | medium | refines |  |
| K15 | AW9364 reference timing | high | refines |  |
| K16 | repo inconsistency: PWM backlight in MIDI examples | low | contradicts |  |
| K17 | R10 battery fuse failures | high | new |  |
| K18 | charge current / R13 | high | refines |  |
| K19 | reverse battery polarity | medium | new |  |
| K20 | battery connector | medium | new |  |
| K21 | official case battery cavity | medium | new |  |
| K22 | '5V' header pin = VSYS; back-feed hazard | medium | contradicts |  |
| K23 | power-input hazards | low | new |  |
| K24 | LEDs | medium | new |  |
| K25 | touch pull-ups populated on non-touch boards | medium | new |  |
| K26 | 4-pin JST-SH connector is UART0 | low | refines |  |
| K27 | button hardware and wake usage | medium | new |  |
| K28 | crystal accuracy / timekeeping | medium | new |  |
| K29 | Wi-Fi antenna: v1.2 weak, v1.3 exists | high | new |  |
| K30 | Wi-Fi association failures | medium | new |  |
| K31 | Wi-Fi current, brownout risk | high | new |  |
| K32 | Bluetooth | low | confirms |  |
| K33 | ESP-IDF console | low | confirms |  |
| K34 | USB-C CC pins unconnected (to verify) | medium | new |  |
| K35 | USB serial connect resets | low | new |  |
| K36 | flashing recovery | low | new |  |
| K37 | on-chip JTAG debugging | low | new |  |
| K38 | flash/PSRAM configuration | low | confirms |  |
| K39 | mechanical: display removal | low | new |  |
| K40 | mechanical CAD files | low | new |  |
| K41 | no official LVGL 9 reference | low | new |  |
| K42 | ADC accuracy | medium | refines |  |
| K43 | GPIO15 must be set first on battery | medium | refines |  |
| K44 | touch vs non-touch hardware | low | confirms |  |
| K45 | 3V3 rail failure diagnostics | low | new |  |

### K01: power path topology

Relevance: high. Novelty: refines. Verification: partially (see lilygo-verification.md, `issues-b:K01`).

**Fact:** v1.2 power path: VBUS -> D3 1N5819 Schottky -> '+5V' (really VSYS); battery JST (P3) -> R10 '5V/2A' fuse -> VBAT -> Q4 SI2307 P-FET -> VSYS. Q4 gate is tied to the VBUS node (R14 100k to GND), so Q4 is ON on battery and OFF when USB is present (battery then isolated from the load and only charged by TP4065). VSYS -> U7 3.3 V LDO -> VDD3V3 (ESP32-S3, flash, panel logic VDD on CN1 pins 2-3, header 3V pins, touch pull-ups).

**Evidence:** schematic/T_Display_S3.pdf@1cc41eb (sheet dated 4/29/2024, re-uploaded in b0a5ecd for #236), 'Power' and 'Charger' blocks; #270 lewisxhe: '5V is just a label... this is the end point of the path between the battery and USB... The correct label should be VSYS, not 5V'.

**Action:** Model Gogadget power states on this topology: on USB the cell is not powering the board; on battery every load passes through R10 + Q4 + LDO dropout.

### K02: GPIO15 / V3V switching

Relevance: high. Novelty: refines.

**Fact:** Q1 (PMBT3904) base is driven through a 1N4148 diode-OR of VBUS (D1) and PWR_EN/GPIO15 (D2), 10k series, 100k pull-down. Q1 turns on BOTH Q3 (SI2301: VDD3V3 -> V3V = AW9364 VIN, LCD backlight anode CN1 pin 20, green LED8) and Q2 (SI2301: VSYS -> 'BAT' net feeding the BAT_ADC divider). With USB present, V3V and the ADC divider are ON regardless of GPIO15; a floating GPIO15 = OFF.

**Evidence:** schematic Power block (Q1/Q2/Q3, D1/D2, R5 10K, R7 100K, note 'Battery power must set IO15 to HIGH'); #352 reporter: 'V3V ... driven by PWR_EN (GPIO15) diode-OR'd with VBUS'; README.md LED section @1cc41eb: 'In USB mode, the green indicator stays on. In battery power mode, GPIO15 lights up when high'.

**Action:** Test any GPIO15-dependent behaviour on battery only. Because USB inserted during deep sleep powers the AW9364, hold GPIO38 LOW in sleep (gpio_hold_en + gpio_deep_sleep_hold_en, GPIO38 is not an RTC pad).

### K03: BAT_ADC measures VSYS, settling time

Relevance: high. Novelty: refines.

**Fact:** The top of the R2/R4 100k 1% divider is the 'BAT' net = VSYS switched by Q2, not the cell. On USB VSYS = VBUS - Vf(1N5819) ~ 4.6 V; on battery VSYS = VBAT - I*(R10 + Q4 Rds). C13 = 10 nF sits across R4, so after Q2 turns on the node settles with tau ~ 50 kohm * 10 nF = 0.5 ms; with GPIO15 LOW on battery the node discharges to 0 V (an ADC read then returns ~0, see #284).

**Evidence:** schematic 'ADC' block (R2, R4 '100K%1', C13 10nF) + Power block (Q2 between +5V and BAT); #190 lewisxhe: 'When USB is plugged in, the BAT voltage is taken over by the TP4065 charging IC, so the voltage tested at this time is meaningless'; #284 (reading went to 0 on battery until GPIO15 HIGH).

**Action:** On wake: set GPIO15 HIGH first, wait >=3 ms before sampling GPIO4; label it 'VSYS under load', expect it a few tens of mV below cell OCV; show battery % only when not on USB.

### K04: USB / charge detection

Relevance: high. Novelty: refines.

**Fact:** No VBUS or TP4065 CHRG signal reaches a GPIO (CHRG only drives the red LED). Maintainer: detecting USB without the ADC 'is not possible with current hardware' (#292); 'If it is greater than 4.2V, it can be determined that the USB is connected, but it cannot be determined whether it is charging' (#254). LilyGO's example uses >4300 mV. USB insertion during deep sleep cannot wake the chip; on USB BAT_ADC ~2.3 V, below the ESP32-S3 VIH (0.75*VDD ~2.48 V), so a GPIO wake on GPIO4 is unreliable.

**Evidence:** #292 (lewisxhe 2024-12-04); #254 (lewisxhe 2024-06-03); examples/GetBatteryVoltage/GetBatteryVoltage.ino:86-88@1cc41eb; schematic Charger block (CHRG -> LED1 via R9 2K from VBUS).

**Action:** USB-present = calibrated, averaged VSYS > ~4.35 V with hysteresis. 'Always-on while on USB' can only start at the next button/timer wake unless you add ULP ADC polling of GPIO4 in deep sleep. The UI can only say 'on USB', never 'charging' or 'full'.

### K05: permanent battery drain R12

Relevance: high. Novelty: new.

**Fact:** R12 = 100 kohm is drawn directly from VBAT (after the fuse, before Q4) to GND: a constant ~37-42 uA drain from the cell that GPIO15 cannot switch off (~1 mAh/day). Its purpose is not documented, possibly a load for TP4065 no-battery detection.

**Evidence:** schematic Power block, VBAT node with C26 10uF/16V, C27 100nF and R12 100K to GND, left of Q4 SI2307.

**Action:** Include ~40 uA in the deep-sleep budget; verify on the board that R12 is fitted. Removing it is a possible hardware mod, but test charger LED/no-battery behaviour afterwards.

### K06: LDO part variants and capacity

Relevance: high. Novelty: new.

**Fact:** U7 is labelled 'AP2112K-3.3V(RT9013)' in the schematic. The maintainer called it RT9013 that 'can only provide 500mA current at most, which may not be enough for ESP32. The maximum peak current of normal ESP32 is generally around 700mA' (#291), later identified 'RT9080 LDO' on current boards (#348, 2026-09), and advises 'any LDO with an output current greater than 700mA can be used for replacement' (#343). Quiescent currents differ widely by part (typical datasheet values, not from the repo: AP2112K ~55 uA, RT9013 ~25 uA, RT9080 ~2 uA). README (2026-08): '3V pin header load does not exceed 100mA'.

**Evidence:** schematic U7; #291 (lewisxhe 2024-12-07); #343 (2026-06-05); #348 (lewisxhe 2026-09-14, in Chinese: '箭头指向的位置都是RT9080 LDO'); README.md @9e7bdc3.

**Action:** Read the LDO marking on each unit (it sets 2-55 uA of the sleep floor). Keep the brownout detector on, cap Wi-Fi TX power on battery, and avoid running full backlight during Wi-Fi association.

### K07: deep-sleep current budget

Relevance: high. Novelty: refines.

**Fact:** ST7789V normal-mode IDD is 6.0 mA typ, which explains #289's 6.42 mA without SLPIN; sleep-in is IDD 15/30 uA plus IDDI 5/10 uA (typ/max). Estimated battery-side floor with SLPIN and GPIO15 LOW: ESP32-S3 ~7-8 uA + R12 ~40 uA + LDO 2-55 uA + panel 20-40 uA + TP4065 <1 uA ~ 70-145 uA. #289 measured 263 uA, so ~120-190 uA is unexplained (likely floating LCD bus inputs on the always-powered panel). #289 also measured 147.7 mA awake.

**Evidence:** datasheet/ST7789V_SPEC_V1.4.pdf section 7.3 Table 3 @1cc41eb; TP4065 datasheet ('battery drain current to less than 1uA', URL cited in #230); #289 ('draws 6.42mA asleep and 147.7mA awake', later '263 uA').

**Action:** Measure with a 3.7-4.2 V bench supply on the battery JST, USB unplugged; iterate pin holds toward ~100-150 uA. Budget: a 10 s glance at ~150 mA ~0.42 mAh; 20 glances/day ~8 mAh + ~6 mAh/day sleep ~ 2-3 weeks on 250 mAh.

### K08: deep-sleep ghosting on matte panels

Relevance: high. Novelty: new.

**Fact:** #300: after multi-minute deep sleep on battery, matte-screen units showed white borders and smeared old text (glossy units did not) when the driver never sent SLPIN. The user fixed it by driving all LCD pins (38, 39-42, 45-48, 15, 5, 6, 7, 8, 9) LOW before deep sleep. The maintainer advised 'After the screen is initialized, refresh the screen to all black, and then turn on the backlight last' and added SLPIN (0x10) to examples/Sleep (fc466c6). Root cause never confirmed; floating CS/WR during sleep could clock garbage into a panel that stays powered (#352).

**Evidence:** #300 (sgbmzm 2025-03-14/18/24; lewisxhe 2025-03-10/19); commit fc466c6; examples/Sleep/Sleep.ino:105-116@1cc41eb; ST7789V datasheet SLPIN: 'MCU interface and memory are still working'.

**Action:** Deep-sleep entry: DISPOFF 0x28 -> SLPIN 0x10 -> wait >=5 ms -> CS=1, WR=1, RD=1, DC=0, D0-D7=0, RST held (measure LOW vs HIGH), GPIO38=0, GPIO15=0 -> gpio_hold_en on each + gpio_deep_sleep_hold_en(). Release the holds only after reconfiguring on wake.

### K09: LilyGO sleep sequence is minimal

Relevance: medium. Novelty: refines.

**Fact:** Both official deep-sleep paths do only: SLPIN 0x10, GPIO15 LOW, GPIO38 LOW, ext0 wake on GPIO14 LOW. No DISPOFF, no explicit 5 ms wait (datasheet requires 5 ms after SLPIN), and no gpio_hold of LCD bus pins; GPIO39-48 are digital pads that float in deep sleep unless held.

**Evidence:** examples/Sleep/Sleep.ino:77-89,105-116@1cc41eb; examples/factory/factory.ino:395-404@1cc41eb (added c9fd58a for #289); ST7789V datasheet SLPIN restriction: 'wait 5msec before sending any new commands'.

**Action:** Treat LilyGO's sequence as a starting point only; add the steps in K08.

### K10: GRAM retention / boot artifacts

Relevance: high. Novelty: new.

**Fact:** ST7789V frame memory is random after power-on, but 'Contents of memory is not cleared' by H/W or S/W reset and is kept in sleep-in. Because panel VDD stays on through deep sleep, the previous frame (stale time) is still in GRAM after wake + reset, while a real cold power-up shows noise. #232: white noise at boot appeared only after >=1 min power-off. Maintainer fix: stop the library enabling the backlight and turn 'on the backlight after tft.begin()'.

**Evidence:** datasheet/ST7789V_SPEC_V1.4.pdf (reset tables: 'H/W Reset: Contents of memory is not cleared'; SLPIN: 'memory keeps its contents'); #232 (lewisxhe 2024-03-13); #300 maintainer quote.

**Action:** Keep AW9364 EN LOW until LVGL's first full-screen flush completes (wait for on_color_trans_done), then DISPON and ramp the backlight. Never show the stale frame.

### K11: pixel clock limits

Relevance: medium. Novelty: refines.

**Fact:** ST7789V 8080 write cycle TWC min 66 ns (~15 MHz max). The factory sketch went 6.528 MHz -> 20 MHz -> 16 MHz with the comment 'Too low or too high pixel clock may cause screen mosaic'; lv_demos still uses 6.528 MHz (visible in #273); LilyGo-Display-IDF uses 10 MHz.

**Evidence:** datasheet/ST7789V_SPEC_V1.4.pdf section 7.4.1 (TWC 66 ns, TWRH/TWRL 15 ns); examples/factory/pin_config.h:24-25@1cc41eb (set in adc038e, 2023-03-03); examples/lv_demos/pin_config.h:14; #273.

**Action:** Keep 10 MHz (in spec; 16 MHz is out of spec). If any mosaic appears on real hardware, sweep 8-12 MHz. No issue reports 10 MHz as 'too low'.

### K12: vendor init table

Relevance: medium. Novelty: refines.

**Fact:** Maintainer: the CMD_1 sequence 'is provided by the screen manufacturer. If this part of the initialization command is not added, the pixels will be blurred.' The factory sends it after esp_lcd_panel_init/invert/swap/mirror/gap/disp_on, starting with SLPOUT 0x11 + 120 ms.

**Evidence:** #229 (lewisxhe 2024-03-05); examples/factory/factory.ino:126-150 and 325-331@1cc41eb.

**Action:** Send the table on every init, cold boot and deep-sleep wake alike; keep the 120 ms after 0x11.

### K13: two LCD panel variants

Relevance: medium. Novelty: new.

**Fact:** Boards ship with glossy or matte LCDs. In #300 only matte units ghosted after deep sleep; the user associates glossy with LilyGO's own 'With Black Shell [K204]' and matte with an AliExpress 'Non-Soldered Pin [H569]' unit. Code comment: 'The product now has two screens, and the initialization code needs a small change in the new version' (LCD_MODULE_CMD_1).

**Evidence:** #300 (sgbmzm 2025-03-10/18); examples/Sleep/Sleep.ino:8-10@1cc41eb.

**Action:** Record each unit's panel finish; run the deep-sleep soak test (minutes-to-hours sleeps) on a matte unit before release.

### K14: esp_lcd / LVGL configuration details

Relevance: medium. Novelty: refines.

**Fact:** Official esp_lcd bring-up: RD (GPIO9) set as output HIGH by the app (the i80 driver doesn't drive RD); invert_color(true); swap_xy(true); mirror(false,true) = 'screen faces you, USB on the left' (mirror(true,false) = USB on the right); set_gap(0,35); dc levels cmd=0/data=1; full-frame LVGL buffer in MALLOC_CAP_DMA|INTERNAL; LVGL 8 with LV_COLOR_16_SWAP 1, because the 8-bit bus needs byte-swapped RGB565.

**Evidence:** examples/factory/factory.ino:257-258, 263-324, 333@1cc41eb; lib/lv_conf.h:30@1cc41eb; #247 and #243 (column/row offset 35, rgb565_byte_swap=True).

**Action:** In LVGL 9 + esp_lvgl_port 2.9, set flags.swap_bytes=true, landscape via swap_xy + mirror, gap (0,35), and drive GPIO9 HIGH yourself.

### K15: AW9364 reference timing

Relevance: high. Novelty: refines.

**Fact:** LilyGO replaced LEDC in the factory sketch on 2024-09-23 (maintainer #278: 'The method of ledc controlling the backlight of display-S3 is wrong'), before 02f0e9d. Reference driver: 16 levels; value 0 -> LOW then delay(3) ms; enabling from off -> HIGH then 30 us (chip starts at level 16); each LOW->HIGH pulse steps down one level, wrapping 1->16; pulses are back-to-back digitalWrite calls with no explicit width.

**Evidence:** examples/factory/factory.ino:223-247@1cc41eb; commit 2d3c294 ('Fixed setBrightness #278'); #278 (lewisxhe 2024-09-23).

**Action:** ESP-IDF gpio_set_level is far faster than Arduino digitalWrite: add esp_rom_delay_us(1) per edge and run the pulse train in a critical section so no LOW drifts toward the shutdown time. Check limits against the AW9364 datasheet (roughly 0.5 us min pulse, about 500 us max LOW, more than 2.5 ms LOW = off).

### K16: repo inconsistency: PWM backlight in MIDI examples

Relevance: low. Novelty: contradicts.

**Fact:** Community MIDI examples merged Feb-Mar 2026 (PRs #328-#334, #336) drive GPIO38 with LovyanGFX Light_PWM at 22 kHz, which LilyGO itself declared invalid; 02f0e9d did not touch them.

**Evidence:** examples/T-Display-S3-Piano/PianoDisplay.h:113@1cc41eb (same in the other T-Display-S3-* MIDI examples); git show --stat 02f0e9d (11 files, none MIDI).

**Action:** Never copy backlight code from the MIDI examples.

### K17: R10 battery fuse failures

Relevance: high. Novelty: new.

**Fact:** R10 ('5V/2A' fuse) sits between the battery connector and VBAT. Field reports: two v1.2 non-touch boards with R10 ~350 ohm (board saw 2.6-2.7 V from a 4.18 V cell, #223); four boards whose R10 degraded to ~5-45 ohm after several charge cycles, 3V3 sagging to 2.78 V and boot loops on battery (#291). Maintainer: 'it can be used safely without R10. Generally, lithium-ion batteries have their own protection circuits.' Root cause never identified.

**Evidence:** schematic Charger block (R10 '5V/2A保险丝' to P3); #223 (csthiang 2026-07-07); #291 (jperrela/lewisxhe 2024-12-05...11).

**Action:** Bring-up check: voltage at the JST vs VSYS under ~100 mA load. In firmware, flag a 'battery path fault' if on-battery VSYS is far below expected or brownouts happen at mid-charge. With a protected cell, bridging R10 is maintainer-sanctioned.

### K18: charge current / R13

Relevance: high. Novelty: refines.

**Fact:** PROG resistor R13 = 2 kohm. The schematic annotation 'R13 IBAT (mA) = 580' is a TP4056 table value; README: 'The default charging current is 500mA'; TP4065 datasheet: RPROG = 1050/IBAT (IBAT > 0.1 A), 2.1 k -> 425/500/575 mA min/typ/max, with thermal throttling above ~0.4 A. The maintainer posted R13's PCB location in #230.

**Evidence:** schematic Charger block; README.md FAQ 8 @1cc41eb; TP4065 datasheet REV 2.0 http://toppwr.com/uploadfile/file/20230304/6403023707f41.pdf ('Formula one: RPROG = 1050/IBAT'); #230 (lewisxhe 2024-03-05, photo).

**Action:** For the 250 mAh cell, confirm it accepts ~2C charge, or rework R13 to ~4.3 k (~245 mA, 1C) or ~8.2 k (~128 mA, 0.5C).

### K19: reverse battery polarity

Relevance: medium. Novelty: new.

**Fact:** Maintainer: 'Reversing the polarity will not cause any damage, and I remember there are positive and negative markings on the board.' The TP4065 datasheet lists 'Lithium-ion batteries Reverse battery protection'.

**Evidence:** #317 (lewisxhe 2025-09-15); TP4065 datasheet features list.

**Action:** Still check plug polarity against the board's +/- marks; 1.25 mm pre-wired cells come in both polarities.

### K20: battery connector

Relevance: medium. Novelty: new.

**Fact:** The battery socket takes a 1.25 mm-pitch 2-pin 'JST 1.25' (MX1.25/PicoBlade-style) plug, not JST-GH as some shops state; the maintainer answered with a photo of the matching plug.

**Evidence:** #287 (lewisxhe 2024-11-20, image); README.md Electrical table @9e7bdc3 ('Battery Connector Model | JST 1.25 mm').

**Action:** Specify a protected 250 mAh cell with a 1.25 mm 2-pin plug and correct polarity.

### K21: official case battery cavity

Relevance: medium. Novelty: new.

**Fact:** The largest battery that fits the official shell is 47 x 25 x 7 mm.

**Evidence:** #324 (lewisxhe 2026-01-26: 'The largest size that can be placed is 47*25*7mm.').

**Action:** Pick a cell inside 47x25x7 mm if you use the LilyGO shell or derive an enclosure from it.

### K22: '5V' header pin = VSYS; back-feed hazard

Relevance: medium. Novelty: contradicts.

**Fact:** Maintainer (#270): the header '5V' is mislabelled and is VSYS; 'If the battery is not connected, you can directly input the DC 5V power supply here. If the battery is connected, this 5V label can directly output the battery voltage.' The schematic agrees (P2-1 '+5V' sits after D3 and Q4). Contradicting docs: #305 lewisxhe 'VBUS = 5V'; README (2026-08) lists 'VBUS (pin header) 5V' and 'VBAT (pin header) 4.2V' (no VBAT header exists in the schematic). With no USB, Q4 is on, so 5 V injected at this pin with a battery attached would charge the cell directly, bypassing TP4065 (raised by a user in #190; maintainer: 'Oh! I overlooked something').

**Evidence:** #270 (2024-08-02); #305 (2025-11-27); #205; #190 (nvx/lewisxhe 2023-10-13...16); schematic Power block; README.md @9e7bdc3.

**Action:** Never power Gogadget through the '5V' header while a battery is fitted; use USB-C only.

### K23: power-input hazards

Relevance: low. Novelty: new.

**Fact:** 5 V fed into the battery connector while USB was connected destroyed a board (maintainer: 'most likely burned out the ... charging chip'; 'Measure the battery holder voltage. It should normally not exceed 4.2V'). External 3.3 V plus USB at the same time 'may be problems' (#228). The board VBUS has no maximum-current requirement: 'A regular 5V 1A adapter is sufficient' (#259).

**Evidence:** #343 (lewisxhe 2026-06-04/08); #228 (2024-03-05); #259 (2024-06-26).

**Action:** For bench current measurements feed 3.7-4.2 V into the battery JST with USB unplugged; never exceed 4.2 V there.

### K24: LEDs

Relevance: medium. Novelty: new.

**Fact:** Red LED = TP4065 CHRG (from VBUS via 2 k): on while charging, off when full, flashing/dim with no battery; maintainer: 'The red LED is meaningless; it's just a charging indicator' and 'A constantly lit red light is incorrect; it should be flashing when there's no battery.' Green LED8 hangs on V3V via 2 k, about 0.2-0.6 mA depending on LED Vf, and is lit whenever V3V is on (USB present or GPIO15 HIGH).

**Evidence:** #322 (lewisxhe 2026-01-13); #343 (2026-06-08); README.md LED section @1cc41eb; schematic (LED8 'Green' 2K on V3V; LED1 'Red' on CHRG).

**Action:** Expect a green glow during every glance on battery and permanently on USB. Remove LED8 or its 2 k resistor if a bedside clock must stay dark or to save ~0.5 mA awake.

### K25: touch pull-ups populated on non-touch boards

Relevance: medium. Novelty: new.

**Fact:** Non-touch boards still carry the touch-controller pull-ups to the always-on VDD3V3: RIIC 10 k (GPIO17/SCL), RIID 10 k (GPIO18/SDA), RINT 100 k (GPIO16/INT). Maintainer: 'Pull-up resistors are already added... If you must use these two GPIOs, you can remove these two resistors... on the back of the screen'.

**Evidence:** #313 (lewisxhe 2025-07-29, 2025-11-05 with photos); schematic Display block (RIIC/RIID 10K, RINT 100K).

**Action:** Never drive or hold GPIO16/17/18 LOW in deep sleep (~330 uA per 10 k pull-up); leave them input/high-Z. External I2C on 17/18 needs no extra pull-ups.

### K26: 4-pin JST-SH connector is UART0

Relevance: low. Novelty: refines.

**Fact:** The 4-pin 1.0 mm 'Qwiic/STEMMA' connector (CNC) carries U0TXD GPIO43, U0RXD GPIO44, VDD3V3 and GND; the community uses it as I2C with SDA=43, SCL=44 (Wire.begin(43,44)), not 18/17.

**Evidence:** schematic 'CNC WAFER-100W-4A' (4 U0RXD, 3 U0TXD); #181, #213, #225, #227 (users/teastain).

**Action:** If a Qwiic sensor is added later, use GPIO43/44, keep the console on USB-Serial-JTAG only, and expect ROM boot-log bytes on GPIO43 at each reset/wake (esp_deep_sleep_disable_rom_logging() suppresses them on deep-sleep wake).

### K27: button hardware and wake usage

Relevance: medium. Novelty: new.

**Fact:** GPIO0 (BOOT): 10 k pull-up to VDD3V3, no capacitor. GPIO14 (KEY): 100 k pull-up + 100 nF to GND (~10 ms RC: hardware-debounced, slow release edge). RST/CHIP_PU: 10 k/1 uF. RST is the side button next to BOOT (#184, #282). Every official and issue deep-sleep example wakes only via ext0 on GPIO14 LOW; none uses GPIO0.

**Evidence:** schematic 'Button' block (R1 10K, R3 100K, C14 100nF, RW 10K, CU 1uF); examples/Sleep/Sleep.ino:84 and 112@1cc41eb; #289, #300, #346.

**Action:** Two-button driver: software debounce >=20 ms and expect a ~10 ms release ramp on GPIO14. Use EXT1 ANY_LOW on GPIO0|GPIO14 (external pull-ups exist). Explicitly test GPIO0 wake and long-press across resets, since GPIO0 is a strapping pin with no official example.

### K28: crystal accuracy / timekeeping

Relevance: medium. Novelty: new.

**Fact:** The 40 MHz main crystal is 10 ppm (maintainer). It only governs time while awake; with no 32 kHz crystal, deep-sleep time runs on the internal RC. A user reported 1-2 min/month drift with one-time NTP (#253).

**Evidence:** #262 (lewisxhe 2024-07-08: 'The default is 10ppm crystal.'); #253.

**Action:** Measure deep-sleep drift per hour across temperatures; schedule NTP resync accordingly. Consider the internal 17.5 MHz/256 RTC clock source if ESP-IDF 6.1 offers it for S3 (better stability, a few uA more).

### K29: Wi-Fi antenna: v1.2 weak, v1.3 exists

Relevance: high. Novelty: new.

**Fact:** A user with ~100 v1.2 boards (touch and non-touch, PCB silkscreen 'T-Display S3 22-8-2 v1.2 XX2') found the onboard antenna very weak. Maintainer: 'The latest version is specifically optimized for the antenna'; 'The image you sent is of an older version, and its antenna performance is somewhat inferior'; 'specify that you need the latest v1.3 version' at lilygo.cc. External antenna: the U.FL is not connected by default; 'this is the 0201 resistor. You need to rotate it' (same on old and new boards). Replacement v1.3 units were 'much better'.

**Evidence:** #340 (lewisxhe 2026-04-13/14, 2026-05-19/26; user 2026-05-18); #188, #267 (U.FL not connected); README.md 'WiFi antenna' + image/WiFi.png @3c048f7.

**Action:** Expect weak RSSI on v1.2: NTP/association code needs generous timeouts and retries and must give up gracefully on battery. Buy v1.3 for new units. Keep metal and the battery away from the antenna end in the enclosure.

### K30: Wi-Fi association failures

Relevance: medium. Novelty: new.

**Fact:** #247: a board that scanned but never connected was judged defective by the maintainer ('I suspect it's a board problem'); the replacement worked. Another user fixed association with WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK). #221: 'Association refused temporarily, comeback time' then a crash (maintainer: ESP core issue). #297: NTP failed on one router under DHCP but worked with a static IP or a phone hotspot (unexplained). 2.4 GHz only (#250).

**Evidence:** #247 (lewisxhe 2024-05-13; mo-kuh 2024-07-07); #221 (2024-03-05); #297 (roblatour 2025-01-14); #250.

**Action:** Set the sta threshold authmode to WPA2_PSK (WPA3/PMF capable), handle the PMF comeback-time retry, use several NTP servers plus the DHCP-provided one, and log disconnect reasons to the USB console for support.

### K31: Wi-Fi current, brownout risk

Relevance: high. Novelty: new.

**Fact:** Maintainer: 'When WiFi is turned on normally, the average current is about 150mA+, and the heat center is in the ESP32 chip' (#295). Reboot-on-Wi-Fi-connect was diagnosed as 'a problem with the PCB power supply' (#275). The LDO is rated 500-600 mA vs ESP32 peaks ~700 mA (#291), and on battery the path adds R10 + Q4 + LDO dropout.

**Evidence:** #295 (2024-12-19); #275 (2024-09-09); #291 (2024-12-07).

**Action:** Don't start Wi-Fi below ~3.5 V cell (VSYS). Keep the backlight dim during association, enable brownout reset, and record the reset reason to detect brownout loops.

### K32: Bluetooth

Relevance: low. Novelty: confirms.

**Fact:** Maintainer: 'ESP32s3 doesn't support classic Bluetooth, only supports ble'.

**Evidence:** #200 (lewisxhe 2023-11-28).

**Action:** BLE-only provisioning (NimBLE) as planned.

### K33: ESP-IDF console

Relevance: low. Novelty: confirms.

**Fact:** With ESP-IDF, monitor output only appeared after the console was switched from the default UART0 to USB Serial/JTAG in menuconfig (USB-C is wired to native USB GPIO19/20; UART0 goes to GPIO43/44). Arduino builds stall at boot on battery when USB-CDC-on-boot is enabled (README FAQ 5, #291, #301).

**Evidence:** #266 (BullT 2024-07-26); README.md FAQ 5 @1cc41eb; schematic (ESP_USB_DP/DM to U5).

**Action:** CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG. Keep wake-path logging minimal and check that console writes never block when no USB host is attached.

### K34: USB-C CC pins unconnected (to verify)

Relevance: medium. Novelty: new.

**Fact:** In the published schematic the USB-C receptacle U5 has CC (A5) and VCONN/CC2 (B5) left unconnected: no 5.1 k Rd pull-downs are drawn. Spec-compliant USB-C sources (C-to-C cables, laptops, PD chargers) then won't enable VBUS. A user on another LilyGO S3 board (T3S3) found 'If you use a usb-c to usb-c cable it doesn't work. Needs to be usb-c to usb-a' (#233).

**Evidence:** schematic Charger block, U5 'TYPEY-C' pins A5 CC / B5 VCONN floating; #233 (franguyarmani 2024-03-16).

**Action:** Test charging/always-on mode with a C-to-C cable from a PD charger and a laptop. If it fails, document 'use a USB-A to C cable' or add Rd resistors in a hardware rev.

### K35: USB serial connect resets

Relevance: low. Novelty: new.

**Fact:** A user saw resets on serial disconnect (AMOLED S3 board, same native USB); maintainer: 'Can't disable'.

**Evidence:** #274 (lewisxhe 2024-08-23).

**Action:** For the Wi-Fi-credentials console command, expect that opening/closing a terminal may reset the board (DTR/RTS). Store credentials in NVS immediately and design the command flow to survive a reset.

### K36: flashing recovery

Relevance: low. Novelty: new.

**Fact:** When an app takes over USB (e.g. the TinyUSB HID example) or crashes early, esptool reports 'No serial data received'; recovery is manual download mode (hold BOOT, press and release RST, release BOOT). A sleeping board also has no USB device.

**Evidence:** #296 (2024-12-20), #201, #237, #308; README.md FAQ 4 @1cc41eb.

**Action:** Document BOOT+RST recovery. Since Gogadget deep-sleeps on battery, flash only while it is awake/on USB, or use BOOT+RST.

### K37: on-chip JTAG debugging

Relevance: low. Novelty: new.

**Fact:** Debugging over the built-in USB-JTAG needs OpenOCD config esp32s3-builtin.cfg (board JSON updated after #210). For breakpoints in flash code, the reporter suggests 'mem 0x40000000 0x4fffffff ro' in GDB.

**Evidence:** #210 (wsiok; lewisxhe 2024-03-05 'It has been updated'); boards/lilygo-t-display-s3.json @1cc41eb ('openocd_target': 'esp32s3-builtin.cfg'); #234/#235.

**Action:** Use idf.py openocd with board/esp32s3-builtin.cfg if on-target debugging is needed.

### K38: flash/PSRAM configuration

Relevance: low. Novelty: confirms.

**Fact:** Board definition: QIO flash at 80 MHz, 16 MB, octal PSRAM ('memory_type': 'qio_opi'), BOARD_HAS_PSRAM; the module is ESP32-S3R8 with an external W25Q128JV flash (schematic).

**Evidence:** boards/lilygo-t-display-s3.json @1cc41eb; schematic SOC block (U2 ESP32_S3R8, U3 W25Q128JV); #256 (16 MB / Huge APP).

**Action:** sdkconfig: ESPTOOLPY_FLASHMODE_QIO, 80 MHz, 16 MB, SPIRAM_MODE_OCT (PSRAM optional, since the draw buffers are internal DMA).

### K39: mechanical: display removal

Relevance: low. Novelty: new.

**Fact:** Maintainer: 'Choose to disassemble the screen bracket, not the display itself... only requires slowly peeling off the adhesive backing... disassembling the display itself is very easy to damage.' Another user cracked the panel trying.

**Evidence:** #335 (lewisxhe 2026-03-04, photo; MattObject 2026-06-19).

**Action:** If headers must be soldered for the enclosure, lift the bracket by its adhesive and never pry the glass.

### K40: mechanical CAD files

Relevance: low. Novelty: new.

**Fact:** dimensions/t-display-s3-full.stp opened corrupt for one user until downloaded via 'RAW' (#251). Requests for a touch-version or 'Long' case STL went unanswered (#244, #280).

**Evidence:** #251 (2024-05-20); #244; #280; dimensions/ folder @1cc41eb.

**Action:** Fetch STEP/STL files raw (git clone) for enclosure CAD.

### K41: no official LVGL 9 reference

Relevance: low. Novelty: new.

**Fact:** The repo still bundles LVGL 8.3.0-dev; a request for an LVGL 9 example (#283) got no answer.

**Evidence:** lib/lvgl/library.json @1cc41eb ('version': '8.3.0-dev'); #283.

**Action:** Expect to port the LVGL 8 flush/buffer setup to LVGL 9.6 + esp_lvgl_port yourself; rely on screenshot tests.

### K42: ADC accuracy

Relevance: medium. Novelty: refines.

**Fact:** LilyGO's battery example reads once with the legacy esp_adc_cal API (11 dB, default Vref 1100) and multiplies by 2. A user measured 3400 mV vs 3600 mV on a DMM. Maintainer: 'I only read the data once and did not do any data processing... the internal voltage reference of ESP32 is affected by the external power supply... Voltage data can only be used as a reference.'

**Evidence:** #190 (krupis/lewisxhe 2023-10-12); examples/GetBatteryVoltage/GetBatteryVoltage.ino:76-90@1cc41eb.

**Action:** Use adc_oneshot + adc_cali curve fitting at 12 dB, average 16-32 samples after the settle delay, and calibrate per unit against a DMM if accurate % is needed.

### K43: GPIO15 must be set first on battery

Relevance: medium. Novelty: refines.

**Fact:** Maintainer and code: GPIO15 must be HIGH 'at the very beginning' or the screen stays dark on battery (backlight and ADC divider unpowered); the panel itself still initialises because its logic supply is VDD3V3.

**Evidence:** #301 (lewisxhe 2025-03-22); #218, #284; examples/factory/factory.ino:252-254@1cc41eb ('IO15 must be set to HIGH before starting, otherwise the screen will not display when using battery').

**Action:** Drive GPIO15 HIGH as the first boot action on every wake, so the ADC divider settles while the panel initialises.

### K44: touch vs non-touch hardware

Relevance: low. Novelty: confirms.

**Fact:** Staff: 'The schematic of the T-Display-S3 Touch is the same as the T-Display-S3'. The only difference is whether the touch panel and controller occupy GPIO16/17/18/21; GPIO17 and 21 are free on non-touch boards (#346).

**Evidence:** #197 (Llgok 2023-10-26); #346 (lewisxhe 2026-08-07); #318.

**Action:** Touch-version issue reports about power/sleep/antenna apply to the non-touch v1.2 too.

### K45: 3V3 rail failure diagnostics

Relevance: low. Novelty: new.

**Fact:** Maintainer triage for dead boards: 3V3-to-GND resistance should be in the kohm-Mohm range; low ohms means a short in the LDO or a capacitor; a burned LDO is 'caused by excessive load current or a 3.3V short'. Input current stuck at ~0.28 A with 3.3 V present points to a damaged charger; U7 pin 4 is NC.

**Evidence:** #322 (lewisxhe 2026-01-14...03-17); #343 (2026-06-04...09); #348 (2026-09-14).

**Action:** Add these checks to the Gogadget hardware troubleshooting notes.

## Reusable code

### 1. AW9364 16-level pulse-count backlight control (setBrightness)

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:223-247 @1cc41eb (introduced in commit 2d3c294 for #278)

**Licence:** MIT (Copyright (c) 2022 Xinyuan-LilyGO, LICENSE at repo root)

**Port notes:** Keep the state machine: static level; value 0 -> EN LOW + >=3 ms; enabling from off -> EN HIGH + 30 us (chip starts at level 16); steps = (16 + (16-value) - (16-level)) % 16 LOW->HIGH pulses (each pulse lowers one level, wrapping 1->16). In ESP-IDF use gpio_set_level(GPIO_NUM_38, x) with esp_rom_delay_us(1) between edges, because back-to-back gpio_set_level is too fast compared with Arduino digitalWrite. Wrap the train in portENTER_CRITICAL/portEXIT_CRITICAL so no LOW stretches toward the shutdown time. Check limits against the AW9364 datasheet. Hold EN LOW in deep sleep with gpio_hold_en + gpio_deep_sleep_hold_en.

### 2. Deep-sleep entry with panel sleep-in and GPIO14 wake

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/Sleep/Sleep.ino:77-89 and 105-116 @1cc41eb (fc466c6, #300); examples/factory/factory.ino:395-404 @1cc41eb (c9fd58a, #289)

**Licence:** MIT (Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** LilyGO order: GPIO15 LOW, GPIO38 LOW, send 0x10 (SLPIN), esp_sleep_enable_ext0_wakeup(GPIO14, 0), esp_deep_sleep_start. In ESP-IDF: esp_lcd_panel_disp_on_off(panel,false) (0x28) -> esp_lcd_panel_io_tx_param(io,0x10,NULL,0) -> vTaskDelay(>=5 ms) -> configure CS=1, WR=1, RD=1, DC=0, D0-D7=0, RST (test LOW vs HIGH), GPIO38=0, GPIO15=0 as outputs -> gpio_hold_en() on each + gpio_deep_sleep_hold_en() -> esp_sleep_enable_ext1_wakeup_io(BIT64(0)|BIT64(14), ESP_EXT1_WAKEUP_ANY_LOW) -> esp_deep_sleep_start(). On wake call gpio_hold_dis/gpio_deep_sleep_hold_dis only after reconfiguring the pins. Do not drive GPIO16/17/18 low.

### 3. Battery/VSYS voltage read and USB-present threshold

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/GetBatteryVoltage/GetBatteryVoltage.ino:42-44 and 76-90 @1cc41eb (added 4b8e1ee for #190)

**Licence:** MIT (Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Original: legacy esp_adc_cal at 11 dB with Vref 1100, single analogRead(4), x2, and 'if (v1 > 4300)' meaning VBUS present (the example's 'No battery connect!' text is misleading). Port to adc_oneshot (ADC1, ADC_CHANNEL_3 = GPIO4, ADC_ATTEN_DB_12) + adc_cali_create_scheme_curve_fitting. Set GPIO15 HIGH first and wait >=3 ms (10 nF across the lower 100 k), average 16-32 samples, multiply by 2. Treat >~4.35 V with hysteresis as USB; otherwise map loaded VSYS to state of charge.

### 4. esp_lcd i80 + built-in ST7789 bring-up and landscape orientation

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:252-331 @1cc41eb

**Licence:** MIT (Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** GPIO15 HIGH first; GPIO9 (RD) as output HIGH; i80 bus 8-bit (DC 7, WR 8, D0-D7 39-42,45-48), CS 6, dc_cmd_level 0 / dc_data_level 1, lcd_cmd_bits/param_bits 8; esp_lcd_new_panel_st7789 (RST 5, RGB, 16 bpp); reset, init, invert_color(true), swap_xy(true), mirror(false,true) for USB on the left (mirror(true,false) for USB on the right), set_gap(0,35), then the CMD_1 table. Update the bus config for IDF 6.1: clk_src LCD_CLK_SRC_DEFAULT; the old psram/sram_trans_align fields are replaced by DMA burst settings. Use pclk 10 MHz (the factory's 16 MHz exceeds the ST7789V 66 ns write-cycle spec). In esp_lvgl_port set flags.swap_bytes=true, the LVGL 9 equivalent of LV_COLOR_16_SWAP 1. Enable the backlight only after the first flush.

### 5. ST7789 vendor init table LCD_MODULE_CMD_1 (already known; listed for completeness)

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:126-150 @1cc41eb (identical in examples/Sleep/Sleep.ino:19-42)

**Licence:** MIT (Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Send with esp_lcd_panel_io_tx_param(io, cmd, data, len & 0x7f) and delay 120 ms when len & 0x80 (only the leading 0x11). Keep the odd single-byte {0xD0,{0xA7},1} entry as-is (vendor code). Send on every init; the maintainer says pixels blur without it (#229).

## Open questions

- Is R12 (100 kohm from VBAT to GND in the schematic) actually fitted on v1.2 non-touch boards, and what is it for? It costs ~40 uA permanently.
- Which LDO is fitted on Gogadget's units (AP2112K, RT9013 or RT9080; quiescent current 2-55 uA), and which LCD finish (glossy or matte)?
- The 263 uA deep-sleep figure in #289 is ~120-190 uA above the component budget. Is the rest floating LCD bus inputs, the panel module, or the measurement setup? Which pin parking (RST LOW vs HIGH, D0-D7 held vs floating) gives the lowest current with no ghosting? LilyGO gives no guidance.
- Root cause of R10 fuse degradation (#223, #291): charge current, inrush, or a bad fuse batch? LilyGO never answered.
- Does the physical board have USB-C CC pull-downs? The schematic shows CC1/CC2 unconnected; a C-to-C charger and a laptop need testing.
- Does waking on GPIO0 (a strapping pin) via EXT1 behave reliably on every board? All official examples wake only on GPIO14.
- What changed in v1.3 besides antenna matching (power path, LDO, panel)? Will a v1.3 schematic be published?
- LilyGO's comment that a 'too low' pixel clock causes 'mosaic' (commit adc038e) is unexplained; no issue reports problems at 10 MHz.
- #297: NTP failed under DHCP but worked with a static IP on one router; never explained.
- #349: header GPIOs 1/12/13/16/17 reportedly always read HIGH when grounded. Closed with no maintainer reply; probably wiring, but unverified.
- README (2026-08) lists 'VBUS (pin header) 5V' and 'VBAT (pin header) 4.2V', but the schematic has only '+5V' (VSYS) on the headers and the maintainer says it is VSYS (#270) yet also 'VBUS = 5V' (#305). Which is right on current boards?
