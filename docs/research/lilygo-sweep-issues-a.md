# LilyGO sweep: issues and PRs #1 to #176

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 3 of 36 items were verified in lilygo-verification.md (2 confirmed / 1 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): RGB565 bytes are swapped in i80 hardware, not with esp_lvgl_port `swap_bytes` (I176-06 and the reusable code); NTP never uses DHCP-supplied servers and uses 1.ntp.vnix.vn and pool.ntp.org (I176-30 and the reusable code); local time is computed in core/ from the fixed UTC+7 offset, not through a TZ string; buttons use an own ISR + FSM driver, not espressif/button; a USB plug-in during sleep is noticed at the next wake (no ULP or timer polling).

Superseded by decision: on USB the epoch is saved to NVS every minute, not every 10 min (I176-11); the firmware shows only 'on USB', never 'charging' (I176-12, I176-17; ADR-0010); GPIO38 needs no deep-sleep hold (I176-12 and reusable item 2); the USB test is GPIO15 LOW then GPIO4 at about 0 V or 2.3 V, not a 4.3 V VSYS threshold (I176-12).

Scope: every issue, PR and comment from #1 to #176. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**Scope:** I read all 176 items: 166 issues and 10 PRs. All 10 PRs were merged, and they only touch docs, 3D files and CI. I also read all 956 comments, 232 of them from LilyGO staff (lewisxhe 99, mmMicky 85, LilyGO 48). I checked the claims against the schematic (three revisions in git, 2022/2023/2024), the Arduino examples at 1cc41eb, and LilyGo-Display-IDF at b1a1cc5. Most threads are Arduino or TFT_eSPI setup questions. The useful hardware facts sit in about 30 threads.

### Highest-impact findings

1. **Hold LCD_RD (GPIO9) high.** With esp_lcd i80 and the ST7789 on IDF 5, the screen stayed black or showed garbage until RD was driven high (#103). Every official init drives it high (`display_s3.c:64-69`, `factory.ino:257-258`). RD, CS and WR should also be held high during deep sleep.
2. **Switching between USB and battery reboots the board.** lewisxhe (#174): *"During the switching process, it will restart. Cannot switch seamlessly."* The handover from always-on (USB) to battery must survive a reset: persist the time and resync NTP.
3. **VBUS forces PWR_EN on.** VBUS and GPIO15 are diode-ORed (#51 *"controlled by either USB or GPIO"*, confirmed in the schematic). Consequences:
   - On USB, GPIO15 cannot turn off the green LED, the AW9364 or the ADC divider.
   - GPIO4 then measures the +5V net, not the battery.
   - Plugging in USB during deep sleep gives only about 2.3 V at GPIO4, below the guaranteed input-high level, so it is not a reliable wake source. Starting always-on mode from sleep needs ULP or timer polling.
4. **Deep-sleep handling of GPIO15.** Even the weak internal pull-up on GPIO15 turns the rail on (#51). Driving it high costs about 0.2 mA of base current. Hold it low, with pulls off, during deep sleep.
5. **Battery drains firmware cannot remove.** R12 (100 kΩ) sits directly across the battery (about 40 µA), and the 3.3 V regulator is always enabled. LilyGO's own figure (#12) is *"about 200uA"*.
6. **Never use the +5V header pin with a battery attached.** When USB is absent, that pin connects straight to the battery through Q4, so 5 V goes into the cell (#42, #51). It also causes an audible whine (#140, *"This is a hardware design problem"*).
7. **There are two panel generations.**
   - The CMD_1 init table is meant for the smooth (glossy) panel, not the frosted one (#87).
   - The RGB/BGR order can differ (#122: *"two versions of the screen"*).
   - Pure red renders orange; LilyGO says the gamma comes from the panel maker and they cannot tune it (#113, unresolved).
8. **Weak Wi-Fi is acknowledged by staff.** #67: *"gain of the onboard antenna is relatively weak"*; #123 was closed unresolved. An external antenna is possible by rotating a 0 Ω resistor next to the IPEX socket (#39). Plan NTP sync for poor signal.
9. **Buttons.** GPIO14 has a 100 kΩ pull-up plus a 100 nF capacitor, so releases are slow (about 10 ms time constant). GPIO0 has a 10 kΩ pull-up, no capacitor, and is the BOOT strapping pin.
10. **Battery ADC.** The divider is exactly 2:1 (100 kΩ / 100 kΩ, 1 %, with 10 nF). Staff say readings are inaccurate and read high (#57, #59). Wait for it to settle, average several samples, and calibrate per unit.

### Nuances against the "already known" list

- **Charge current:** the schematic labels R13 (2 kΩ) as **580 mA**, not about 525 mA. A "5V/2A" fuse sits in series with the battery connector.
- **"V1.2" does not identify the charger IC.** The schematic's charger changed TP4054 → TP4056B → TP4065, but boards already carried "V1.2" on the silkscreen in 2022 (#24, #168).
- **USB-C CC pins look unterminated** in every schematic revision (no 5.1 kΩ resistors), so USB-C-to-C chargers and cables may not supply power. lewisxhe asked about exactly this in #65; #166 also points that way.
- **I2C on SDA 18 / SCL 17 is confirmed** (#154). The vendor's website image was still wrong in 2025.
- **The 4-pin JST is UART0 (GPIO43/44) with no pull-ups.** It works as a UART log console for deep-sleep debugging (#108).
- **Community LEDC brightness snippets (#33, #74) are obsolete**, consistent with commit 02f0e9d.

### Mechanical

- Conductive shipping foam placed inside a case caused failures (#65).
- The LCD and its flex cable crack easily (#51, #128). The panel is held by double-sided tape at each end (#176).
- The official non-touch and board+screen 3D models were deleted in commit 899de7a but can be recovered from git.

### Still unresolved upstream

- Weak Wi-Fi.
- Gamma and red rendering.
- Intermittent colour inversion (#118).
- 1-Wire sensors failing on GPIO43 (#161).
- One unit that showed a black screen only with TFT_eSPI (#151).

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| I176-01 | display / esp_lcd i80 | high | new |  |
| I176-02 | display / panel variants (CMD_1) | medium | refines |  |
| I176-03 | display / colour order | medium | new |  |
| I176-04 | display / colour accuracy | medium | new |  |
| I176-05 | display / landscape mapping | medium | refines |  |
| I176-06 | display / RGB565 byte order | medium | refines |  |
| I176-07 | display / pixel clock | low | refines |  |
| I176-08 | display / intermittent glitch | low | new |  |
| I176-09 | display / image retention | low | new |  |
| I176-10 | display / sleep current | low | confirms |  |
| I176-11 | power / USB-battery changeover | high | new |  |
| I176-12 | power / PWR_EN forced by VBUS | high | refines | confirmed |
| I176-13 | power / GPIO15 in deep sleep | high | new |  |
| I176-14 | power / permanent battery drains | high | refines | confirmed |
| I176-15 | power / +5V header pin hazard | high | new |  |
| I176-16 | battery / charger | medium | refines |  |
| I176-17 | indicators / LEDs | low | new |  |
| I176-18 | USB-C / CC termination | medium | new |  |
| I176-19 | enclosure / conductive foam | medium | new |  |
| I176-20 | mechanical / LCD fragility | medium | new |  |
| I176-21 | mechanical / 3D models | low | new |  |
| I176-22 | Wi-Fi / antenna | high | new |  |
| I176-23 | Wi-Fi / external antenna option | medium | new |  |
| I176-24 | power supply / Wi-Fi start | low | new |  |
| I176-25 | PSRAM / flash configuration | medium | refines |  |
| I176-26 | flashing / recovery | medium | refines |  |
| I176-27 | console / UART0 on JST | medium | new |  |
| I176-28 | buttons | high | new |  |
| I176-29 | battery ADC | high | refines | partially |
| I176-30 | time / NTP / timezone | medium | new |  |
| I176-31 | hardware revision marking | low | refines |  |
| I176-32 | pins / I2C and UART1 | low | confirms |  |
| I176-33 | pins / JTAG and strapping overlap | low | new |  |
| I176-34 | USB identity | low | new |  |
| I176-35 | flash hardware failure | low | new |  |
| I176-36 | display / bus re-initialisation | low | new |  |

### I176-01: display / esp_lcd i80

Relevance: high. Novelty: new.

**Fact:** The ST7789 RD line (GPIO9) is not driven by the ESP-IDF esp_lcd i80 bus driver; if it floats or sits low, transfers are corrupted (black screen or garbage; touching the board edges changed the picture). Driving GPIO9 high (output high, or input with pull-up) before creating the bus fixes it. Every LilyGO init (IDF and Arduino) drives RD high; the maintainer could not find this cause himself.

**Evidence:** #103 (ESP-IDF v5.0 port): hoffmajs 2023-05-02 'PIN_LCD_RD should be driven high (by setting it to input with pullup) even though the bus driver itself does not use the pin, but if it is driven low, it would impact the transfer'; confirmed by simonchatts 2023-05-15 and KamranAghlami 2023-05-16 ('that was it'); KamranAghlami 2023-04-25 'if I touch sides of the board some times I get an screen refresh with the correct color but wrong shape'; mmMicky 2023-02-17 'I read carefully several times, but couldn't find the crux of the problem'. Code: LilyGo-Display-IDF main/display_s3.c:64-69@b1a1cc5; T-Display-S3 examples/factory/factory.ino:257-258@1cc41eb; examples/lv_demos/lv_demos.ino:156-157@1cc41eb; #160 Rust port also needed rd.set_high().

**Action:** In Gogadget's display init, call gpio_config on GPIO9 as an output set to 1 before esp_lcd_new_i80_bus(). Before deep sleep, gpio_hold_en() RD, CS and WR high (with gpio_deep_sleep_hold_en()): the panel stays powered and in SLPIN, so floating control lines must not clock garbage into it.

### I176-02: display / panel variants (CMD_1)

Relevance: medium. Novelty: refines.

**Fact:** There are two LCD panel generations. The LCD_MODULE_CMD_1 init table (custom porch, VCOM and power settings plus E0/E1 gamma) suits the newer panel with a smooth/glossy surface; frosted (matte) panels do not need it. No electronic way exists to tell them apart.

**Evidence:** #87 mmMicky 2023-01-30: 'LCD_MODULE_CMD_1 is suitable for optimizing screen display. This can only be observed intuitively. It's not easy to tell the model number of the screen in the complete package... Theoretically observe the screen surface, if it is smooth, it needs to be defined. If it is frosted, no definition is required.' examples/factory/factory.ino:14-16@1cc41eb: 'The product now has two screens, and the initialization code needs a small change in the new version. The LCD_MODULE_CMD_1 is used to define the switch macro.' (added 772b8d8, 2022-12-20). #48 mmMicky 2023-01-09: 'sometimes it will make the screen display better'.

**Action:** Keep sending the CMD_1 table (as LilyGo-Display-IDF does), but behind a Kconfig/NVS flag. Check the unit's glass (glossy or matte) and compare colour and gamma with and without the table before freezing the UI palette.

### I176-03: display / colour order

Relevance: medium. Novelty: new.

**Fact:** The colour element order can differ between panel batches. A non-touch unit bought in March 2023 needed RGB where LilyGO's bundled TFT_eSPI setup used BGR, and the maintainer said there are two screen versions. The bundled Setup206 switched from BGR to RGB only in 2026, though TFT_eSPI's own semantics also changed over time. LilyGO's esp_lcd code uses RGB order with inversion on.

**Evidence:** #122 daavery 2023-03-16 (non-touch); lewisxhe 2023-03-21: 'There are two versions of the screen, and my display is correct. When did you buy yours?' and 'Because my display with and without touch is normal' (closed stale, unresolved). lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h:10@1cc41eb = TFT_RGB (changed in f15652b 2026-03-02; TFT_BGR in c3dfc1a/04f26b4/0742ef9). #48 bratoff: BGR became necessary after a TFT_eSPI upstream change. LilyGo-Display-IDF main/display_s3.c:115,125@b1a1cc5 (ESP_LCD_COLOR_SPACE_RGB, invert true).

**Action:** Default to RGB element order plus esp_lcd_panel_invert_color(true). Add a console command that draws R/G/B bars and an NVS flag that flips to BGR without a rebuild.

### I176-04: display / colour accuracy

Relevance: medium. Novelty: new.

**Fact:** Saturated red (RGB565 0xF800) renders orange, and light greys get a green tint off-axis. LilyGO cannot tune the gamma because the panel maker supplies it; the issue was never fixed.

**Evidence:** #113 doh38 2023-03-03 (teastainGit: 'Me too!'); lewisxhe 2023-03-13: 'Sorry, the current initial gamma value is provided by the screen manufacturer. I don't know how to calibrate the gamma value. These values are generally provided by the screen manufacturer. I have already fed back to the screen manufacturer to see if they have any solutions'; closed stale 2024-05-22.

**Action:** Choose the Vietnamese UI palette on real hardware and do not depend on pure red for warnings. Treat simulator screenshots as layout goldens, not colour truth. Optionally A/B the CMD_1 E0/E1 gamma against the panel default.

### I176-05: display / landscape mapping

Relevance: medium. Novelty: refines.

**Fact:** LilyGO's esp_lcd landscape setup for 320x170 is: invert on, swap_xy = true, mirror(false, true) with the USB port on the left (or mirror(true, false) with it on the right), and gap (0, 35). One Rust/mipidsi user needed an offset of 36 instead of 35.

**Evidence:** LilyGo-Display-IDF main/display_s3.c:125-137@b1a1cc5 ('The screen faces you, and the USB is on the left' / 'the USB is to the right'); examples/factory/factory.ino:310-322@1cc41eb; #103 working IDF-5 config; #160 will-lynas 2025-07-19 '.display_offset(36, 0) // Not sure where this number comes from, but this is what works in testing'.

**Action:** Offer USB-left and USB-right orientation as a setting. On the device, draw a 1-px frame and confirm every edge row and column is visible with gap 35 (simulator tests cannot catch an off-by-one gap).

### I176-06: display / RGB565 byte order

Relevance: medium. Novelty: refines.

**Fact:** Over the 8-bit i80 bus, RGB565 pixels must be byte-swapped. LilyGO's IDF example enables LV_COLOR_16_SWAP, community IDF ports used the i80 swap_color_bytes flag, and a missing swap shows up as swapped or shifted colours.

**Evidence:** LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:3@b1a1cc5 (CONFIG_LV_COLOR_16_SWAP=y); #103 io_config.flags.swap_color_bytes = 1; zef/t-display-s3-espidf-lvgl src/display.cpp:87 (MIT); #84 mmMicky 2023-01-14 'Method 1: tft.setSwapBytes(true)'.

**Action:** With LVGL 9.6 and esp_lvgl_port 2.9, set lvgl_port_display_cfg_t.flags.swap_bytes = true (swap at flush). Keep the desktop simulator path unswapped so screenshot goldens stay in canonical RGB565.

### I176-07: display / pixel clock

Relevance: low. Novelty: refines.

**Fact:** LilyGO runs the i80 bus at 16 MHz (factory example), 6.528 MHz (lv_demos) and 10 MHz (IDF example); community IDF ports used 2 MHz. One unit stayed black only with TFT_eSPI's bit-banged bus but worked with esp_lcd; LilyGO called that unit faulty.

**Evidence:** examples/factory/pin_config.h:25@1cc41eb (16*1000*1000); examples/lv_demos/pin_config.h:14@1cc41eb (6528000 //(10*1000*1000)); LilyGo-Display-IDF main/display_s3.c:23@b1a1cc5 (10 MHz); #103 and zef pin_config.h (2 MHz); #151 lewisxhe 2023-05-22 'There is no doubt about it, it can be determined that it is a malfunction', 2023-05-26 'It is not normal.'

**Action:** Keep 10 MHz (inside LilyGO's own range), but expose the pixel clock in Kconfig so a marginal unit can drop to about 6.5 MHz.

### I176-08: display / intermittent glitch

Relevance: low. Novelty: new.

**Fact:** A developer driving the ST7789 through the esp_lcd i80 driver on a custom carrier PCB saw intermittent colour inversions lasting about 1 s (yellow-on-red became purple-on-green), as if INVON had been lost. LilyGO had never seen it and it was never resolved.

**Evidence:** #118 tonystuart 2023-03-10; lewisxhe 2023-03-13 'I have not encountered this situation. I have added a tag to your issue'; closed stale 2024-05-22.

**Action:** In always-on (USB) mode, re-send INVON, COLMOD and MADCTL every few minutes (microseconds of bus time). Keep RD held high (I176-01), which removes one known source of bus corruption.

### I176-09: display / image retention

Relevance: low. Novelty: new.

**Fact:** The maintainer put one unit's half-screen garbage and black mask down to an OLED-like burn-in effect on the LCD.

**Evidence:** #128 lewisxhe 2023-03-24: 'This may not be possible, as it may be a screen burning phenomenon similar to OLED.'

**Action:** For the static always-on USB clock, shift the layout a few pixels periodically and/or dim or blank at night.

### I176-10: display / sleep current

Relevance: low. Novelty: confirms.

**Fact:** Turning the backlight off and sending DISPOFF still left about 30 mA more draw than a board whose display was never initialised. DISPOFF is not a low-power state.

**Evidence:** #33 wichjan 2023-06-21: 'it does NOT result in a complete shutdown of the display ... the board is drawing approx. 30mA more current compared to the consumtion when the board is running with the display never being activated' (no staff answer).

**Action:** Before deep sleep use SLPIN (0x10) and pull the backlight EN low (as factory.ino:397 does after the #289 fix); never rely on DISPOFF alone.

### I176-11: power / USB-battery changeover

Relevance: high. Novelty: new.

**Fact:** The board restarts when its supply switches between USB and battery; the changeover is not seamless. In the schematic, the power path is Q4 (SI2307 P-FET) from the battery to the +5V net, with its gate tied to the VBUS node (R14 100 kΩ to GND, D3 1N5819 from VBUS to +5V). That the reboot happens during this handover is my inference from the circuit.

**Evidence:** #174 (asked: run from USB with a battery as buffer, USB drops then returns) lewisxhe 2023-07-21: 'During the switching process, it will restart. Cannot switch seamlessly'. schematic/T_Display_S3.pdf@1cc41eb Power block (Q4, D3, R14).

**Action:** Treat every plug or unplug as a possible reboot. On each boot, log esp_reset_reason() and check USB presence via GPIO4. Restore the last-known epoch and a 'synced' flag from RTC memory/NVS (write about every 10 min while on USB). Show the time as unsynced until NTP succeeds. Bench-test plug and unplug with Wi-Fi and backlight active, and record which reset reason appears and whether the RTC time survives.

### I176-12: power / PWR_EN forced by VBUS

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `issues-a:I176-12`).

**Fact:** PWR_EN is a diode-OR of VBUS (D2) and GPIO15 (D1), feeding R5 10 kΩ into Q1 (PMBT3904, with R7 100 kΩ pull-down). Q1 drives Q2 (+5V to the BAT net that feeds the ADC divider) and Q3 (VDD3V3 to V3V, which feeds the green LED, AW9364 VIN and the LCD LED anode on FPC pin 20). So on USB the rail, green LED and divider are on whatever GPIO15 does, and GPIO4 measures the +5V net (VBUS minus the Schottky drop), not the cell.

**Evidence:** mmMicky #51 2022-11-21 (schematic snippet): 'There is a PWR_EN label here on the schematic. It is controlled by either USB or GPIO.'; schematic/T_Display_S3.pdf@1cc41eb Power block (D1/D2 1N4148, R5, R7, R8, Q1, Q2/Q3 SI2301) and Display block (CN1 pin 20 = V3V, U4 AW9364 VIN = V3V); sheet note 'Battery power must set IO15 to HIGH'.

**Action:** Detect USB by GPIO4 (scaled reading above about 4.3 V). While on USB, show 'charging' without a percentage, because the cell voltage cannot be measured. Hold GPIO38 low in deep sleep, since V3V is live whenever USB is present. Plugging USB in during deep sleep puts only about 2.3 V on GPIO4, below the guaranteed input-high level (0.75·VDD ≈ 2.48 V), so ext1 is not a reliable wake. If always-on mode must start from sleep, poll GPIO4 (ADC1_CH3) with the ULP-RISC-V or a periodic timer wake.

### I176-13: power / GPIO15 in deep sleep

Relevance: high. Novelty: new.

**Fact:** Even the weak internal pull-up on GPIO15 (e.g. INPUT_PULLUP) turns the PWR_EN rail on. Driving GPIO15 high costs about 0.2 mA of Q1 base current through R5 (10 kΩ), plus about 26 µA in R7 and about 40 µA in R8. The claim in #51 that the pull-up 'persists when power is removed' is wrong.

**Evidence:** #51 wiegleyj 2022-12-20 (gpio_reset_pin or INPUT_PULLUP on GPIO15 powers the LCD) and doh38 2023-03-10 ('it indeed works perfectly on arduino with the line you gave'); schematic Power block R5 10K, R7 100K, R8 100K, D1 1N4148 @1cc41eb.

**Action:** Before esp_deep_sleep_start(): set GPIO15 to 0, disable its pull-up, enable its pull-down, then call gpio_hold_en(15) and gpio_deep_sleep_hold_en(). Do not call gpio_reset_pin(15): in IDF 5.x it enables the pull-up (check the v6.1 behaviour). Keep GPIO15 high only while the screen or ADC is in use.

### I176-14: power / permanent battery drains

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `issues-a:I176-14`).

**Fact:** R12 (100 kΩ) sits directly across VBAT, drawing about 37–42 µA all the time. The 3.3 V regulator (AP2112K-3.3, alternate RT9013) is always enabled (EN pulled to +5V through R11 100 kΩ). Together they set a deep-sleep floor that firmware cannot remove. LilyGO quoted about 200 µA in deep sleep for the non-touch board.

**Evidence:** schematic/T_Display_S3.pdf@1cc41eb Power block (R12 between VBAT and GND; U7 EN tied through R11); #12 DrJaymz 2022-10-20 'looks like VBAT is loaded with another 100K R12 which is another 40uA'; LilyGO 2022-10-13: 'The T-Display-S3 without touch when entering deep sleep is about 200uA.'

**Action:** Budget at least 0.2–0.26 mA while asleep: a 250 mAh cell lasts about 40 days with no wakes at all. Keep wake time and backlight energy minimal. Optional hardware mod: remove R12 to save about 40 µA (about 15 % of the floor).

### I176-15: power / +5V header pin hazard

Relevance: high. Novelty: new.

**Fact:** The '+5V' header pin (P2 pin 1) is the +5V net, which connects to the battery through Q4 whenever USB is absent. Feeding 5 V into it pushes 5 V straight into the cell, bypassing the charger. With GPIO15 low it also produces an audible high-frequency whine. Despite this, staff told users the pin can power the board.

**Evidence:** #42 DevJulian1966 2023-11-01: 'The charging voltage jumps up to 5V, because of backpowering. If you're using an unprotected battery it might blow up'; #51 doh38 2023-03-10: 'when powered through 5v Pin... the battery output is 5V... not 4.2V'; #140 dima-fedotovs 2023-04-16 ('very quiet high frequency noise sound' when output 15 is LOW); lewisxhe 2023-04-17: 'This is a hardware design problem. The board is designed to use the IO port to turn on the power supply design of the screen.'; #51 mmMicky 2022-11-21: 'From here the port can be powered with 5V.'; schematic Power and GPIO-header blocks @1cc41eb.

**Action:** Power and charge Gogadget only through USB-C (plus the 2-pin JST cell). Leave the +5V pin unconnected. Any dock contacts must feed VBUS (e.g. at the USB-C pads), never the +5V pin. Do not put a second battery on the header pins (#175). Even the protected cell's overcharge cut-off should not be relied on.

### I176-16: battery / charger

Relevance: medium. Novelty: refines.

**Fact:** The battery can only be charged from USB VBUS (TP4065 VCC = VBUS; VBUS is not on any header). The PROG resistor R13 is 2 kΩ, annotated 'R13 IBAT (mA) = 580'. The battery connector P3 sits behind R10, labelled as a '5V/2A' fuse. Across schematic revisions the charger IC changed from TP4054 to TP4056B to TP4065, with R13 at 2 kΩ throughout.

**Evidence:** #42 mmMicky 2022-11-12: 'We have not considered this situation. Maybe you can solve it by soldering a wire from VBUS.'; schematic Charger block @1cc41eb (R13 note, R10 '5V/2A保险丝'); older sheets c19f3ad (dated 2022/10/10, TP4054), 811c949 (2023-03-13, TP4056B), b0a5ecd (2024-04-29, TP4065).

**Action:** Assume a charge current of 0.5–0.58 A, which is more than 2C for the 250 mAh protected cell. If you modify the board, raise R13 per the TP4065 datasheet table to reach about 0.5–1C. Otherwise give the cell airflow and avoid charging it hot.

### I176-17: indicators / LEDs

Relevance: low. Novelty: new.

**Fact:** The red LED is the TP4065 CHRG output, powered from VBUS through 2 kΩ; it does not drain the battery and the MCU cannot read it. It blinks with no battery, stays on while charging, and turns off when full or not charging. The green LED (2 kΩ on V3V) follows PWR_EN, so it is always on with USB. The WS2812 drawn in the schematic is not fitted.

**Evidence:** #31 LilyGO 2022-10-24: 'It flashes when you don't have the battery plugged in. It's constant when you plug in the battery and it's charging. Turns off when you are not charging or when the battery is fully charged.'; #57 mmMicky 2022-12-02 ('The other one is connected to the same line as the backlight to be controlled together with the backlight') and 2022-12-03 ('There is no WS2812 led light on this board'); schematic Charger/Power blocks @1cc41eb.

**Action:** Gogadget cannot read the charge state. On USB show a generic charging glyph; a fast-blinking red LED tells the user no battery is connected.

### I176-18: USB-C / CC termination

Relevance: medium. Novelty: new.

**Fact:** In all three schematic revisions the USB-C receptacle's CC (A5) and VCONN/CC2 (B5) pins are unconnected, with no 5.1 kΩ resistors to ground. Standards-compliant USB-C sources (C-to-C cables, PD chargers, power banks) may therefore never enable VBUS, while A-to-C cables always work.

**Evidence:** schematic/T_Display_S3.pdf@1cc41eb Charger block (U5 A5 'CC' and B5 'VCONN' open; identical in the c19f3ad and 811c949 sheets); lewisxhe #65 2023-04-25: 'So you are using the USB-C TO USB-C interface? The USB-C can't supply power to the board, so it can't be programmed?'; #166 david65536 2023-06-28: 'The USB-C to USB-C (thunderbolt 3) cable continued to show up as an unrecognized device.' Counter-evidence: the #65 and #143 wall-charger failures were finally traced to foam and firmware.

**Action:** Specify an A-to-C cable and charger for Gogadget. Measure CC-to-GND resistance on your unit. If a C-to-C or PD charger must work, add 5.1 kΩ from each CC pin to GND.

### I176-19: enclosure / conductive foam

Relevance: medium. Novelty: new.

**Fact:** The foam LilyGO ships around the board caused malfunctions when reused inside a case (screen barely lit, sketch not running on wall chargers). Removing the foam fixed it.

**Evidence:** #65 jessedmatlock 2023-04-25: 'that foam is either conductive or something.. because after removing the foam IT WORKS'; lewisxhe: 'Unbelievable, but now that it works it's great'.

**Action:** Never reuse the shipping foam in the Gogadget enclosure. Insulate the PCB back side (Kapton or fish paper) where it meets the battery or case.

### I176-20: mechanical / LCD fragility

Relevance: medium. Novelty: new.

**Fact:** The LCD glass and its flex cable crack easily: several users broke them pressing the board into a breadboard, the bottom of the flex (where the data lines are) being most fragile. The panel is held only by double-sided tape at each end, which can let go in warm climates.

**Evidence:** #51 OXOTHuK1976 2022-11-21 ('The screen is very fragile, it cracked when I tried to insert the device into the breadboard'); #128 dejavu1987 2023-04-22, mickeyl 2023-05-15 ('the flatband connector is really finnicky, especially the lower part where the data lines are'), svdrummer 2023-04-22 ('I have several that fall apart due to warmer climates'); #176 teastainGit 2023-07-27 ('The display is taped down at each end and can easily be lifted... Re-attach with fresh two sided tape').

**Action:** Design the enclosure to clamp the PCB edges, never the glass. Keep pressure off the flex-cable end. Allow for tape creep in Vietnamese heat (a retaining lip over the panel ends).

### I176-21: mechanical / 3D models

Relevance: low. Novelty: new.

**Fact:** Two official models are gone from the current tree: the non-touch 'noTouch.stl' and the board+screen+acrylic STEP from PR #165. Both were deleted when the 3D folder became dimensions/ in 899de7a, but git history still has them. dimensions/ now holds the full and PCB STEP files, PCB.dwg and LilyGO shell STLs, and recommends a 7×18×50 mm Li-Po.

**Evidence:** #133 lewisxhe 2023-04-06 'Done ! .../3D_file/noTouch.stl'; PR #165 (merged 2023-06-28) added '3D_file/LILYGO T-Display-S3 H569 Not Soldered.step'; both deleted in 899de7a (2024-01-31 'Rename shell size directory'); dimensions/README.MD@1cc41eb 'Battery Recommendation:3.7V Li-Po battery, 7mm wide 18mm long 50mm'; #36 tyeth glass offsets (~1.5 mm from top edge, ~5–5.5 mm board-to-glass).

**Action:** Recover the models with git show '899de7a^:3D_file/LILYGO T-Display-S3 H569 Not Soldered.step' and git show 899de7a^:3D_file/noTouch.stl, and design the case around dimensions/t-display-s3-full.stp.

### I176-22: Wi-Fi / antenna

Relevance: high. Novelty: new.

**Fact:** Weak Wi-Fi from the onboard antenna is widespread and acknowledged by staff: 1–2 bars next to the access point, pings of 200 ms or more, unchanged with Bluetooth disabled, the same under ESP-IDF and Arduino. A promised antenna redesign was never confirmed, and the issue was closed stale without a fix.

**Evidence:** #67 mmMicky 2022-12-22: 'The RF signal is also modulated, maybe the gain of the onboard antenna is relatively weak. It is expected that the antenna scheme will be changed in the next version.'; #123 MicroMkr 2023-03-17 (several units), kdg3737 2023-12-29 ('ping times of 200ms and more. Tested with bluetooth disabled, on both ESPIDF and Arduino'), 8OND007 2023-12-23, richardsg307 2024-01-08 (a 2.4 GHz extender fixed it); lewisxhe 2023-03-21 only asked 'Can you provide a picture of the antenna section?'; closed stale 2024-05-20.

**Action:** Make NTP sync tolerate weak signal: long connect timeouts, exponential backoff, several NTP servers, and log RSSI. Keep the antenna end (top, opposite the USB-C, the 'XX2' area) clear of the battery, metal and fingers. Do BLE provisioning close to the phone. Never let a failed sync block the 10 s time display.

### I176-23: Wi-Fi / external antenna option

Relevance: medium. Novelty: new.

**Fact:** The board has an IPEX socket. A 0 Ω resistor beside it, at the top end next to the onboard antenna, selects the path; rotating it 90° switches to the external antenna. User results with an external antenna were mixed.

**Evidence:** #39 LilyGO 2022-11-02: 'It can use the onboard antenna. The jumper is only necessary when you need to use an external antenna.'; mmMicky 2022-11-07 (photo with the resistor arrowed): 'Switching to external requires rotating this resistor 90 degrees.'; #123 MicroMkr: external antenna 'with the same poor results'; #67 teastain 'Someone even added an external antenna with no improvement'.

**Action:** If the enclosure shields the antenna, rotate the 0 Ω resistor and route an IPEX pigtail; measure RSSI before and after rather than assuming an improvement.

### I176-24: power supply / Wi-Fi start

Relevance: low. Novelty: new.

**Fact:** With a power-hungry Wi-Fi firmware (Tasmota), the soft-AP did not appear when the board ran from a PC USB port but did on a stronger power supply.

**Evidence:** #18 Scanboostar 2023-01-08: 'Please try a power supply that can deliver more current... On the USB port of my PC no Ap, using a Raspberry PowerSupply is presented de AP'; jkrijger67 confirmed.

**Action:** In always-on USB mode, recommend a proper 5 V adapter. Start Wi-Fi only after the display is up and the backlight has ramped, to avoid stacking current peaks.

### I176-25: PSRAM / flash configuration

Relevance: medium. Novelty: refines.

**Fact:** The module is an ESP32-S3R8 with 8 MB of in-package octal PSRAM (boot log: AP Memory, generation 3, 64 Mbit, 3 V) plus an external 16 MB Winbond W25Q128JV running QIO at 80 MHz. Configuring the PSRAM as quad gives 'PSRAM ID read error: 0x00ffffff', then 'Failed to init external RAM' and a boot loop. Boards from 2022–23 report ESP32-S3 revision v0.1.

**Evidence:** #103 KamranAghlami 2023-02-16 boot log ('octal_psram: vendor id : 0x0d (AP)', 'density : 0x03 (64 Mbit)', 'VCC : 0x01 (3V)', 'qio_mode: Enabling QIO for flash chip WinBond', 'chip revision: v0.1'); krupis 2023-05-23 ('quad_psram: PSRAM ID read error: 0x00ffffff', 'Failed to init external RAM!'; booted fine after disabling PSRAM); #41 mmMicky 2022-11-07 '...silk screen inside is 25Q128. If so, it is 16MB FLASH'; #71 mmMicky 2022-12-22 'board_build.arduino.memory_type = qio_opi'; esptool 'Chip is ESP32-S3 (revision v0.1)' in #76, #100, #106, #138, #162; LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:7-8@b1a1cc5 (CONFIG_SPIRAM=y, CONFIG_SPIRAM_MODE_OCT=y).

**Action:** In sdkconfig.defaults set CONFIG_ESPTOOLPY_FLASHMODE_QIO, FLASHFREQ_80M and FLASHSIZE_16MB. Set either CONFIG_SPIRAM=n (the draw buffers are internal) or CONFIG_SPIRAM_MODE_OCT=y, never quad. On an early board, check that IDF 6.1's CONFIG_ESP32S3_REV_MIN still allows v0.1.

### I176-26: flashing / recovery

Relevance: medium. Novelty: refines.

**Fact:** The ROM first-stage bootloader cannot be overwritten, so the board cannot be bricked. An app that crash-loops makes the USB-Serial-JTAG port appear and disappear repeatedly, and an app that floods the console can block esptool's auto-reset. Recovery is manual download mode. After flashing in manual download mode the app runs only after RST or a power cycle.

**Evidence:** lewisxhe #137 2023-04-06 'the first-level boot is unchangeable, so the esp32 can never be bricked'; #116 2023-03-13; exact sequence lewisxhe #162 2023-06-16 '1. connect the board via the USB cable 2. press and hold the BOOT button While still pressing the BOOT button, press RST 3.release the RST 4.release the BOOT button'; LilyGO #16 2022-10-08 'There will be no response after uploading the program in download mode. User code execution begins only after a reset.' and 'the program keeps resetting and constantly enumerating the device'; LilyGO #7 2022-08-27 'it needs to be powered on again'; #100 pdurys and #150 369Martin369 (console flooding blocked uploads).

**Action:** Document the BOOT+RST recovery in the Gogadget README. Rate-limit logs. Add a console command that reboots into ROM download mode (e.g. REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT); esp_restart()) so flashing never needs the buttons. Note that USB-Serial-JTAG disappears during deep sleep.

### I176-27: console / UART0 on JST

Relevance: medium. Novelty: new.

**Fact:** The 4-pin JST-SH 1.0 (WAFER-100W4A) beside the USB-C is, from the USB side: GND, 3V3 (always on), GPIO43 (U0TXD), GPIO44 (U0RXD). The pins go straight to the ESP32-S3 with no pull-ups, and the same signals appear on header P1 pins 11/10. LilyGO points users to it for FTDI flashing. I2C works there via Wire.begin(43, 44); DS18B20 1-Wire did not, and the cause was never found.

**Evidence:** #108 lewisxhe 2023-03-13 (photo of the connector): '43 and 44 are the TX and RX of ESP32S3 Please note that if you use uart to upload, you must put ESP32S3 into the download mode each time you upload the program'; #161 lewisxhe 2023-06-16 'T display s3 io43, io44 is directly connected to esp32s3, there is no additional circuit, I also encountered the same problem with ds18b20 before', 2023-06-27 'I didn't solve it'; #85 teastainGit 'From the USB it is: G, 3.3V, SDA 43, SCL 44'; #26 LilyGO '4Pin is the IIC connector'; schematic GPIO-header block (CNC pins 1–4, P1 pins 10/11) @1cc41eb.

**Action:** Use a 3.3 V USB-UART on the JST as a secondary log console (UART0 on 43/44) to capture wake, sleep and reset-reason logs that USB-Serial-JTAG misses. ROM boot messages also come out on GPIO43, so do not hang timing-sensitive single-wire devices there.

### I176-28: buttons

Relevance: high. Novelty: new.

**Fact:** GPIO0 (BOOT) has a 10 kΩ pull-up and no capacitor. GPIO14 (KEY) has a 100 kΩ pull-up plus 100 nF to GND: a 10 ms time constant, so a release takes about 14 ms to reach the input-high level. That makes GPIO14 hardware-debounced but slow. GPIO0 is the boot strapping pin: a chip reset while it is held lands in ROM download mode. Peripherals can also claim GPIO0; one I2S library defaulted MCLK to it. LilyGO's factory deep sleep wakes on GPIO14 via ext0.

**Evidence:** schematic Button block @1cc41eb (R1 10K + S2 on GPIO0; R3 100K, C14 100nF, S3 on KEY); #7 log 'rst:0x15 (USB_UART_CHIP_RESET),boot:0x0 (DOWNLOAD(USB/UART0)) ... waiting for download'; #101 pdurys 2023-02-14 ('audioI2S library was setting GPIO0 for MCLK by default' gave a permanent long-press); examples/factory/factory.ino:402@1cc41eb esp_sleep_enable_ext0_wakeup(PIN_BUTTON_2, 0).

**Action:** In the two-button driver: debounce GPIO0 in software (about 20–30 ms); for GPIO14, rely on the RC and allow at least 20 ms of settling before deciding press or release. No internal pulls are needed (the external ones exist), so disable RTC pulls in sleep. Wake with ext1 ANY_LOW on GPIO0|GPIO14. Never route CLK_OUT or I2S MCLK to GPIO0. Note that if a reset (I176-11) happens while BOOT is held, the clock will sit in download mode.

### I176-29: battery ADC

Relevance: high. Novelty: refines. Verification: partially (see lilygo-verification.md, `issues-a:I176-29`).

**Fact:** Battery sense goes from the BAT net (switched by PWR_EN) through a 100 kΩ 1 % / 100 kΩ 1 % divider with 10 nF to GPIO4: exactly ÷2, about 50 kΩ source impedance, about 0.5 ms time constant. LilyGO staff say the high-value divider makes readings inaccurate and too high, and users report ADC noise that depends on CPU load.

**Evidence:** schematic ADC block @1cc41eb (R2/R4 '100K%1', C13 10nF); #22 LilyGO 2022-10-10 '`batValue = (analogRead(4)*2*3.3)/4096;//(unit:V)` The current battery voltage value can be roughly obtained'; #57 mmMicky 2022-12-03 'I think the current of the sampling resistor is too small. lead to inaccurate sampling. I think it may be more accurate if this resistor is replaced with a smaller value.'; #59 mmMicky 2022-12-12 'I feel that the overall voltage is too high'; #152 369Martin369 2023-05-20 (unstable with CPU load, no staff answer).

**Action:** On wake: set GPIO15 high, wait at least 5 ms for Q2 and the RC to settle, then take 16–32 oneshot samples (12 dB attenuation, curve-fitting calibration) before Wi-Fi or the backlight ramp. Use the median or mean, and apply a per-unit two-point correction stored in NVS. Ignore readings while on USB (I176-12).

### I176-30: time / NTP / timezone

Relevance: medium. Novelty: new.

**Fact:** LilyGO's factory clock looks up the timezone at https://ipapi.co/timezone/ and maps it through a posix_tz_db table, where Asia/Ho_Chi_Minh is '<+07>-7'. SNTP uses pool.ntp.org and time.nist.gov and also accepts NTP servers supplied by DHCP.

**Evidence:** examples/factory/pin_config.h:13-21@1cc41eb; examples/factory/zones.h:251@1cc41eb ({"Asia/Ho_Chi_Minh", "<+07>-7"}); examples/factory/factory.ino:453-471@1cc41eb (sntp_servermode_dhcp + configTime); #80 mmMicky 2023-01-31 'I updated the method to get the timezone automatically'; PR #93 (merged 2023-02-01) adds a CUSTOM_TIMEZONE override.

**Action:** Hard-code TZ='<+07>-7' (Vietnam has no DST) and skip the IP lookup. Use esp_netif_sntp with server_from_dhcp plus pool.ntp.org / vn.pool.ntp.org fallbacks. On battery wakes, show the RTC time immediately and never block on NTP.

### I176-31: hardware revision marking

Relevance: low. Novelty: refines.

**Fact:** 'V1.2' on the silkscreen does not identify a hardware generation. Boards marked 'T_Display_S3 22-8-2 V1.2' already existed in 2022, while the published schematic's charger changed from TP4054 (2022) to TP4056B (2023-03) to TP4065 (2024-04). Staff pinout images show an even older 'T_Display_S3 22-5-18 V1.0' board.

**Evidence:** #24 alandsidel 2022-10-12 'My PCBs are marked V1.2 on the silk screen'; #168 shyney7 2023-06-29 'The Version No. on the pcb says: T_Display_S3 22-8-2 V1.2'; schematic history c19f3ad / 811c949 / b0a5ecd; lewisxhe #140 pinout image (silk 'T_Display_S3 22-5-18 V1.0').

**Action:** Read the marking on charger U6 (near the battery JST) to know the charge current, rather than trusting the V-number. This adds nuance to the known point that v1.2 changed only the charger IC (#236).

### I176-32: pins / I2C and UART1

Relevance: low. Novelty: confirms.

**Fact:** The maintainer confirmed SDA 18 / SCL 17 after a wrong pinout image; the image on lilygo.cc was still wrong in 2025. UART1 is used on RX 18 / TX 17. In the schematic these header nets carry 10 kΩ pull-ups to 3V3 (the touch I2C).

**Evidence:** #154 lewisxhe 2023-06-01 'The schematic is correct and the sketch is marked The pin diagram has been updated'; paco3346 2025-05-16 'The image on the manufacturer's site ... is still incorrect ... sda: 18 scl: 17'; #164 lewisxhe 2023-06-18 'Serial1.begin(9600, SERIAL_8N1, 18, 17);'; schematic Display block RIIC/RIID 10K to VDD3V3 @1cc41eb.

**Action:** If a sensor is added later, use 17/18 (already pulled up) or 43/44 (no pull-ups), and ignore the vendor image's SDA/SCL labels.

### I176-33: pins / JTAG and strapping overlap

Relevance: low. Novelty: new.

**Fact:** LCD D0–D3 occupy the ESP32-S3 JTAG pins (GPIO39–42 = MTCK/MTDO/MTDI/MTMS), and D4/D5 sit on strapping pins GPIO45/46. The 2022 schematic notes GPIO45/46 default to pull-down and that GPIO46 = 1 together with GPIO0 = 0 is an invalid boot combination.

**Evidence:** schematic SoC block @1cc41eb (U2 pins 44–52); c19f3ad sheet note 'GPIO0 ----默认上拉 / GPIO45 ----默认下拉 / GPIO46 ----默认下拉 / GPIO46(1) -----GPIO0(0) 不可使用'.

**Action:** Debug only through the built-in USB-JTAG (an external JTAG probe is impossible). Never add pull-ups on LCD D4/D5.

### I176-34: USB identity

Relevance: low. Novelty: new.

**Fact:** The board enumerates as the generic USB-Serial-JTAG 303A:1001, which IDEs misidentify (e.g. as 'Deneyap Kart G'). Espressif has allocated LilyGO T-Display-S3 PIDs: 0x813E (Arduino), 0x813F (CircuitPython), 0x8140 (UF2).

**Evidence:** #23 jeremyamoore 2022-12-05 (espressif/usb-pids allocated-pids.txt); #41 hwids 0x303A/0x1001; #112 LuigiPulcini 'BN: Deneyap Kart G VID: 0x303A PID: 0x1001'; #162 'u-blox NORA-W10'.

**Action:** No action for the USB-Serial-JTAG console. Host scripts should match on VID/PID or serial number, not product names. The allocated PID 0x813E exists if TinyUSB is ever used.

### I176-35: flash hardware failure

Relevance: low. Novelty: new.

**Fact:** For random 'Unable to verify flash chip connection (No serial data received.)' upload failures together with a dying serial monitor, the maintainer's diagnosis was a damaged flash chip.

**Evidence:** #138 lewisxhe 2023-04-09: 'in my experience, this should be a flash damage. If possible, you can try replacing a flash, which can be repaired most of the time'; 2023-04-10 'Yes, I also think it is a hardware problem'.

**Action:** If a Gogadget unit shows intermittent flash-verify errors that BOOT+RST download mode does not cure, suspect the W25Q128JV before the firmware.

### I176-36: display / bus re-initialisation

Relevance: low. Novelty: new.

**Fact:** Creating the i80 bus again without releasing the previous one fails with error 261 (0x105, ESP_ERR_NOT_FOUND) in esp_lcd_new_i80_bus.

**Evidence:** #142 codiak6335 2023-04-21: 'OSError: 261(esp_lcd_new_i80_bus)' when re-running the lcd.I8080 + ST7789 init without a hardware reset.

**Action:** If Gogadget tears the display down (e.g. leaving always-on mode without a reboot), call esp_lcd_panel_del, esp_lcd_panel_io_del and esp_lcd_del_i80_bus in that order before re-creating.

## Reusable code

### 1. i80 + ST7789 init that drives RD (GPIO9) high, sends the LCD_MODULE_CMD_1 init table, sets landscape (invert, swap_xy, mirror for USB on the left or right, gap 0/35) and runs at a 10 MHz pixel clock

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:20-148 @ b1a1cc54994bf1b417e3bb30c437bbe1036bff7f (pins in main/product_pins.h)

**Licence:** MIT (file header: Lewis He, Shenzhen Xinyuan Electronic Technology Co., Ltd, 2024)

**Port notes:** Keep gpio_config(GPIO9 output) + gpio_set_level(9,1) before esp_lcd_new_i80_bus. Replace the LVGL 8 lv_disp_drv / on_color_trans_done glue with esp_lvgl_port 2.9 lvgl_port_add_disp (buff_dma=true, internal double buffers, flags.swap_bytes=true; never PSRAM buffers). For IDF 6.1, swap color_space for rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB and check the i80 bus alignment fields (psram/sram_trans_align were superseded by a dma_burst_size-style field). Send the CMD_1 table with esp_lcd_panel_io_tx_param after esp_lcd_panel_init, keeping the 120 ms delay after 0x11, then disp_on. Make CMD_1 and RGB/BGR runtime-selectable (I176-02, I176-03).

### 2. Deep-sleep entry sequence: panel SLPIN (0x10), GPIO15 (PWR_EN) low, backlight EN low, ext0 wake on GPIO14

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:395-403 @ 1cc41eb1c76b2f3215130a3dd64e7138ce00b482 (fixed per #289 in c9fd58a)

**Licence:** MIT (repo LICENSE, Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Port to plain ESP-IDF C: esp_lcd_panel_io_tx_param(io, 0x10, NULL, 0); wait about 5 ms; gpio_set_level(38,0); gpio_set_level(15,0) with pull-up disabled and pull-down enabled; gpio_hold_en on 15, 38, 9 (RD high), 6 (CS high) and 8 (WR high), plus gpio_deep_sleep_hold_en(). Then esp_sleep_enable_ext1_wakeup_io((1ULL<<0)|(1ULL<<14), ESP_EXT1_WAKEUP_ANY_LOW) and esp_deep_sleep_start(). On wake, release the holds, send SLPOUT and wait 120 ms, or redo the full init.

### 3. POSIX TZ string for Vietnam from LilyGO's timezone table

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/zones.h:251 @ 1cc41eb ({"Asia/Ho_Chi_Minh", "<+07>-7"})

**Licence:** MIT (LilyGO repo); the table is derived from nayarsystems/posix_tz_db (MIT)

**Port notes:** Only one constant is needed: setenv("TZ", "<+07>-7", 1); tzset(); The Arduino String-based table is not worth porting.

### 4. SNTP start-up that accepts DHCP-provided NTP servers with pool fallbacks

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:453-471 @ 1cc41eb (init_sntp_time) and examples/factory/pin_config.h:13-14

**Licence:** MIT

**Port notes:** Rewrite with esp_netif_sntp: ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(2, ESP_SNTP_SERVER_LIST("pool.ntp.org","vn.pool.ntp.org")), .server_from_dhcp = true, .renew_servers_after_new_IP = true, .index_of_first_server = 1, .sync_cb to set the 'synced' flag. Never block the 10 s glance display on the sync.

### 5. Community ESP-IDF + LVGL 9.1 reference for this board: i80 panel setup, LVGL 9 display and flush glue, espressif/button wiring for GPIO0/GPIO14

**Source:** github.com/zef/t-display-s3-espidf-lvgl src/display.cpp, src/pin_config.h, src/buttons.cpp @ af59c83520c5fd7ad78d618ff4f533f1174991ba (linked from #103, 2024-05-29)

**Licence:** MIT (Copyright (c) 2024 Zef Houssney)

**Port notes:** Reference only, since Gogadget uses esp_lvgl_port. It holds RD high (input+pull-up, line 37) and uses swap_color_bytes (line 87) and gap 0/35 (lines 104-107). Caveats: 2 MHz pixel clock; it calls lv_disp_flush_ready in the flush callback (line 26) as well as on DMA completion (line 31), which can let LVGL reuse a buffer that is still being sent; static buffers of half the screen. krupis/T-Display-S3-esp-idf (also an IDF-5 LVGL port, #156) has no licence, so do not copy from it.

## Open questions

- Does the reboot when switching between USB and battery (#174) happen on plug-in as well as unplug? Which esp_reset_reason appears, and do the RTC timer and RTC slow memory survive it? This needs a bench test with the 250 mAh protected cell, with Wi-Fi and backlight both on and off.
- Are the USB-C CC pins really unterminated on current TP4065 production boards (the schematic shows no 5.1 kΩ resistors)? That decides whether USB-C PD chargers or C-to-C cables can keep Gogadget in always-on mode. Measure CC1/CC2-to-GND on the actual unit.
- Which panel generation is the Gogadget unit: the smooth one that needs CMD_1 or the frosted one? And which RGB/BGR order does it use? No electronic ID exists (mmMicky #87), so this must be judged visually.
- What accounts for the rest of the measured 263 µA sleep current beyond R12 (~40 µA) and the regulator's quiescent current (datasheet typical ~25–55 µA, depending on whether it is an RT9013 or AP2112K)? Candidates are reverse leakage of D3 (1N5819), the ST7789's sleep-in current, and flash/PSRAM. No per-rail measurement exists in #1–#176.
- Does ESP-IDF v6.1 still support ESP32-S3 revision v0.1 (what esptool reports on 2022–23 boards in #76, #100, #103, #106, #138, #162)? Are there v0.1 errata affecting octal PSRAM or USB-Serial-JTAG?
- When Gogadget runs from a plain USB charger with no host, does the USB-Serial-JTAG console in IDF 6.1 block or slow down logging and LVGL? Arduino users saw sketches run only with a serial monitor open (#126). Verify that the driver drops output when no host is connected.
- With USB present, the backlight supply (V3V) stays live through deep sleep. How does the AW9364 behave if GPIO38 floats then? The issues never say whether its EN pin has an internal pull-down, so hold GPIO38 low until this is verified.
- What exactly does the TP4065 PROG formula give for R13 = 2 kΩ? The schematic annotation says 580 mA (a TP4056-style table) and the already-known measurement is about 525 mA. Which R13 value gives a safe 0.5–1C for a 250 mAh cell?
- Several problems were never resolved upstream: weak Wi-Fi (#67, #123; it is unknown whether later batches got the promised antenna change), red and gamma rendering (#113), the intermittent colour inversion (#118), 1-Wire failure on GPIO43/44 (#161), and one unit that stayed black only with TFT_eSPI (#151). Could any of these be signal-integrity effects of floating LCD control lines (RD, CS) that holding them in a defined state would cure?
