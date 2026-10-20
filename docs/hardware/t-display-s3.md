# T-Display-S3 v1.2 (non-touch): hardware reference

This is the board Gogadget runs on: LilyGO T-Display-S3 v1.2 without touch (ESP32-S3R8, 16 MB flash, ST7789V 170x320 panel on an 8-bit i80 bus). This file records what the hardware does and the evidence for it. It does not make design decisions; those live in `docs/adr/`.

- Energy model, deep-sleep sequence, USB plug/unplug, power checks without a meter: [power-budget.md](power-budget.md).
- Acceptance tests, first flash, per-unit log: [bring-up.md](bring-up.md).

## Conventions

- **Tags.** [V] verified against the primary source (schematic, datasheet, ESP-IDF v6.1 source or docs). [I] inference or estimate: confirm on hardware. [C] corrects an earlier claim or a vendor document. Untagged lines rest on the cited evidence.
- **Evidence.**
  - `T-Display-S3:<path>:<line>` means Xinyuan-LilyGO/T-Display-S3 at 1cc41eb.
  - `LilyGo-Display-IDF:<path>:<line>` means Xinyuan-LilyGO/LilyGo-Display-IDF at b1a1cc5.
  - A bare `#NNN` is an issue or PR in Xinyuan-LilyGO/T-Display-S3. Other repositories are named, for example `esp-idf#15921`.
  - "SCH" is the v1.2 schematic dated 2024-04-29 (T-Display-S3:schematic/T_Display_S3.pdf, last changed in b0a5ecd; archived in [docs/research/upstream/](../research/upstream/T_Display_S3-schematic-b0a5ecd.pdf)). "DS" is the Sitronix ST7789V datasheet v1.4. Other datasheets are listed in [Datasheets and references](#datasheets-and-references).
- **Stable anchors.** Code comments cite sections as `docs/hardware/t-display-s3.md#<anchor>`, for example `#backlight-aw9364`. Rename a heading only together with a search of the codebase for its anchor.

| Anchor | Topic |
|---|---|
| `#board-identification-and-revisions` | revisions, BOM variants, v1.3 |
| `#pin-map`, `#free-and-spare-pins`, `#connectors` | every GPIO, headers, FPC |
| `#power-tree`, `#switched-rails`, `#ldo-and-supply-limits` | rails, GPIO15, VBUS diode-OR |
| `#sleep-current` | deep-sleep current composition |
| `#battery-and-charging` | TP4065, R13, R10, LEDs, connector |
| `#gpio4-sensing` | VSYS and USB detection on GPIO4 |
| `#display-panel-st7789v` and its subsections | panel facts, init table, timing, orientation, quirks |
| `#backlight-aw9364` | pulse-count dimming, timing, current per level |
| `#buttons` | RC values, wake, strapping caveat |
| `#usb-console-and-flashing` | console, recovery, CC pins |
| `#wi-fi-ble-and-antenna` | radio, antenna, v1.3 |
| `#memory-flash-and-psram` | chip, flash, PSRAM, boot timing |
| `#timekeeping` | RTC clock sources, drift |
| `#mechanical`, `#known-errata`, `#open-questions` | |

## Board identification and revisions

- **Board.** Non-touch SKUs are H569 (pins not soldered) and H577 (soldered). Touch and non-touch boards share one schematic; on touch boards the touch parts use GPIO16/17/18/21 (#197, #346).
- **The silkscreen does not identify the BOM.** Boards marked "22-8-2 V1.2" shipped from 2022, while the charger on the published schematic changed twice: TP4054 (2022-10) -> TP4056B (2023-03) -> TP4065 (2024-04) (#24, #168; SCH history 410f030, 811c949, b0a5ecd).
  - Identify the charger by its marking: the TP4065 is a SOT-23-5 marked "65bYWW" [V TP4065 DS]. The TP4056 is an SOP-8.
  - [C] The 2024 sheet also changed the GPIO0 and CHIP_PU pull-ups from 100 kΩ to 10 kΩ, although the maintainer said only the charger changed (#236).
- **LDO.** SCH says "AP2112K-3.3V(RT9013)". The maintainer says RT9013 (#291), and RT9080 on current boards (#348). The fitted part sets part of the sleep floor; see [Sleep current](#sleep-current).
- **Panel.** Two finishes, glossy and matte, with no electronic ID (#87, #122, #300). See [Vendor init table](#vendor-init-table).
- **v1.3** (silkscreen "25-9-12 V1.3"). The only stated change is a re-matched onboard antenna (#340). lilygo.cc ships it on request; distributors may ship older stock. No v1.3 schematic is published.
  - A new FCC grant, 2ASYE-T-DISPLAYS3 dated 2026-09-24, likely covers it [I]. Older boards carry the label 2ASYE-T-DISPLAY-S3 (granted 2022-12-12).
- **Chip revision.** Boards from 2022-23 report ESP32-S3 v0.1 (#76, #100, #103, #106, #138, #162). IDF v6.1 boots it: `CONFIG_ESP32S3_REV_MIN` defaults to v0.0 [V esp-idf v6.1 components/esp_hw_support/port/esp32s3/Kconfig.hw_support].
- No GPIO number changed in any LilyGO `pin_config.h` in four years of history.
- Record each unit with the template in [bring-up.md](bring-up.md#per-unit-log).

## Pin map

Every GPIO the board connects.

- **Pad type.** RTC pads (GPIO0-21) can be ext1 wake sources and keep a `gpio_hold_en()` level through deep sleep. Digital pads (GPIO26-48; the S3 has no GPIO22-25) hold through deep sleep only with `gpio_hold_en()` plus `gpio_deep_sleep_hold_en()`; `gpio_hold_en()` alone does not hold them [V esp-idf v6.1 gpio.h, esp_sleep.h].
- **Pulls.** Resistors listed are on the board. "Internal" means the ESP32-S3 reset default [V ESP32-S3 datasheet v2.2, Table 2-1].

| GPIO | Pad | Board net and parts [V SCH] | Notes | Gogadget use |
|---|---|---|---|---|
| 0 | RTC | BOOT button S2 to GND; R1 10 kΩ pull-up to VDD3V3; no capacitor | Strapping: LOW at a chip reset selects download mode. Internal pull-up at reset | Enter (press only); ext1 wake |
| 1, 2, 3 | RTC | Header P2 | GPIO3 is a strapping pin (JTAG source; ignored with default eFuses) | free |
| 4 | RTC | BAT_ADC (ADC1_CH3): midpoint of R2/R4 100 kΩ/100 kΩ 1%, C13 10 nF to GND. Divider top is the switched BAT net | USB level (~2.3 V) is below VIH, so no reliable level wake [I] | VSYS and USB sensing ([GPIO4 sensing](#gpio4-sensing)) |
| 5 | RTC | LCD RES (CN1-5) | 60 µs low glitch at power-up resets the panel | LCD reset |
| 6 | RTC | LCD CS (CN1-6) | | LCD chip select |
| 7 | RTC | LCD D/C (CN1-7) | | LCD data/command |
| 8 | RTC | LCD WR (CN1-8) | | i80 write strobe (pixel clock) |
| 9 | RTC | LCD RD (CN1-9), no pull | esp_lcd never drives it; floating RD gives a black or garbage screen (#103) | driven HIGH at all times |
| 10-13 | RTC | Header P2 (the SD shield uses 11 CLK, 12 D0, 13 CMD) | | free |
| 14 | RTC | KEY button S3 to GND; R3 100 kΩ pull-up to VDD3V3; C14 100 nF to GND | τ ≈ 10 ms: slow release edge | Next (press), Back (hold); ext1 wake |
| 15 | RTC | PWR_EN -> D1 1N4148 -> R5 10 kΩ -> Q1 base (R7 100 kΩ to GND). Pad XTAL_32K_P | Any pull-up on this pin turns V3V on (#51). `gpio_reset_pin()` enables the pull-up [V gpio.h] | V3V and divider enable ([Switched rails](#switched-rails)) |
| 16 | RTC | TP_INT (CN1-29), header P1; RINT 100 kΩ pull-up. Pad XTAL_32K_N | Pull-up fitted on non-touch boards too (#313) | unused; never drive LOW in sleep |
| 17 | RTC | I2C SCL (CN1-27), header P1; RIIC 10 kΩ pull-up | Default drive 10 mA | reserved for a future I2C RTC |
| 18 | RTC | I2C SDA (CN1-28), header P1; RIID 10 kΩ pull-up | | reserved for a future I2C RTC |
| 19, 20 | RTC | USB D-, D+ to USB-C | USB-Serial-JTAG (303A:1001) | console, flashing |
| 21 | RTC | TP_RESET (CN1-26), header P1; no pull | Assumed unused by the non-touch panel [I] (#346) | free (best spare) |
| 26-32 | - | SPI flash (W25Q128JV) on VDD_SPI | not exposed | - |
| 33-37 | - | Octal PSRAM inside the ESP32-S3R8 | not exposed | - |
| 38 | digital | LCD_BLK -> AW9364 EN, no series or pull resistor | AW9364 EN has an internal 150 kΩ pull-down [V AW9364 DS] | backlight pulse driver |
| 39-42 | digital | LCD D0-D3 (CN1-11..14) | These are the pad-JTAG pins (MTCK/MTDO/MTDI/MTMS), so external JTAG is impossible | LCD data |
| 43, 44 | digital | U0TXD, U0RXD: 4-pin JST-SH and header P1; no pull on the board | ROM boot messages appear on GPIO43 | spare debug UART |
| 45 | digital | LCD D4 (CN1-15) | Strapping: HIGH at a chip reset selects a 1.8 V VDD_SPI; the flash needs 3.3 V [V ESP32-S3 DS Table 3-4]. Internal pull-down | LCD data; never park HIGH |
| 46 | digital | LCD D5 (CN1-16) | Strapping: boot mode (with GPIO0) and ROM printing. Internal pull-down | LCD data; never park HIGH |
| 47, 48 | digital | LCD D6, D7 (CN1-17, 18) | SPICLK_P/N pads | LCD data |

Sources: SCH SoC block; T-Display-S3:examples/factory/pin_config.h:33-65; LilyGo-Display-IDF:main/product_pins.h:202-231; ESP32-S3 datasheet v2.2, Tables 2-1, 2-2, 3-1, 3-4.

### Free and spare pins

- Free on the non-touch board: GPIO1-3, 10-13, 21.
- GPIO21 is the best spare: no pull, no analog function, no power-up glitch [V ESP32-S3 DS Tables 2-1, 2-2]. It also reaches CN1-26, assumed unconnected inside the non-touch panel [I].
- GPIO16, 17 and 18 carry pull-ups to VDD3V3 (100 kΩ, 10 kΩ, 10 kΩ). Driving one LOW costs about 33, 330 and 330 µA (#313).
  - Keep 17/18 for I2C; the pull-ups are already fitted. They are the planned bus for the RV-3032-C7 RTC upgrade (ADR-0011).
- GPIO43/44 (UART0) work as a second log port (#108) or as remapped I2C. The ROM prints on GPIO43 at every reset, and a DS18B20 1-Wire sensor failed there (#161).
- GPIO1-19 show a 60 µs low-level glitch at power-up (GPIO20 a pull-down glitch), and GPIO18-20 also show high-level glitches [V ESP32-S3 DS Table 2-2]. GPIO0 and GPIO21 show none.
- No peripheral may default a signal onto GPIO0 or GPIO14: an audio library once put I2S MCLK on GPIO0 (#101). Set unused signals to -1.

### Connectors

Headers, seen from the back (component side, as in T-Display-S3:image/T-DISPLAY-S3.jpg), antenna end first [V SCH]:

```
  P2 (left)          P1 (right)
  3V3                GND
  GPIO1              GND
  GPIO2              GPIO43 (U0TXD)
  GPIO3              GPIO44 (U0RXD)
  GPIO10             GPIO18 (SDA, 10k pull-up)
  GPIO11             GPIO17 (SCL, 10k pull-up)
  GPIO12             GPIO21
  GPIO13             GPIO16 (100k pull-up)
  NC                 NC
  NC                 GND
  GND                GND
  5V (= VSYS)        3V3
         USB-C   JST-SH 4-pin       <- USB end
```

- **3V3 pins** are the always-on LDO output; GPIO15 does not switch them (#352). Load limit is 100 mA (T-Display-S3:README.md:274).
- **"5V" pin** [C] is VSYS (the "+5V" net), not VBUS (#270, SCH).
  - It is about 4.6 V on USB and about the cell voltage on battery.
  - Feeding 5 V into it with a cell fitted charges the cell through Q4, bypassing the charger. Never do it.
- **4-pin JST-SH 1.0 mm** (CNC, WAFER-100W-4A), from pin 1: GND, 3V3, GPIO43 (U0TXD), GPIO44 (U0RXD).
  - [C] It is sold as "STEMMA QT / Qwiic", but it carries UART0, not I2C 17/18.
- **P3 battery connector**: 2-pin, 1.25 mm pitch, MX1.25/PicoBlade style.
  - [C] It is not JST-GH, as the product page says (#287).
  - +/- are marked on the PCB (#317). It feeds VBAT through the fuse R10.
- **USB-C (U5)**: VBUS, D+ (GPIO20), D- (GPIO19).
  - CC (A5) and VCONN/CC2 (B5) are unconnected in every schematic revision [V SCH]. See [USB, console and flashing](#usb-console-and-flashing).
- **LCD FPC CN1** (30-pin, 0.5 mm, KH-FG0.5-H2.0-30PIN) [V SCH]:

| CN1 | Net | CN1 | Net |
|---|---|---|---|
| 2 | VDD3V3, panel logic supply (C17 1 µF, C18 100 nF) | 19 | not connected (no TE line) |
| 5 | RES (GPIO5) | 20 | V3V, LED anodes (C20 1 µF) |
| 6 | CS (GPIO6) | 21-24 | LEDK1-4 -> AW9364 LED1-4 |
| 7 | D/C (GPIO7) | 26 | TP_RESET (GPIO21) |
| 8 | WR (GPIO8) | 27 | SCL (GPIO17, 10 kΩ pull-up) |
| 9 | RD (GPIO9) | 28 | SDA (GPIO18, 10 kΩ pull-up) |
| 11-18 | D0-D7 (GPIO39-42, 45-48) | 29 | TP_INT (GPIO16, 100 kΩ pull-up) |
| 1, 3, 4, 10, 25, 30 | GND (one bus) | | |

## Power tree

[V SCH] Power and Charger blocks, confirmed by verification against the 2023 and 2024 sheets:

```
 VBUS (USB-C) --+------------- D3 1N5819 ->|-------------+--> VSYS (net "+5V", header "5V")
                |                                        |
                +--> TP4065 VCC (PROG: R13 2k)           |
                |       BAT ---+--> VBAT --> Q4 SI2307 --+    Q4 gate = VBUS (R14 100k to GND):
                |              |     |                        conducts only when USB is absent
                |              |     +--> R12 100k --> GND    always, about 40 uA
                |              +--> R10 "5V/2A" fuse --> P3 battery (2-pin 1.25 mm)
                |
                +--> D2 1N4148 --+
 GPIO15 ----------> D1 1N4148 --+--> R5 10k --> Q1 PMBT3904 base (R7 100k to GND)
                                    Q1 on: pulls the shared Q2/Q3 gate low (R6 1k; R8 100k to VSYS)

 VSYS ----> U7 LDO 3.3 V (EN tied to VSYS by R11 100k) --> VDD3V3, always on
 VSYS ----> Q2 SI2301, on when Q1 is on --> BAT net --> R2/R4 100k/100k, C13 10 nF --> GPIO4
 VDD3V3 --> Q3 SI2301, on when Q1 is on --> V3V --> AW9364 VIN, LED anodes (CN1-20), green LED (2k)
```

- **USB path.** VBUS -> D3 -> VSYS. About 4.6 V on USB; one user measured 4.62 V (#295).
- **Battery path.** Cell -> R10 -> VBAT -> Q4 -> VSYS. With USB present, Q4 is off, the load runs from VBUS, and the TP4065 only charges the cell.
- **VDD3V3** (always on) feeds the ESP32-S3, the flash (through the chip's VDD_SPI), the panel logic (CN1-2 only [C]), the header 3V3 pins, and every board pull-up (GPIO0, GPIO14, CHIP_PU, GPIO16-18).
- **V3V** (switched) feeds only the AW9364 VIN, the LED anodes and the green LED.
  - The panel itself is not on V3V. It stays powered in deep sleep, so it must be put to sleep with SLPIN (#352, SCH).
- **BAT net** (switched) feeds only the GPIO4 divider. The divider measures VSYS, not the cell.

### Switched rails

Q1 is driven by a diode-OR of VBUS (D2) and GPIO15 (D1) [V SCH]:

| Power source | GPIO15 | V3V (backlight supply, green LED) | BAT net / GPIO4 |
|---|---|---|---|
| USB | any | on (via D2) | on: GPIO4 ≈ VSYS/2 ≈ 2.3 V |
| Battery | HIGH | on | on: GPIO4 ≈ VSYS/2 |
| Battery | LOW, or floating with pulls off | off (R7 keeps Q1 off) | off: GPIO4 ≈ 0 V (#284) |
| Battery | internal pull-up enabled | on (#51) | on |

- On USB, GPIO15 changes nothing. The green LED stays lit, and only AW9364 EN can turn the backlight off (T-Display-S3:README.md:253-256).
- On battery, GPIO15 HIGH costs about 0.5-0.9 mA before any backlight [I], from schematic values:
  - Q1 base about 0.18 mA;
  - R8 about 40 µA;
  - green LED 0.25-0.6 mA, depending on its unknown Vf;
  - divider about 21 µA.
- Never call `gpio_reset_pin(GPIO_NUM_15)`. IDF v6.1 documents that it enables the pull-up [V gpio.h], which turns V3V on (#51).
- Never drive GPIO38 (AW9364 EN) HIGH while V3V is off: it would back-feed an unpowered chip [I]. Raise GPIO15 first.
- [C] The README calls GPIO15 "LCD power control" (T-Display-S3:README.md:149). It gates only V3V and the divider.

### LDO and supply limits

- **U7** is marked "AP2112K-3.3V(RT9013)"; batches vary (#291, #348).
  - AP2112K-3.3 [V DS39724]: 600 mA; quiescent 55 µA typical, 80 µA max; dropout 125/200 mV (typical/max) at 300 mA and 250/400 mV at 600 mA.
  - RT9013 is about 25 µA and RT9080 about 2 µA quiescent [I]; these are vendor typicals, not checked here.
- **Peak current.** The maintainer puts ESP32 peaks at about 700 mA against a 500 mA RT9013 (#291), and replacement advice is any LDO above 700 mA (#343). Wi-Fi TX peaks are 283-340 mA [V ESP32-S3 DS Table 5-7]; the average with Wi-Fi on is about 150 mA or more (#295). Reboots at Wi-Fi connect were blamed on the board supply (#275).
- **On battery,** VSYS = Vcell - I x (R10 + Q4 + the cell's protection FETs). Below about 3.35 V VSYS at glance load the LDO is in dropout, and VDD3V3 follows VSYS [I].
- **Brownout.** IDF v6.1's ESP32-S3 brownout detector is on by default at level 7, about 2.44 V (Espressif calls the levels estimates) [V Kconfig.power]. Without the hidden interrupt option, the hardware issues a System Reset (`BROWNOUT_RESET_LEVEL_SYSTEM`) [V esp-idf v6.1 components/esp_hal_pmu/brownout_hal.c].
- **Header 3V3** load: at most 100 mA (README).

## Sleep current

Battery-side current in deep sleep, with the panel in SLPIN, GPIO15 LOW and the backlight off.

| Component | Current | Basis |
|---|---|---|
| R12, 100 kΩ across VBAT | 37-42 µA at 3.7-4.2 V | [V SCH]; firmware cannot remove it |
| U7 LDO quiescent | AP2112K 55 µA typ (80 max); RT9013 ~25 µA; RT9080 ~2 µA | [V] AP2112 DS; [I] for the Richtek parts and for which part is fitted |
| ST7789V in Sleep In | IDD 15 µA + IDDI 5 µA typ (30 + 10 max) | [V DS Table 3]. Typicals are at VDD 2.75 V; at 3.3 V expect closer to max [I] |
| ESP32-S3 deep sleep | 7 µA (RTC peripherals off), 8 µA (on) | [V ESP32-S3 DS Table 5-10]; "might be higher" on PSRAM variants such as the S3R8 |
| TP4065 BAT drain, no VBUS | 0-1 µA | [V TP4065 DS] |
| AW9364 | 0: V3V is off on battery (shutdown 0.1 µA typ when powered) | [V AW9364 DS] |
| Flash, PSRAM | ~0: VDD_SPI is powered down in deep sleep by default | [I] |
| D3 1N5819 reverse leakage, VSYS -> VBUS -> R14 | small at 25 °C; rises steeply with temperature | [I] |
| **Sum** | **~70-145 µA (about 120 µA with an AP2112K)** | [I] |

- **Measured.** 243 µA (LilyGO bench photo, T-Display-S3:examples/Sleep/IMAGE.jpg), 263 µA (#289), "about 200uA" (#12).
  - [I] #289 used ext0, which keeps the RTC peripherals powered, and held no LCD pin. Its board revision and supply path are not stated.
- **Unexplained.** About 120-190 µA. The main suspect is floating LCD inputs on the always-powered panel [I]. Pin parking ([power-budget.md](power-budget.md#pin-parking)) is meant to recover part of it.
- **Faults that add current:**

| Fault | Extra current | Basis |
|---|---|---|
| Panel left in Sleep Out (no SLPIN) | ~6 mA (6.42 mA total in #289) | [V DS Table 3: 6.0 mA typ, 7.5 max] |
| GPIO15 HIGH, or its pull-up enabled | 0.5-0.9 mA | [I] schematic values |
| BOOT (GPIO0) held | ~330 µA | 3.3 V / 10 kΩ |
| KEY (GPIO14) held | ~33 µA | 3.3 V / 100 kΩ |
| GPIO17 or GPIO18 driven LOW | ~330 µA each | #313 |
| GPIO16 driven LOW | ~33 µA | #313 |
| RTC peripherals kept on (ext0, internal pulls) | ~1 µA | [V ESP32-S3 DS Table 5-10] |
| RTC clock 17.5 MHz/256 instead of the RC | ~5 µA | [V IDF System Time docs] |

## Battery and charging

- **Charger.** TP4065, SOT-23-5, VCC = VBUS. Only USB charges the cell; no header carries VBUS (#42).
- [V] **Charge current.** R13 = 2 kΩ, and RPROG = 1050/IBAT (for IBAT > 0.1 A) gives about 525 mA. The datasheet spec at 2.1 kΩ is 425/500/575 mA (min/typ/max) [V TP4065 DS].
  - About 2.1C for the 250 mAh cell. The owner accepted this (ADR-0010).
  - [C] The schematic note "R13 IBAT (mA) = 580" is a TP4056 value. The README says 500 mA (T-Display-S3:README.md:238).
- **Charge profile** [V TP4065 DS]:
  - trickle at about 15% of the set current below the trickle threshold;
  - CC/CV to 4.2 V ±1% (4.158-4.242 V);
  - thermal regulation reduces the current above about 0.4 A;
  - termination at C/10 of the set current (40-55 mA at 2.1 kΩ);
  - automatic recharge at about 80-90% of capacity;
  - standby supply current 75 µA from VCC.
  - [I] Terminating near 50 mA (about C/5 for this cell) stops a few percent short of full. CC lasts about 25 min, the whole charge about 35-45 min.
- **On USB** the load runs from VBUS through D3, and the cell is only charged and topped up. Always-on mode needs no charge control.
- **No charge-status GPIO.** CHRG drives only the red LED (LED1, R9 2 kΩ from VBUS) [V SCH]:
  - on: charging;
  - off: full, or not charging;
  - flashing or dim: no battery. The datasheet gives a 0.5-3 s period for 10 µF on BAT [V TP4065 DS]; this board has about 20 µF on VBAT.
  - Firmware can say "on USB", never "charging" or "full" (ADR-0010).
- **Green LED** (LED8, 2 kΩ from V3V) is lit whenever V3V is on, so always on USB. It cannot be switched off on USB.
- **R10** (the "5V/2A" fuse) degrades in the field: 5-45 Ω after charge cycles with boot loops on battery (#291), and about 350 Ω on two v1.2 boards (#223). The maintainer says bridging it is safe with a protected cell (#291). Health check: [power-budget.md](power-budget.md#r10-health-from-the-vsys-sag).
- **No protection IC** on the board apart from R10 (#230). Gogadget uses a protected cell.
- **Connector polarity.** The maintainer says reverse polarity "will not cause any damage" (#317), and the TP4065 lists reverse-battery protection. Check the plug against the board marking anyway.
- **Input limits.**
  - Never put more than 4.2 V on P3: 5 V there "most likely burned out the ... charging chip" (#343).
  - Never feed the "5V" header with a cell fitted (#270).
- **Cell size.** The README says at most 7 x 18 x 50 mm (T-Display-S3:dimensions/README.MD). The official shell takes at most 47 x 25 x 7 mm (#324).

## GPIO4 sensing

GPIO4 (ADC1_CH3) reads half of the switched BAT net, which is VSYS, not the cell [V SCH, #190, #284]. Source impedance is 50 kΩ (R2 and R4 in parallel), so C13 settles with τ = 0.5 ms when Q2 turns on. With Q2 off, C13 discharges through R4 with τ ≈ 1 ms [I].

**USB test** (run first; it needs GPIO15 LOW):

1. Backlight off, GPIO15 LOW. On battery, V3V and the backlight supply drop out.
2. Wait at least 5 ms, unless GPIO15 was already LOW (for example right after a deep-sleep wake).
3. Read GPIO4. About 2.3 V means USB; about 0 V means battery. Use a threshold of about 1 V at the pin [V SCH, #284].
4. USB is the only thing that can power Q1 with GPIO15 LOW. There is no other VBUS sense line (#292).
5. `usb_serial_jtag_is_connected()` detects a USB data host, not a charger [V IDF v6.1 header].

**VSYS measurement** (battery only):

1. GPIO15 HIGH, then wait 3-5 ms: transistor turn-on plus about 7τ.
2. Sample before the backlight or Wi-Fi load starts.
3. Use `adc_oneshot` on ADC1_CH3, `ADC_ATTEN_DB_12`, with curve-fitting calibration (the legacy `esp_adc_cal` was removed in IDF 6.0 [V]).
4. Average 16-32 samples and multiply by 2.
5. Label the result "VSYS". It reads a few tens of mV below the cell's open-circuit voltage: VSYS = Vcell - I x (R10 + Q4 + protection FETs).

- The 2:1 ratio is nominal: two 1% resistors give about ±1% [I].
- LilyGO staff call the uncalibrated readings rough (#22, #57, #59); one user saw 3400 mV against 3600 mV on a meter (#190).
- On USB the reading is about (VBUS - Vf(D3))/2 and says nothing about the cell (T-Display-S3:README.md:170).
- [C] LilyGO's "No battery connect!" above 4300 mV really means "USB present" (T-Display-S3:examples/GetBatteryVoltage/GetBatteryVoltage.ino:86-90, #190).
- No level wake: 2.3 V is below the ESP32-S3 VIH of 0.75 x VDD, about 2.48 V [V ESP32-S3 DS Table 5-4]. Noticing USB in deep sleep needs a timer wake or a ULP ADC poll [I].

## Display panel (ST7789V)

- **Module.** NFP190B (named in the LilyGO shell STEP), ST7789V controller, 170x320 IPS.
- **Frame memory.** 240x320x18 bit. The 170 visible columns are centred: 35 + 170 + 35 = 240 [V DS].
- **Supplies.** Logic VDD/VDDI from VDD3V3 (CN1-2, always on). LED anodes on V3V (CN1-20). Cathodes LEDK1-4 go to the AW9364 [V SCH].
- **Current** [V DS Table 3, 60 Hz, default registers]:
  - Normal mode: IDD 6.0 mA typical (7.5 max).
  - Sleep In: IDD 15 µA + IDDI 5 µA typical (30 + 10 max).
- **Write-only bus.** esp_lcd i80 cannot read the panel (RD is held HIGH), so the panel ID and revision cannot be read (#87, #122, #300).

### Bus and pixel clock

- **Bus.** 8-bit 8080: DC 7, WR 8, D0-D7 = 39, 40, 41, 42, 45, 46, 47, 48, CS 6, RST 5, RD 9.
- **Limit** [V DS Table 4, p.41]: write cycle TWC >= 66 ns and TWRL/TWRH >= 15 ns, so at most about 15.15 MHz.
- **Clock** [V esp-idf v6.1 components/esp_lcd/priv_include/esp_lcd_common.h `LCD_PERIPH_CLOCK_PRE_SCALE`, components/soc/esp32s3/include/soc/clk_tree_defs.h]: the S3 i80 clock is PLL_F160M with a fixed pre-scale of 2, so 80 MHz divided by an integer. 10 MHz is exactly /8 (100 ns cycle).
  - If artefacts appear, step down to 8 MHz (/10) or 6.67 MHz (/12), never up (ADR-0006).
- [C] **LilyGO's clocks.** 16 MHz (factory, adc038e), 20 MHz (2022) and 40 MHz (nes) all exceed the datasheet. lv_demos asks for 6.528 MHz and actually runs at 6.67 MHz (/12).
  - The factory comment "Too low or too high pixel clock may cause screen mosaic" (T-Display-S3:examples/factory/pin_config.h:24) is unexplained.
- **PSRAM buffers.** Espressif's IDF v6.1 i80 example drops to 2 MHz when draw buffers are in PSRAM on the S3, because of PSRAM bandwidth (esp-idf#12342 shows corruption at 10 MHz under Wi-Fi). Gogadget's buffers are in internal DMA RAM.
- **Frame time.** A full 320x170 RGB565 frame (108,800 B) takes 10.9 ms at 10 MHz.

### Vendor init table

LilyGO's `LCD_MODULE_CMD_1` table: 14 entries, byte-identical in all 8 LilyGO copies [V]. Sources: T-Display-S3:examples/factory/factory.ino:133-149, send loop factory.ino:325-331; LilyGo-Display-IDF:main/display_s3.c:32-47, send loop display_s3.c:139-143. Decoded against DS v1.4:

| # | Cmd | Register | Bytes sent | Decode | Power-on default |
|---|---|---|---|---|---|
| 1 | 11h | SLPOUT | (none), then wait 120 ms | Leave Sleep In: DC/DC, oscillator and scanning start (p.184) | Sleep In |
| 2 | 3Ah | COLMOD | 05 | RGB-interface field 000 (unused on 8080); MCU-interface field 101 = 16 bpp (p.224) | 18 bpp |
| 3 | B2h | PORCTRL | 0B 0B 00 33 33 | Normal mode: back porch 11, front porch 11 lines. PSEN 0. Idle mode BP/FP 3/3, partial mode BP/FP 3/3 (p.263) | 0C 0C 00 33 33 |
| 4 | B7h | GCTRL | 75 | VGH 14.97 V (VGHS 7), VGL -10.43 V (VGLS 5) (p.268) | 35h: 13.26 V / -10.43 V |
| 5 | BBh | VCOMS | 28 | VCOM 1.100 V (p.273) | 20h: 0.900 V |
| 6 | C0h | LCMCTRL | 2C | XBGR 1, XMX 1, XMH 1: XOR the MADCTL BGR and MX bits (p.275). No-op | 2Ch |
| 7 | C2h | VDVVRHEN | 01 | CMDEN 1: VDV/VRH come from registers (p.277). The second byte, FFh, is not sent. No-op | 01h/FFh |
| 8 | C3h | VRHS | 1F | GVDD (VAP) 5.10 V + (VCOM + offset + VDV); GVCL -5.10 V + offsets (p.278) | 0Bh: 4.10 V |
| 9 | C6h | FRCTRL2 | 13 | NLA 0 = dot inversion; RTNA 13h = 53 Hz in the table (p.284) | 0Fh: 60 Hz |
| 10 | D0h | PWCTRL1 | A7 | [C] Off-spec: the first parameter is documented as a fixed A4h, and the second byte is missing (p.290). Superseded by entry 11 | A4h/81h |
| 11 | D0h | PWCTRL1 | A4 A1 | A4 = fixed key. A1: AVDD 6.8 V, AVCL -4.8 V, VDS 2.3 V (p.290) | A4h/81h: AVCL -4.4 V |
| 12 | D6h | (undocumented) | A1 | [C] D6h is not in the v1.4 command list | - |
| 13 | E0h | PVGAMCTRL | F0 05 0A 06 06 03 2B 32 43 36 11 10 2B 32 | Positive gamma, 14 parameters (fields below, p.294) | see below |
| 14 | E1h | NVGAMCTRL | F0 08 0C 0B 09 24 2B 22 43 38 15 16 2F 37 | Negative gamma, same layout (p.296) | see below |

Gamma fields (hex). The layout is the same for E0h (V..P) and E1h (V..N):

| Param | Field(s) | E0h vendor | E0h default | E1h vendor | E1h default |
|---|---|---|---|---|---|
| 1 | V63[7:4], V0[3:0] | F, 0 | D, 0 | F, 0 | D, 0 |
| 2 | V1[5:0] | 05 | 00 | 08 | 00 |
| 3 | V2[5:0] | 0A | 02 | 0C | 02 |
| 4 | V4[4:0] | 06 | 07 | 0B | 07 |
| 5 | V6[4:0] | 06 | 0B | 09 | 05 |
| 6 | J0[5:4], V13[3:0] | 0, 3 | 1, A | 2, 4 | 2, 5 |
| 7 | V20[6:0] | 2B | 31 | 2B | 2D |
| 8 | V36[6:4], V27[2:0] | 3, 2 | 5, 4 | 2, 2 | 4, 4 |
| 9 | V43[6:0] | 43 | 40 | 43 | 44 |
| 10 | J1[5:4], V50[3:0] | 3, 6 | 2, 9 | 3, 8 | 1, C |
| 11 | V57[4:0] | 11 | 12 | 15 | 18 |
| 12 | V59[4:0] | 10 | 12 | 16 | 16 |
| 13 | V61[5:0] | 2B | 12 | 2F | 1C |
| 14 | V62[5:0] | 32 | 17 | 37 | 1D |

- **What it does.** The table only tunes porches, voltages, frame rate and gamma. It contains no MADCTL, INVON, RAMCTRL, CASET/RASET or DISPON, so esp_lcd's orientation, inversion and endianness settings survive it.
- **Why it is sent.** The maintainer: "This part of initialization is provided by the screen manufacturer. If this part of the initialization command is not added, the pixels will be blurred." (#229).
  - [I] In 2023 another maintainer said frosted (matte) panels do not need it (#87). LilyGO has sent it on every unit since 2023, and Gogadget sends it byte-for-byte on every init (ADR-0006).
- [I] **COLMOD.** 05 and esp_lcd's 55 differ only in the RGB-interface field, which the 8080 bus does not use. The datasheet asks for 55h for 16-bit writes (p.224, note 2).
- **Frame rate.** Frame rate = 10 MHz / ((320 + FPA + BPA) x (250 + 16 x RTNA)) [V DS p.284]; the 10 MHz is the panel's internal oscillator, not the pixel clock. With CMD_1's 11/11 porch this gives 52.8 Hz, an 18.95 ms scan. The defaults give about 59 Hz.
- **Port notes.**
  - Send each entry with `esp_lcd_panel_io_tx_param()`; it accepts at most 32 parameter bytes and drains queued colour DMA first [V esp_lcd_panel_io_i80.c @v6.1].
  - Store an explicit `delay_ms` per entry instead of LilyGO's 0x80 length flag.
  - Never copy the send loops in nes, usb_hid_pad, touch_test and GetBatteryVoltage: `j < len & 0x7f` parses as `(j < len) & 0x7f` and sends 128 bytes after SLPOUT ([Known errata](#known-errata)).

### Panel timing rules

| Rule | Value | Source |
|---|---|---|
| RESX low pulse | >= 10 µs; pulses under 5 µs are rejected | [V DS Table 9, p.48] |
| After RESX release, before any command | >= 5 ms | [V DS Table 9 note 7, p.49] |
| After RESX release, before SLPOUT | >= 120 ms | [V DS Table 9 note 7] |
| Reset completes (registers reloaded) | <= 5 ms if reset in Sleep In; <= 120 ms if reset in Sleep Out (display blanked meanwhile) | [V DS Table 9 notes 1, 3, 5, 6] |
| After SLPOUT, before the next command | >= 5 ms | [V DS 9.1.12, p.184] |
| After SLPOUT, before SLPIN | >= 120 ms | [V DS 9.1.11-12] |
| After SLPIN, before the next command | >= 5 ms | [V DS 9.1.11, p.182] |
| SLPOUT when awake, SLPIN when asleep | no effect | [V DS p.182, p.184] |
| After SWRESET | >= 5 ms; >= 120 ms before SLPOUT if sent in Sleep In | [V DS 9.1.2, p.163] |
| State after power-on or hardware reset | Sleep In, display off, inversion off, 18 bpp (SWRESET keeps COLMOD) | [V DS register defaults] |

What the IDF v6.1 driver does [V esp-idf v6.1 components/esp_lcd/src/esp_lcd_panel_st7789.c]:

- `esp_lcd_new_panel_st7789()`: configures RST as an output (`gpio_config`) when `reset_gpio_num >= 0`. The pin then drives whatever its output register holds.
- `esp_lcd_panel_reset()`: RST low for 10 ms, then high, then 10 ms. Without an RST GPIO it sends SWRESET and waits 20 ms.
- `esp_lcd_panel_init()`: SLPOUT, 100 ms, MADCTL, COLMOD 55, RAMCTRL 00 F0 (F8 for little-endian). No DISPON since IDF 5.0.
- `esp_lcd_panel_disp_sleep(panel, true/false)`: SLPIN or SLPOUT, then a fixed 100 ms (the datasheet needs 5 ms).
- `esp_lcd_panel_set_brightness()`: writes WRDISBV, which does nothing on this board (the AW9364 sets brightness).

Cold-init timeline with CMD_1 (nominal):

```
 t (ms)   0      10       20                    120   ~120                         ~240
          |RST low|RST high|SLPOUT (IDF) ... 100 ms|MADCTL COLMOD RAMCTRL INVON|SLPOUT (CMD_1) ... 120 ms|rest of CMD_1
                   ^ release                                                   then: first frame, DISPON, backlight
```

- **Fixed delays** total about 240 ms (10 + 10 + 100 + 120), most of the 0.4-0.5 s wake-to-time estimate.
- [C] **The IDF SLPOUT is early.** It comes 10 ms after RESX release, inside the 120 ms window that note 7 forbids. CMD_1's SLPOUT + 120 ms is the safety margin LilyGO relies on, so dropping it is not a free 120 ms saving.
- [I] **CMD_1's SLPOUT is early too.** It lands about 110 ms after release, also inside the window. LilyGO ships exactly this sequence on every unit, so the panel evidently accepts it. If a unit ever comes up blank or wrong after `esp_restart()` (reset applied in Sleep Out), first make CMD_1's SLPOUT land >= 120 ms after release. See [Open questions](#open-questions).
- [V] **Tick granularity.** IDF's waits are `vTaskDelay(pdMS_TO_TICKS(n))`. `pdMS_TO_TICKS()` rounds down, and `vTaskDelay(k)` returns after k-1 to k tick periods.
  - At IDF's default `CONFIG_FREERTOS_HZ=100`, a 10 ms wait is anything from about 0 to 10 ms, and `pdMS_TO_TICKS(5)` is 0. The early reset wait is harmless here, because CMD_1 re-sends SLPOUT and every register it cares about.
  - Gogadget's `firmware/sdkconfig.defaults` sets 1000 Hz, where a 5 ms wait lasts 4-5 ms.
  - For a hard minimum (5 ms after SLPIN, 3 ms of backlight-off), add one tick or use `esp_rom_delay_us()`.
- **Short wakes.** Never send SLPIN within 120 ms of an SLPOUT, for example when sleep is requested right after a wake. Delay the SLPIN instead.

### Orientation and gap

- **Inversion.** Always on: `esp_lcd_panel_invert_color(panel, true)`. It is an IPS panel; every LilyGO path enables it.
- **Orientation** [V T-Display-S3:examples/factory/factory.ino:310-322, LilyGo-Display-IDF:main/display_s3.c:125-137]:
  - `swap_xy(true)` + `mirror(false, true)` = "The screen faces you, and the USB is on the left" (MADCTL MV|MY, TFT_eSPI rotation 3). This is Gogadget's default.
  - `swap_xy(true)` + `mirror(true, false)` = USB on the right (MV|MX). With esp_lvgl_port `.rotation = {swap_xy 1, mirror_x 0, mirror_y 1}`, `LV_DISPLAY_ROTATION_180` produces exactly this.
- **Gap** (0, 35) in both landscape orientations: the 170-row window is centred in 240.
  - esp_lvgl_port never calls `esp_lcd_panel_set_gap()`, so the application must set it [V esp_lvgl_port 2.9 source].
  - One Rust user needed 36 instead of 35 (#160). The 1-px frame test in [bring-up.md](bring-up.md#display) settles it per unit.
- **Colour order.** RGB (`LCD_RGB_ELEMENT_ORDER_RGB`) with CMD_1, whose LCMCTRL 2Ch is the power-on default with XBGR = 1.
  - [I] TFT_eSPI's generic init writes LCMCTRL 0Ch (XBGR = 0). That likely explains why LilyGO's TFT_eSPI setup used BGR from 2022 to 2026, and the RGB/BGR confusion in #121 and #122.
- **RD (GPIO9)** must be driven HIGH before the bus is created and kept HIGH. The esp_lcd i80 bus never touches it (#103).

### Byte order

- RGB565 must be byte-swapped on this 8-bit bus; LilyGO swaps in software (LVGL 8 `LV_COLOR_16_SWAP`).
- **Gogadget** uses the LCD_CAM hardware swap: `esp_lcd_panel_io_i80_config_t.flags.swap_color_bytes = 1`, with esp_lvgl_port `swap_bytes = 0` (ADR-0006).
  - It applies to colour data only. Command parameters are untouched because param_bits equals the bus width [V esp_lcd_panel_io_i80.c @v6.1].
  - LVGL buffers stay plain RGB565, identical to the simulator.
- **Fallbacks.** Use exactly one swap:
  1. `data_endian = LCD_RGB_DATA_ENDIAN_LITTLE`, which sets RAMCTRL B0h to 00 F8. Untested on this panel; CMD_1 never writes B0h.
  2. esp_lvgl_port `swap_bytes = 1`, a CPU copy on every flush.

### GRAM retention

[V DS p.182, p.201, p.194]:

- Frame memory is random at power-on.
- It is **not cleared** by a hardware or software reset.
- It is kept in Sleep In; the MCU interface keeps working there.
- DISPOFF and DISPON do not change it.

The panel's VDD is never switched, so:

- After a deep-sleep wake or `esp_restart()`, the previous frame (the stale time) is still in GRAM.
- After a real power-off of about a minute or more, GRAM shows noise at the next boot (#232).
- Never turn the backlight on (or send DISPON) before the new frame is written. LilyGo-Display-IDF turns the backlight fully on before LVGL draws anything (LilyGo-Display-IDF:main/display_s3.c:147-153), which is wrong.
- [I] esp_lvgl_port's `lvgl_port_add_disp()` may paint a white frame into GRAM before the application draws (esp-bsp issue #776). It stays invisible while DISPON and the backlight wait for the first application frame.
- [V] `esp_lcd_panel_disp_on_off(panel, true)` drains queued colour DMA before DISPON, so calling it right after `lv_refr_now()` shows a complete frame.

### Colour quirks

- Pure red (0xF800) shows as orange, and light greys turn greenish off-axis. The gamma comes from the panel maker and LilyGO cannot tune it (#113, unresolved). Choose the palette on the real panel; simulator screenshots are not colour truth.
- One developer saw an intermittent colour flip for about 1 s, as if INVON were lost, on a custom carrier board (#118, unresolved).
- [I] The maintainer blamed a half-screen fault on an OLED-like burn-in effect (#128). Always-on mode shifts the face by 1-2 px every few minutes for this reason.
- Glossy and matte panels may differ; record the finish of every unit.

### No TE line

- CN1 pin 19 is not connected and no TE GPIO exists [V SCH], so tear-free (vsync) updates are impossible.
- [I] A 10 MHz full-frame write (10.9 ms) is shorter than one 18.95 ms panel scan, so an unsynchronised full frame shows at most one tear boundary. Keep updates small and flush each changed region in one transaction.

### Panel sleep

- **SLPIN is mandatory before every deep sleep.** The panel draws 6.0 mA in normal mode against about 20 µA in Sleep In; #289 measured 6.42 mA versus 263 µA.
- **#300 ghosting.** Matte units left in Sleep Out during minutes of deep sleep showed white borders and ghost images. The user fixed it by driving every LCD pin LOW.
- **DISPOFF is not a low-power state.** It only blanks the output; one user still saw about 30 mA extra (#33). SLPIN blanks the display by itself.
- [V] **LilyGO's own sleep code** sends SLPIN with no 5 ms wait, holds no pin and wakes via ext0 on GPIO14 only (T-Display-S3:examples/factory/factory.ino:395-404; examples/Sleep/Sleep.ino:77-116). Gogadget's sequence is in [power-budget.md](power-budget.md#deep-sleep-entry-and-exit).

### Reset-free wake

[I] Untested; this is the planned fallback if wake-to-time exceeds 0.5 s. VDD3V3 stays on and Sleep In keeps registers and GRAM, so a deep-sleep wake could skip reset, init and CMD_1 (about 0.23 s):

1. Keep RST (GPIO5) and CS (GPIO6) held HIGH through sleep.
2. Create the panel with `reset_gpio_num = -1`, or set RST's output level HIGH first, so `esp_lcd_new_panel_st7789()` cannot pull RST low.
3. Write the new frame while the panel is still in Sleep In. The MCU interface works there [V DS p.182].
4. SLPOUT, wait >= 5 ms, then DISPON (if DISPOFF was sent), then the backlight.
5. Use the full init for every reset reason other than `ESP_RST_DEEPSLEEP`.

## Backlight (AW9364)

- **Wiring** [V SCH]: U4 AW9364DNR. EN = GPIO38 directly; VIN = V3V (CW1 1 µF); LED1-4 sink the four LED strings (LEDK1-4, CN1-21..24). The anodes are on V3V (CN1-20).
- **Control.** Pulse counting on EN, never LEDC/PWM (ADR-0007). Every PWM low phase of 0.5-500 µs counts as one step down, so PWM gives erratic brightness [I]. The maintainer called LEDC "wrong" (#278) and removed it in 02f0e9d.
  - [C] 02f0e9d missed the community LovyanGFX MIDI examples (#328-#336), which still drive GPIO38 with `Light_PWM` at 22 kHz.

Datasheet timing and limits [V Awinic AW9364 datasheet V2.4, pp.5-9]:

| Parameter | Datasheet | Gogadget |
|---|---|---|
| Start-up after EN HIGH (TON) | 20 µs typ ("recommended to be greater than 20 µs") | >= 30 µs (LilyGO's value) |
| Dimming pulse high time (THI) | >= 0.5 µs | 1-2 µs |
| Dimming pulse low time (TLO) | 0.5-500 µs | 1-2 µs, whole train inside a critical section |
| Shutdown delay with EN LOW (TOFF) | 0.8-2.5 ms | EN LOW >= 3 ms |
| EN input | active high, internal 150 kΩ pull-down; VIH >= 1.5 V, VIL <= 0.3 V; deglitch filter | no hold needed in deep sleep |
| VIN | 2.8-5.5 V (here V3V ≈ 3.3 V) | - |
| LED current at 16/16 | 16.5 / 20 / 23.5 mA per channel (min/typ/max); matching ±1% typ, ±5% max | about 80 mA total |
| Dropout | 50 mV typ, 170 mV max at 20 mA | - |
| Quiescent current, enabled | 330 µA typ | counted in glance current |
| Shutdown current | 0.1 µA typ, 1 µA max | - |

- **Undefined window.** A low phase longer than 500 µs but shorter than 2.5 ms may or may not shut the chip down. That is why the pulse train runs inside a critical section, and why the cached level is invalidated after any EN-low period longer than 0.5 ms or any loss of V3V (on battery, GPIO15 LOW) (ADR-0007).

Levels [V AW9364 DS Table 1, Fig. 7]. EN HIGH from shutdown is rising edge 1 and gives level 16 (20 mA per channel). Each further rising edge lowers the current by 1.25 mA per channel, and edge 17 wraps back to 16:

| Level | Pulses after enable | Per channel | Total, 4 strings (typ) |
|---|---|---|---|
| 16 | 0 | 20.00 mA | 80 mA |
| 12 | 4 | 15.00 mA | 60 mA |
| 8 | 8 | 10.00 mA | 40 mA |
| 6 (battery default) | 10 | 7.50 mA | 30 mA |
| 4 | 12 | 5.00 mA | 20 mA |
| 1 | 15 | 1.25 mA | 5 mA |
| 0 | EN LOW >= 3 ms | 0 | 0 |

- **Current.** Level L draws L x 1.25 mA per channel, about 5 mA x L in total, ±17.5% per the ILED spread. Add 0.33 mA quiescent while enabled. The LED current comes from the cell through the linear LDO, so battery current ≈ LED current.
- **Step maths** (LilyGO, MIT): pulses from level a to level b (both 1-16) = (16 + a - b) mod 16. So going from 1 to 2 takes 15 pulses (T-Display-S3:examples/factory/factory.ino:223-248, 2d3c294).
- [I] **Headroom.** V3V ≈ 3.3 V is below the datasheet's 3.6 V test condition, and white LEDs need about 3.1-3.2 V plus dropout. The top levels may compress, so check monotonicity, not linearity ([bring-up.md](bring-up.md#backlight)).
- **No flash at reset.** At reset GPIO38 is an input and the internal pull-down holds EN LOW, so the backlight stays off until firmware enables it, even on USB where V3V is on from power-up.
- **On USB**, V3V is forced on; only EN turns the backlight off.
- [I] Behaviour when V3V rises while EN is already HIGH is undocumented, presumably level 16. Raise GPIO15 before EN.

## Buttons

| | GPIO0 (BOOT, S2) | GPIO14 (KEY, S3) |
|---|---|---|
| Pull-up [V SCH] | R1 10 kΩ to VDD3V3 (100 kΩ on the 2023 sheet [C]) | R3 100 kΩ to VDD3V3 |
| Capacitor | none, so contact bounce is visible | C14 100 nF to GND |
| Release edge | fast | τ ≈ 10 ms; reaches VIH (0.75 x VDD) about 14 ms after release [I] |
| Current while held | ~330 µA | ~33 µA |
| Debounce | 20-30 ms in software | allow >= 20 ms to settle |
| Strapping pin | yes (boot mode) | no |
| Gogadget (ADR-0008) | Enter, press only | Next (press), Back (hold); long press at boot reserved for provisioning |

- **Positions** [I], derived by rotating LilyGO's pinmap drawing:
  - USB on the left: GPIO0 upper-left, GPIO14 lower-left.
  - USB on the right: GPIO14 upper-right, GPIO0 lower-right.
  - RST is the small side button near the USB connector (#184, #282); CHIP_PU has RW 10 kΩ + CU 1 µF [V SCH].
- **Wake.**
  - Both pull-ups go to the always-on VDD3V3, so ext1 ANY_LOW on `BIT64(0) | BIT64(14)` works with internal pulls off and the RTC peripherals powered down [V esp_sleep.h @v6.1: ext1 "will work even if RTC peripherals are shut down"].
  - Use `esp_sleep_enable_ext1_wakeup_io()` and `esp_sleep_get_ext1_wakeup_status()` to learn which button woke the chip.
  - ext1 is level-triggered: a button still held at sleep entry wakes the chip at once. Both lines must read HIGH before sleeping.
  - LilyGO only ever woke via ext0 on GPIO14 (factory, Sleep, #289, #300). GPIO0 and ext1 wake are untested upstream.
  - OneButton 2.0.4 (BSD; 50/400/800 ms) is LilyGO's reference state machine (T-Display-S3:lib/OneButton/src/OneButton.cpp:217-330).
- **Strapping caveat.**
  - [V] A deep-sleep wake and `esp_restart()` are Core Resets; strapping pins are latched only at a Chip Reset (ESP32-S3 TRM v1.8, Table 7.1-1 and chapter 8). So BOOT is safe as a wake key.
  - A Chip Reset happens at power-on and when RST (CHIP_PU) is released. GPIO0 LOW at that moment selects download mode [V ESP32-S3 DS Table 3-3]; this is the manual recovery path (T-Display-S3:README.md:215-222).
  - [I] IDF v6.1 programs the brownout detector for a System Reset, not a Chip Reset ([LDO and supply limits](#ldo-and-supply-limits)). A brownout with BOOT held should therefore boot normally, but a full supply collapse is a power-on reset.
  - Treat "BOOT held at any reset" as a download-mode risk, and never teach users to hold BOOT (ADR-0008).

## USB, console and flashing

- **Port.** USB-C goes to the S3's native USB (GPIO19/20), enumerated as USB-Serial-JTAG 303A:1001. UART0 goes to the JST-SH connector.
  - IDF's default console is UART0; select the USB-Serial-JTAG console, as Gogadget does (#266).
- **IDF v6.1 USB-Serial-JTAG behaviour:**
  - [V usb_serial_jtag_vfs.c] With no host connected, writes fail at once (EIO). With a host connected but not reading, one write stalls for up to 50 ms, then output is dropped. Logging cannot block a battery wake.
  - [V] It does not work in or after light sleep. `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` blocks automatic light sleep while a host is connected.
  - [V IDF v6.1 console docs] The USB device disappears during deep sleep and re-enumerates on wake, so a monitor loses the port at every sleep (`power.stay` keeps it, ADR-0016).
- **Resets from the host.** Opening or closing a terminal may reset the board via DTR/RTS (#274).
- **Download mode by hand**: hold BOOT, press and release RST, release BOOT.
  - After flashing in this mode, press RST to run the app (T-Display-S3:README.md:215-222; #16, #162).
  - The ROM bootloader cannot be overwritten, so a bad app cannot brick the board (#137).
  - An app that crash-loops or floods the console can block auto-reset; use the manual mode (#100, #150).
- [I] **Reboot to download mode from software**: set `RTC_CNTL_FORCE_DOWNLOAD_BOOT`, then reset (ESP32-S3 TRM chapter 8). This is a possible console command, not yet planned.
- **Second console.** UART0 on the JST-SH (GPIO43 TX, GPIO44 RX, 3V3, GND; 3.3 V levels, no pull-ups) works as a second log port (#108). It needs a 3.3 V USB-UART adapter, whose port stays open on the host through deep sleep, unlike the board's own USB device.
  - The ROM prints its boot messages on UART0 and USB-Serial-JTAG by default [V ESP32-S3 DS §3.3]. `esp_deep_sleep_disable_rom_logging()` silences them on deep-sleep wakes (esp-idf#15921).
- **Debugging.** On-chip USB-JTAG works with OpenOCD `board/esp32s3-builtin.cfg` (#210). External JTAG is impossible: GPIO39-42 are LCD D0-D3.
- **TinyUSB** builds (as in usb_hid_pad) lose the USB-Serial-JTAG console and auto-download. Gogadget does not use TinyUSB.
- **CC pins.** No schematic revision shows the 5.1 kΩ Rd resistors on CC1/CC2 [V SCH], so compliant USB-C sources and C-to-C cables may not supply VBUS (#65, #166, #233).
  - [I] Unconfirmed on hardware: the #230 PCB photo shows parts R01/R04 that are not in the schematic.
  - LilyGO ships a USB-A-to-C cable. Test each unit ([bring-up.md](bring-up.md#usb-plug-and-unplug)).
- **Factory images** (T-Display-S3:firmware/) are Arduino builds: DIO flash mode, the 8 MB layout, and Wi-Fi configuration stored in NVS at 0x9000. Erase the flash before the first Gogadget flash ([bring-up.md](bring-up.md#first-flash)). The "ScreenDetection" image only prints compile-time pin settings; it does not probe the panel.

## Wi-Fi, BLE and antenna

- **Radio.** 2.4 GHz 802.11 b/g/n and Bluetooth LE only; no Classic Bluetooth (#200).
  - Current [V ESP32-S3 DS Tables 5-7, 5-8]: TX peaks 283-340 mA, RX 88-91 mA, BLE TX up to 335 mA at 21 dBm.
  - The average is about 150 mA or more with Wi-Fi on, and the heat is centred on the ESP32-S3 (#295).
- **v1.2 antenna is weak.** One report covers about 100 boards (#340); others report 1-2 bars and pings of 200 ms or more (#67, #123). Expect long or failed associations: bounded timeouts, retries, RSSI logging.
- **v1.3** has a re-matched onboard antenna, the only stated change (#340). Its silkscreen and FCC grant are in [Board identification and revisions](#board-identification-and-revisions).
- **External antenna.** There is an IPEX/U.FL socket. Rotate the 0201 0 Ω resistor next to it to switch paths (T-Display-S3:README.md:258-262, #39, #340). Results were mixed (#67, #123).
- **Enclosure.** Keep the antenna end (opposite the USB port) clear of the battery, metal and fingers.
- **Association.** One user fixed an association failure by setting WPA2 as the minimum security (#247). One NTP failure on a DHCP network is unexplained; a DHCP-supplied NTP server is suspected (#297). Gogadget ignores DHCP NTP servers (ADR-0011).

## Memory, flash and PSRAM

- **Chip.** ESP32-S3R8: dual-core LX7, 8 MB octal PSRAM in the package (AP Memory, 3.3 V, per the #103 boot log), using GPIO33-37 internally.
  - A quad PSRAM configuration boot-loops (#103): use `CONFIG_SPIRAM_MODE_OCT`.
- **Flash.** External Winbond W25Q128JV, 16 MB, on VDD_SPI (3.3 V via the chip's internal switch, which needs GPIO45 LOW at a chip reset). It runs QIO at 80 MHz (T-Display-S3:boards/lilygo-t-display-s3.json).
- **Boot timing.** Issue #4 in LilyGo-Display-IDF, on a sibling board with the same SoC (IDF 5.3-dev, DIO flash, default settings): app image load about 180 ms, octal PSRAM init about 65 ms, PSRAM memtest about 735 ms. Gogadget turns the memtest off and uses QIO (sdkconfig decisions).
- [I] **Wake-to-time.** Expected 0.4-0.5 s with a full panel init, of which about 0.24 s is fixed panel delay. A reset-free wake could save about 0.23 s.
- **Draw buffers.** The factory example fits a 108.8 KB full-frame internal DMA buffer alongside Wi-Fi. Gogadget uses two 320x40 internal DMA buffers.
- **Partitions.** The partition layout is in ADR-0013. LilyGO publishes no IDF partition table.
- **RTC memory** [V esp-idf v6.1 components/esp_system/ld/esp32s3/memory.ld.in]: 8 KB RTC FAST plus 8 KB RTC SLOW. Kept through deep sleep; lost at power-on and RST (CHIP_PU).
- [V] **Deep-sleep entry with digital-pad holds** (`gpio_deep_sleep_hold_en()`) asserts that the calling task's stack is in internal RAM. When task stacks may be placed in PSRAM, IDF v6.1 also refuses deep sleep from a task with a PSRAM stack (`esp_deep_sleep_try_to_start()` returns `ESP_ERR_NOT_ALLOWED`).

## Timekeeping

- **Awake.** The 40 MHz crystal is 10 ppm (#262), but it only counts while the chip is awake.
- **No 32 kHz crystal is possible.** XTAL_32K_P/N are GPIO15 (PWR_EN) and GPIO16 [V SCH].
- **Deep-sleep clock sources** [V esp-idf v6.1 components/esp_hw_support/port/esp32s3/Kconfig.rtc]:
  - `CONFIG_RTC_CLK_SRC_INT_RC`: internal 136 kHz RC, the default, calibrated at boot over `RTC_CLK_CAL_CYCLES` = 1024.
  - `CONFIG_RTC_CLK_SRC_INT_8MD256`: 17.5 MHz RC divided by 256, calibration default 3000 cycles. "Better frequency stability" for about 5 µA more in deep sleep [V IDF System Time docs].
  - The choice follows a 24 h drift test ([bring-up.md](bring-up.md#rtc-drift)).
- [I] **Drift is unmeasured on this board.** The RC "drifts with temperature" (IDF docs) with no ppm figure; an ESP32-C6 RC was measured at -1540 ppm/°C (esp-idf#19248). Expect 10^2-10^3 ppm, that is 9-90 s per day.
  - Each sleep interval is converted with a 10-cycle calibration taken at sleep entry (sleep_modes.c), while the die is still warm.
- **±1 min budget.** ±60 s over a 6 h sync interval is 2,800 ppm; over 24 h it is 690 ppm.
- **Resets.** The RTC timer persists across all resets except power-on resets [V IDF System Time docs].
  - [I] The S3 brownout is a System Reset, which also resets the RTC domain (ESP32-S3 TRM v1.8 §7.1). Expect the time to be lost after a brownout: the NVS epoch is the recovery point.
- **Upgrade path.** An external TCXO RTC (RV-3032-C7 on I2C 17/18) is the upgrade path (ADR-0011). [I] Its quoted 160 nA and ±2.5 ppm are not yet checked against a datasheet copy ([datasheets.md](../research/upstream/datasheets.md)).

## Mechanical

- **Models** (T-Display-S3:dimensions/):
  - `t-display-s3-pcb.stp`: PCB, 61.98 x 26.09 x 7.86 mm.
  - `t-display-s3-full.stp` and `.stl`: non-touch board with panel, 61.98 x 26.31 x 9.88 mm.
  - `PCB.dwg`: the only source of mounting-hole positions. The pinmap photo shows two holes at the antenna end.
  - Older models (noTouch.stl, the "H569 Not Soldered" STEP) are recoverable from `899de7a^:3D_file/`.
- **Fragile panel.** The glass and the FPC crack easily (#51, #128); the lower part of the FPC, where the data lines are, is the most fragile.
  - Only double-sided tape at each end holds the panel, and it can let go in heat (#176).
  - To remove the panel, lift the bracket by its adhesive and never pry the glass (#335).
- **Shipping foam is conductive.** Reused inside a case it caused failures (#65). Never put it in the enclosure; insulate the back of the PCB where it meets the battery.
- **Battery space.** The README limits the cell to 7 x 18 x 50 mm; the official shell (T-Display-S3:dimensions/LilyGo-Shell/) fits up to 47 x 25 x 7 mm (#324).
- **Antenna end.** Keep it clear (see [Wi-Fi, BLE and antenna](#wi-fi-ble-and-antenna)).

## Known errata

Board and documentation:

1. [C] The "5V" header is VSYS, not VBUS (#270). The README's "VBUS (pin header) 5V" and "VBAT (pin header) 4.2V" rows (T-Display-S3:README.md:271-272) do not match the schematic, which has only the "+5V" (VSYS) net on the headers.
2. [C] The README calls GPIO15 "LCD power control"; it gates only V3V and the ADC divider (#352).
3. [C] The schematic's "R13 IBAT (mA) = 580" is a TP4056 value; the TP4065 gives about 525 mA.
4. [C] The product page says the battery connector is JST-GH; it is a 1.25 mm MX1.25/PicoBlade-style connector (#287).
5. [C] The 4-pin JST-SH sold as "STEMMA QT / Qwiic" carries UART0 (GPIO43/44), not I2C 17/18.
6. [C] The 2024 schematic changed the GPIO0 and CHIP_PU pull-ups (100 kΩ -> 10 kΩ) without notice (#236).
7. The silkscreen version does not identify the charger IC or the LDO ([Board identification and revisions](#board-identification-and-revisions)).
8. No USB-C CC resistors are shown on the schematic: C-to-C sources may not power the board.
9. The R10 fuse degrades in the field (#223, #291).
10. The LilyGO website pinout image had the I2C pins wrong (#154); the schematic is right (SDA 18, SCL 17).

Vendor code (do not copy):

1. The CMD_1 send loops in nes, usb_hid_pad, touch_test and GetBatteryVoltage use `j < len & 0x7f` and send 128 bytes after SLPOUT. Correct: T-Display-S3:examples/factory/factory.ino:327, examples/Sleep/Sleep.ino:59.
2. The IDF >= 5 ST7789 init no longer sends DISPON. LilyGO fixed the factory example only in c9b939f (2026-07-28); lv_demos still refuses IDF >= 5.
3. The factory pixel clock (16 MHz) is out of spec, and so is nes (40 MHz).
4. The LovyanGFX MIDI examples (2026) still drive the AW9364 with PWM.
5. GetBatteryVoltage's "No battery connect!" means "USB present" (#190).
6. The sleep paths send SLPIN without a 5 ms wait, hold no pins and wake only via ext0 on GPIO14.
7. LilyGo-Display-IDF:
   - turns the backlight fully on before the first frame;
   - uses PSRAM draw buffers at 10 MHz and an idle-priority LVGL task (its issue #8, "very slow", is unanswered);
   - runs touch init on non-touch boards;
   - leaves return codes unchecked;
   - does not build on IDF 6.x (removed fields `color_space`, `psram_trans_align`, `sram_trans_align`).

Still unresolved upstream:

- weak v1.2 Wi-Fi (#67, #123);
- red rendering as orange (#113);
- an intermittent colour flip (#118);
- 1-Wire failing on GPIO43 (#161).

A stale bot closes issues after 30 + 14 days, so "closed" does not mean "fixed".

## Open questions

1. Panel:
   - Does CMD_1 help or hurt on matte panels?
   - What do D0 A7 and D6 A1 do?
   - Does the panel honour RAMCTRL little-endian?
   - Is FPC pin 19 the panel's TE output?
   - What did the factory's "mosaic" comment refer to?
2. Panel timing: is an SLPOUT inside the 120 ms post-reset window honoured, in particular after a reset applied in Sleep Out (`esp_restart()`)?
3. Ghosting: what caused #300 (no SLPIN, floating pins, or the panel itself)? Which parking (Gogadget's or #300's all-LOW) is best on a matte unit?
4. Sleep current: what makes up the 120-190 µA remainder? Which LDO is fitted on each unit?
5. Reset-free wake: do registers and GRAM stay reliable through long deep sleeps with RST and CS held HIGH?
6. GPIO0 and ext1 wake: are they reliable on every unit? (Never tested upstream.)
7. USB unplug: which reset reason appears, and does the RTC time survive? Can plug-in also reset the board?
8. AW9364:
   - Which level does it start at when V3V rises with EN HIGH?
   - Do the top levels compress at 3.3 V?
9. Hardware:
   - What else changed in v1.3?
   - Are CC resistors fitted on real boards?
   - What is R12 for, and can it be removed safely?
   - What are the green LED's Vf and current?
10. Is GPIO21 (CN1-26) loaded inside the non-touch panel?
11. Timekeeping: what are the RC and 17.5 MHz/256 drift on this board, at desk and pocket temperatures?
12. ADC: how accurate is the calibrated GPIO4 reading per unit, without a meter to trim it?

## Datasheets and references

Vendor datasheets are linked, not copied (copyright). The hashes identify the copies read on 2026-10-20; vendors replace files at the same URL. The canonical list, with page counts and what each document was used for, is [docs/research/upstream/datasheets.md](../research/upstream/datasheets.md).

| Document | Version | Public URL | SHA-256 of the copy read |
|---|---|---|---|
| Sitronix ST7789V datasheet | v1.4 (2014-10) | https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/1cc41eb1c76b2f3215130a3dd64e7138ce00b482/datasheet/ST7789V_SPEC_V1.4.pdf | 94c959b15fea4a8dc9381e4c4fa94e51037a39d990f16803a03c7c2294a9cdde |
| Awinic AW9364 datasheet | V2.4 (Feb 2024) | https://doc.awinic.com/doc/202402/d1a46d65-a45d-494f-bfc9-3d6a6a93a860.pdf | 385a9bf7561f639596ba72b710364041a313f396e3061bd97406538d0f45b20f |
| NanJing Top Power (TOPPOWER) TP4065 datasheet | REV_2.0 | https://www.toppwr.com/uploadfile/file/20230304/6403023707f41.pdf | c051635d7740ec188a5b6cbd9c49a0d2e8d47715c5d1324853fc598cd48b61a9 |
| Diodes AP2112 datasheet | DS39724 Rev. 2-2 | https://www.diodes.com/assets/Datasheets/AP2112.pdf | ef8d376f2ec356e29172eb9e053819a0ebdcc576dba7fc9ab0505c568427920f |
| Espressif ESP32-S3 Series Datasheet | v2.2 | https://documentation.espressif.com/esp32-s3_datasheet_en.pdf | 2d5a7cb7fd559d8d972bd88db32669c0196d23f22d7afaafb0f63d099b589a3f |
| Espressif ESP32-S3 Technical Reference Manual | v1.8 | https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf | 4484bf8a69035ec42a731c58c64ada6fbd1f1618c5559409f134d9ea083f444f |
| LilyGO T-Display-S3 schematic (MIT) | 2024-04-29, commit b0a5ecd (blob 318cc4f) | https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/b0a5ecd7967dfc9f3baa9f046a876f0516d0c817/schematic/T_Display_S3.pdf; archived copy: [docs/research/upstream/T_Display_S3-schematic-b0a5ecd.pdf](../research/upstream/T_Display_S3-schematic-b0a5ecd.pdf) | 607b63300636521fc78fbb36ed371ac10e81e3145ffa770784f45c2bf52e08c9 |

Other primary sources:

- LilyGO repositories: https://github.com/Xinyuan-LilyGO/T-Display-S3 (pinned 1cc41eb), https://github.com/Xinyuan-LilyGO/LilyGo-Display-IDF (pinned b1a1cc5).
- ESP-IDF v6.1 (https://github.com/espressif/esp-idf/tree/v6.1):
  - `components/esp_lcd/src/esp_lcd_panel_st7789.c`
  - `components/esp_lcd/i80/esp_lcd_panel_io_i80.c`
  - `components/esp_driver_gpio/include/driver/gpio.h`
  - `components/esp_hw_support/include/esp_sleep.h`
  - `components/esp_hw_support/port/esp32s3/Kconfig.rtc` and `Kconfig.hw_support`
  - `components/esp_hw_support/power_supply/port/esp32s3/Kconfig.power`
  - `components/esp_hal_pmu/brownout_hal.c`
- ESP-IDF System Time docs (esp32s3): https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/system_time.html

## Notes

Discrepancies between research sources and the settled decisions. The decision wins in each case.

- **COLMOD.** One sweep proposed replacing CMD_1's COLMOD 05 with 55 or dropping it. Decision: send the table byte-for-byte. The two values differ only in the unused RGB-interface field [I].
- **CMD_1's SLPOUT.** One sweep said the 10 ms IDF reset is enough after a deep-sleep wake, and a fact-check called CMD_1's SLPOUT redundant. The knowledge base corrected this, and the decision keeps CMD_1's SLPOUT + 120 ms.
- **GPIO38 hold.** Several sweeps proposed holding GPIO38 LOW with `gpio_deep_sleep_hold_en()`. Decision: no hold, because the AW9364 EN pull-down keeps it off (verified in the datasheet).
- **Byte swap.** Several sweeps proposed esp_lvgl_port `swap_bytes = 1`. Decision: i80 `swap_color_bytes = 1` and `swap_bytes = 0`.
- **PSRAM.** One sweep proposed disabling PSRAM to speed up wakes. Decision: octal PSRAM with the memtest off and no XIP from PSRAM.
- **Charge current.** Several sources proposed reworking R13 or using a larger cell. Decision: keep R13 = 2 kΩ; the owner accepted about 2.1C (ADR-0010).
- **USB test.** Some sources used "VSYS > 4.2-4.35 V" as the USB test. Decision: GPIO15 LOW, then GPIO4 ~0 V vs ~2.3 V. The VSYS threshold is a secondary hint only.
- **Verification fixes.** One sweep swapped the D1/D2 roles and put panel VDD on CN1 pins 2-3. Verification against the schematic: D2 is the VBUS diode, D1 the GPIO15 diode, and only CN1-2 is VDD3V3.
