# LilyGO T-Display-S3 v1.2 knowledge base

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive: seven parallel source sweeps of Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5, adversarial verification of the 24 top claims, then a synthesis agent that wrote this file)

Status: synthesis; the 24 top claims behind it were adversarially verified (19 confirmed / 5 partially / 0 refuted, see lilygo-verification.md); per-line tags: [V] verified, [I] inference, [C] correction

Superseded by decision: the battery default backlight is 6/16 and the planning case is 50 glances and 4 syncs a day, about 11-13 days per charge ([power-budget.md](../hardware/power-budget.md)), not the level-4, 20-glance budget in section 3; local time comes from the fixed UTC+7 offset in `core/`, so the `<+07>-7` TZ string in section 8 is not used. The maintained hardware reference built from this file is [t-display-s3.md](../hardware/t-display-s3.md).

Compiled on 2026-10-20 from:
- T-Display-S3 (TDS3) @1cc41eb
- LilyGo-Display-IDF (LDI) @b1a1cc5
- the v1.2 schematic (SCH, TDS3 b0a5ecd, 2024-04-29)
- the ST7789V v1.4 datasheet (DS, in TDS3 `datasheet/`)
- the AW9364 V2.4 and TP4065 REV2.0 datasheets
- ESP-IDF v6.1

Tags:
- **[V]**: checked again against the primary source.
- **[I]**: an inference or estimate; confirm it on hardware.
- **[C]**: corrects an earlier claim or a vendor document.
- Untagged lines rest on the cited sweep evidence.

## 1. Board, pins, revisions

- **Pins:**
  - LCD D0–D7: 39, 40, 41, 42, 45, 46, 47, 48. WR 8, RD 9, DC 7, CS 6, RST 5.
  - Backlight EN 38, PWR_EN 15, BAT_ADC 4.
  - Buttons: 0 and 14.
  - I2C: SDA 18, SCL 17.
  - UART0: 43/44, on the 4-pin JST-SH.
  - Free on the non-touch board: 1–3, 10–13, 21. Pins 16–18 are usable but carry pull-ups.
- **The silkscreen does not identify the BOM.** Boards marked "22-8-2 V1.2" shipped in 2022, yet the charger drawn on the schematic later changed twice: TP4054 → TP4056B (2023-03) → TP4065 (2024-04) (#24, #168; SCH history 410f030, 811c949, b0a5ecd). Read the marking on the charger chip instead: "65b…" means TP4065.
- [C] The 2024 schematic also changed the GPIO0 and CHIP_PU pull-ups from 100 kΩ to 10 kΩ, although the maintainer said only the charger changed (#236).
- **V1.3** (silkscreen "25-9-12 V1.3") is sold by lilygo.cc on request; distributors may ship older stock. The only stated change is a re-matched antenna (#340). No V1.3 schematic is published. A new FCC grant (2ASYE-T-DISPLAYS3, 2026-09-24) likely covers it [I].
- **Two panel finishes,** glossy and matte, with no electronic ID. esp_lcd i80 cannot read the panel ID (#87, #122, #300).

## 2. Power tree and sleep

- **Power path [V]:**
  - USB: VBUS → D3 1N5819 → VSYS (labelled "+5V").
  - Battery: cell → R10 fuse → VBAT → Q4 SI2307 → VSYS. Q4's gate is on VBUS, so it conducts only without USB.
  - VSYS → LDO U7 (always enabled) → VDD3V3, which feeds the ESP32-S3, the flash, the panel logic (CN1 pin 2 only [C]) and the header 3V pins.
- **Switched rails [V]:**
  - Q1 is driven by a diode-OR of VBUS (D2) and GPIO15 (D1) through 10 kΩ, with 100 kΩ to GND.
  - Q1 turns on Q3 (VDD3V3 → V3V, which feeds the AW9364 VIN, the LED anodes and the green LED).
  - Q1 also turns on Q2 (VSYS → the BAT net, i.e. the ADC divider).
  - With USB present, GPIO15 changes nothing.
  - On battery, GPIO15 HIGH costs about 0.5–0.9 mA before any backlight [I].
- **GPIO15 pull-up hazard:** even a weak internal pull-up on GPIO15 turns V3V on (#51). IDF v6.1 `gpio_reset_pin()` enables the pull-up [V `gpio.h`], so never use it on GPIO15.
- **LDO:**
  - The schematic says "AP2112K-3.3V(RT9013)". The maintainer says RT9013, 500 mA (#291), and RT9080 on current boards (#348).
  - Quiescent current is 2–55 µA depending on the part [I].
  - The header 3V pin is limited to 100 mA (README).
- **Fixed drains [V]:** R12 (100 kΩ across VBAT, about 40 µA) plus the LDO quiescent current add up to about 95 µA with an AP2112K. Firmware cannot remove them.
- **Deep-sleep current:**
  - Measured: 263 µA (#289), 243 µA (`examples/Sleep/IMAGE.jpg`), about 200 µA (#12).
  - The panel draws 6.0 mA in normal mode and 15 + 5 µA in sleep-in (DS Table 3) [V], so SLPIN is mandatory.
  - About 120–190 µA is unexplained. The main suspect is floating LCD inputs on the always-powered panel [I].
- **Pin holds [V `gpio.h`]:** RTC pads (GPIO0–21) keep their state with `gpio_hold_en`. Digital pads (GPIO38–48) also need `gpio_deep_sleep_hold_en`.
- **LilyGO's sleep code** (factory c9fd58a, Sleep fc466c6) only sends SLPIN, drives GPIO15 and the backlight LOW, and arms ext0 on GPIO14. It has no 5 ms wait, no pin holds and no Wi-Fi stop [V].
- **#300 ghosting:** matte panels showed white borders and ghost images after minutes of deep sleep when SLPIN was never sent. The user fixed it by driving every LCD pin LOW. The maintainer's advice: draw the frame first and turn the backlight on last.
- **Pull-ups and strapping pins:** GPIO16/17/18 have 100k/10k/10k pull-ups to VDD3V3, so driving one LOW in sleep costs 33–330 µA (#313). GPIO45/46 (LCD D4/D5) are strapping pins: never hold them HIGH.
- **USB unplug:** unplugging USB may reboot the board ("it will restart", #174). Likely cause: Q4's body diode feeds VSYS until VBUS decays, and the LDO drops out [I].
- [C] **The "5V" header pin is VSYS, not VBUS** (#270 and the schematic, against the README). Feeding 5 V there with a cell fitted charges the cell through Q4, bypassing the charger. Never put more than 4.2 V on the battery connector (#343).

## 3. Battery and charging

- [V] **Charge current:** TP4065 with R13 = 2 kΩ; RPROG = 1050/IBAT gives about 525 mA (spec 425–575 mA at 2.1 kΩ). The chip folds back thermally above about 0.4 A, terminates at C/10 and charges to 4.2 V ±1%. That is about 2.1C for a 250 mAh cell.
  - [C] The schematic note "580 mA" is a TP4056 table value; the README says 500 mA.
- **R10** (the "5V/2A" fuse) degrades in the field: reports of 45 Ω and 350 Ω, with boot loops on battery (#223, #291). The maintainer says it is safe to bridge it when using a protected cell.
- **No charge-status GPIO.** CHRG drives only the red LED: on = charging, off = full, flashing = no battery. The green LED sits on V3V (lit with USB or GPIO15 HIGH) and draws 0.25–0.6 mA.
- [C] **Connector:** 1.25 mm 2-pin, MX1.25/PicoBlade style, not JST-GH as the product page says (#287).
  - Reverse polarity "will not cause any damage" (#317).
  - Cell size: the README says ≤7×18×50 mm; the official shell takes ≤47×25×7 mm (#324).
- **GPIO4 (ADC1_CH3) [V]:** a 100k/100k 1% divider plus 10 nF (τ 0.5 ms) on the BAT net. That net is VSYS switched by Q2, not the cell.
  - Battery, GPIO15 HIGH: (VBAT − I·(R10 + Q4))/2.
  - Battery, GPIO15 LOW: 0 V.
  - USB: about 2.3 V whatever GPIO15 does. So "GPIO15 LOW and GPIO4 above about 1 V" is a clean USB test [V SCH, #284].
  - No VBUS GPIO exists (#292). 2.3 V is below the S3 input-high level, so a level wake on GPIO4 is unreliable [I].
  - LilyGO's code reads once through the legacy `esp_adc_cal` (removed in IDF 6.0 [V]). Its "No battery connect!" message really means USB is present (#190).
- **On USB** the load runs from VBUS (Q4 is off) and the TP4065 recharges the cell once it falls to about 80–90%, so always-on mode needs no charge control.
- **Currents:**
  - 147.7 mA awake (#289); about 80 mA of that is likely the backlight at full [I].
  - Wi-Fi averages about 150 mA or more (#295).
  - ESP32 peaks reach about 700 mA, against a 500 mA LDO (#291).
- **Budget [I]:**
  - Sleep: about 6 mAh/day.
  - One 10 s glance: about 0.17 mAh at backlight level 4, about 0.37 mAh at level 16.
  - One NTP sync: about 0.2–0.4 mAh.
  - With 20 glances and one sync a day, 250 mAh lasts about 2.5–3.5 weeks.

## 4. Display panel and init

- **Panel:** ST7789V, 170×320 IPS (module NFP190B), 8-bit 8080 bus.
  - There is no TE line (FPC pin 19 is unconnected), so no vsync.
  - A full frame at 10 MHz takes 10.9 ms, shorter than one ~18.9 ms panel scan, so tearing is at most one line [I].
- **Pixel clock [V]:** the datasheet's write cycle TWC ≥66 ns gives a limit of about 15.15 MHz.
  - 10 MHz (80 MHz/8, exact) is within spec.
  - LilyGO's 16 MHz (factory), 20 MHz (2022) and 40 MHz (nes) exceed it.
  - lv_demos asks for 6.528 MHz but actually runs at 6.67 MHz.
  - Espressif's IDF 6.1 i80 example drops to 2 MHz when the buffers are in PSRAM.
  - The factory's "mosaic" comment (adc038e) is unexplained.
- **CMD_1 [V]:** 14 entries, byte-identical in all 8 copies (`factory.ino:133-149`, LDI `display_s3.c:32-47`).
  - Decoded: SLPOUT + 120 ms, COLMOD 05, porch 0B/0B, VGH 14.97 V, VCOM 1.10 V, LCMCTRL 2C and C2 01 (both power-on defaults), GVDD 5.10 V, about 53 Hz frame rate, AVCL −4.8 V, vendor gamma.
  - [C] D0 A7 is off-spec (the first parameter is fixed at A4h) and D6 A1 is undocumented.
  - COLMOD 05 and 55 differ only in the RGB-interface field, which the 8080 bus does not use [I].
  - The table contains no MADCTL, INVON, RAMCTRL, CASET or DISPON.
  - The maintainer says pixels blur without it (#229). In 2023 Micky said frosted panels don't need it (#87).
- **IDF ST7789 driver [V]:**
  - Since IDF 5.0 its init sends no DISPON. That was the real cause of the factory fix in c9b939f.
  - The v6.1 init sends SLPOUT, waits 100 ms, then MADCTL, COLMOD 55 and RAMCTRL 00 F0 (F8 for little-endian).
  - `disp_sleep()` sends SLPIN or SLPOUT, then waits 100 ms.
  - `set_brightness()` writes WRDISBV, which has no effect on this board.
- **Reset timing [V DS Table 9 note 7]:** commands may follow RESX release after ≥5 ms, but SLPOUT needs ≥120 ms.
  - IDF sends SLPOUT 10 ms after release, so CMD_1's own SLPOUT + 120 ms is the real safety margin. [C] Dropping it is not a free 120 ms saving.
  - SLPOUT does nothing when the panel is already awake (p.184).
  - Wait 5 ms after SLPIN, and send SLPIN no sooner than 120 ms after SLPOUT.
- [V] **GRAM** is random at power-on but survives a hardware or software reset and sleep-in, so after a wake the stale frame is still there. After a cold power-off the panel shows noise (#232). Turn the backlight on only after a new frame is written; LDI turns it fully on before LVGL has drawn anything.
- **Orientation [V]:** inversion on. `swap_xy(true)` + `mirror(false,true)` puts USB on the left; `mirror(true,false)` puts it on the right. The gap is (0,35) in both. Colour order is RGB with CMD_1 (LCMCTRL 2Ch). TFT_eSPI's historic BGR setting likely came from its LCMCTRL 0Ch [I].
- **RD (GPIO9) must be driven HIGH.** esp_lcd i80 never touches it; left floating it gives a black or garbage screen (#103).
- **Byte order:** LilyGO swaps RGB565 bytes in software (LVGL 8 `LV_COLOR_16_SWAP`). In IDF v6.1, `flags.swap_color_bytes` swizzles colour data only; command parameters are untouched because param_bits equals the bus width [V `esp_lcd_panel_io_i80.c:519,886`]. RAMCTRL little-endian mode is untested on this panel.
- **Colour:**
  - Pure red (0xF800) shows as orange, and greys turn green off-axis. The vendor gamma cannot be tuned (#113).
  - One intermittent colour inversion remains unresolved (#118).
  - The maintainer blamed one half-screen fault on OLED-like burn-in (#128).
- **Fonts:** LilyGO's Montserrat sets are ASCII-only. `font_Alibaba.c` (the factory's 120 px digits) has no stated licence.

## 5. Backlight (AW9364)

- **Wiring [V]:** EN = GPIO38 (direct), VIN = V3V, four LED strings.
- **Datasheet [V]:**
  - EN is active high with an internal 150 kΩ pull-down, so a floating GPIO38 means off.
  - Start-up takes ≥20 µs. Pulses: high time THI ≥0.5 µs, low time TLO 0.5–500 µs. EN LOW for more than 2.5 ms shuts the chip down.
  - Enabling starts at 20 mA per channel. Each rising edge lowers it by 1.25 mA, wrapping from level 1 back to 16.
  - That is about 5 mA per level and about 80 mA at full (16.5–23.5 mA per channel). Quiescent current is 330 µA while on.
- **LilyGO reference** `factory.ino:223-248` (2d3c294; #278 called LEDC "wrong"):
  - Level 0 = EN LOW, then 3 ms.
  - From off: EN HIGH, then 30 µs, gives level 16.
  - Pulses needed = (16 + current − target) % 16.
  - The code sets no explicit pulse width.
- **PWM is meaningless** here: every low phase of 0.5–500 µs counts as a step [I]. [C] Commit 02f0e9d missed eight 2026 LovyanGFX examples (#328–#336) that still configure `Light_PWM` at 22 kHz.
- On USB, V3V is forced on; only EN can turn the backlight off.

## 6. Buttons

- **GPIO0** (BOOT, a strapping pin): 10 kΩ pull-up (100 kΩ in 2023), no capacitor.
- **GPIO14:** 100 kΩ pull-up + 100 nF (τ ≈ 10 ms, so it releases slowly).
- Both pull up to the always-on VDD3V3, so ext1 wake works with RTC_PERIPH off and no internal pulls [V `esp_sleep.h`]. A held button costs about 330 µA (GPIO0) or 33 µA (GPIO14).
- LilyGO only wakes via ext0 on GPIO14; waking on GPIO0 or through ext1 is untested. The factory uses OneButton 2.0.4 (50/400/800 ms).
- **Positions,** derived from the pinmap drawing [I]:
  - USB on the left: GPIO0 upper-left, GPIO14 lower-left.
  - USB on the right: GPIO14 upper-right, GPIO0 lower-right.
- Libraries can grab GPIO0 by default (I2S MCLK, #101).

## 7. USB, console, flashing

- **Ports:** USB-C goes to native USB (GPIO19/20, 303A:1001). UART0 is on the JST-SH (G, 3V, 43, 44) with no pull-ups; it works as a second log port (#108).
- **USB-C CC pins:** no schematic revision shows the 5.1 kΩ CC resistors, so C-to-C chargers may not supply VBUS (#65, #166, #233).
  - This is unconfirmed on hardware; the #230 PCB photo shows parts R01/R04 that are not in the schematic.
  - LilyGO ships an A-to-C cable.
- **Console:**
  - IDF's default console is UART0; select USB-Serial-JTAG instead (#266).
  - IDF v6.1 [V `usb_serial_jtag_vfs.c`]: with no host connected, writes return EIO at once. With a host connected but not reading, a write stalls once for up to 50 ms, then output is dropped.
  - USB-Serial-JTAG does not work in or after light sleep. `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` blocks automatic light sleep while a host is connected.
- **Recovery and debug:**
  - The USB device disappears in deep sleep. To recover, hold BOOT, tap RST, release BOOT. After flashing in this manual mode, press RST (#162, #16).
  - Terminals may reset the board via DTR/RTS (#274).
  - On-chip USB-JTAG debugging works (`esp32s3-builtin.cfg`, #210).
  - TinyUSB builds (usb_hid_pad) lose the USB-Serial-JTAG console and auto-download.
- **Factory images** in `firmware/` are DIO builds with the 8 MB layout and keep Wi-Fi config in NVS at 0x9000, so erase the flash once. Keep the no-touch image as a hardware sanity check. The ScreenDetection image only prints compile-time pin settings.

## 8. Wi-Fi, BLE, antenna

- **Antenna:** the v1.2 antenna is weak (one report covers about 100 boards, with ~200 ms pings; #67, #123, #340); v1.3 is re-matched. An external antenna needs a 0201 resistor rotated to the U.FL path (README, #39); results were mixed.
- **Radio:** 2.4 GHz only; BLE only (#200). IDF 6.0 moved `wifi_provisioning` to `espressif/network_provisioning` [V].
- **Factory NTP:**
  - The geo-IP timezone lookup broke when its pinned certificate expired on 2025-03-29 (#297).
  - A DHCP-supplied NTP server is suspected in one sync failure (#297, unconfirmed).
  - The Vietnam timezone string is `<+07>-7` (`zones.h:251`).
  - Setting WPA2 as the minimum security fixed one association failure (#247).

## 9. Memory, boot, timekeeping

- **Chip and flash:** ESP32-S3R8 (8 MB octal PSRAM) and a W25Q128JV 16 MB flash at QIO 80 MHz (board JSON). A quad PSRAM config boot-loops (#103). Boards from 2022–23 report chip revision v0.1.
- The factory example fits a 108.8 KB full-frame internal DMA buffer alongside Wi-Fi. LDI's sdkconfig is a copied template, not tuned settings. LilyGO publishes no IDF partition table.
- **Boot timing:**
  - Issue #4 (sibling board, IDF 5.3, DIO flash): app load about 180 ms; PSRAM memtest about 735 ms.
  - Expected Gogadget wake-to-first-frame with a full panel init: about 0.4–0.5 s, of which about 0.24 s is fixed panel delay.
  - A reset-free wake could save about 0.23 s; untested [I].
- **Timekeeping:**
  - No 32 kHz crystal is possible (its pins are GPIO15/16), so deep-sleep time runs on the 136 kHz RC (the default) or on 17.5 MHz/256.
  - The 17.5 MHz/256 source is more stable for about +5 µA [V IDF docs, Kconfig.rtc]. `RTC_CLK_CAL_CYCLES` defaults to 1024.
  - The 10 ppm crystal (#262) only applies while awake. RC drift on this board has not been measured.

## 10. Mechanical

- **Models:** the PCB STEP is 61.98×26.09×7.86 mm and the non-touch model 61.98×26.31×9.88 mm. Older models are in `899de7a^:3D_file/`. Mounting holes exist only in `dimensions/PCB.dwg`.
- **Handling:**
  - The glass and flex cable crack easily (#51, #128), and only tape holds the panel (#176).
  - The shipping foam is conductive (#65).
  - Keep the antenna end (opposite the USB port) clear of the battery and metal.

## 11. Errata and open items

- **Example bug:** `j < len & 0x7f` makes nes, usb_hid_pad, touch_test and GetBatteryVoltage send 128 bytes after SLPOUT.
- **LDI issues** (its S3 path is unchanged since 2024-01-31):
  - PSRAM draw buffers, an idle-priority LVGL task, `-Og` and a 100 Hz tick (#8 "very slow", unanswered).
  - Touch init runs on non-touch boards.
  - Errors are not checked.
  - It does not build on IDF 6.x.
- **Arduino examples** target core 2.0.14 (CI uses 2.0.17); only the factory builds on core 3.3.11 (c9b939f). The factory itself:
  - runs at 16 MHz, out of spec;
  - uses fields removed in IDF 6.1;
  - calls the blocking `getLocalTime`;
  - boots slowly (GIF, tone, 30 s + 120 s Wi-Fi waits).
- A stale bot closes issues after 30 + 14 days, so "closed" does not mean fixed.
- **Open questions:**
  - Panel: CMD_1 on matte panels; the cause of #300; RAMCTRL little-endian.
  - Sleep and wake: the 120–190 µA sleep remainder; reset-free wake; GPIO0 wake.
  - Power events: the reset reason on unplug and whether the RTC survives it; the AW9364 level after V3V loss.
  - Hardware: the v1.3 BOM; CC resistors; what R12 is for.
  - Timekeeping: RC drift.
