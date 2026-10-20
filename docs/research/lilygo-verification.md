# LilyGO deep dive: verification of the top claims

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): an adversarial verifier re-checked the 24 top claims from the seven sweeps against the schematic, the datasheets, the upstream sources and the issue threads

Status: verified: 19 confirmed / 5 partially / 0 refuted

Superseded by decision: CMD_1 is sent byte-for-byte, including its leading SLPOUT + 120 ms (ADR-0006). The idf:K04 correction and the verifier's note that dropping that SLPOUT saves about 120 ms per wake were overruled by the knowledge base [C]: IDF's own SLPOUT comes 10 ms after reset, inside the datasheet's 120 ms window, so CMD_1's SLPOUT is the safety margin ([t-display-s3.md](../hardware/t-display-s3.md#panel-timing-rules)).

Each claim is quoted from its sweep file (lilygo-sweep-<sweep>.md, item ID after the colon). Most confirmations add nuance; read the correction field as well as the verdict.

## Summary

| Claim | Topic | Verdict |
|---|---|---|
| `display:K01` | ST7789 init: CMD_1 table decoded | partially |
| `display:K07` | ESP-IDF 5.0+ ST7789 init no longer turns the display on | confirmed |
| `display:K10` | LilyGO bring-up order | partially |
| `display:K11` | Pixel clock values and history | confirmed |
| `display:K13` | Orientation, gap and inversion | confirmed |
| `display:K23` | AW9364 wiring | confirmed |
| `display:K25` | LilyGO's pulse-count setBrightness | confirmed |
| `display:K29` | V3V is forced on by USB (VBUS diode-OR) | confirmed |
| `display:K31` | LilyGO display sleep sequences | confirmed |
| `display:K33` | Sleep current: panel vs whole board | confirmed |
| `power:K01` | Battery ADC topology (GPIO4) | confirmed |
| `power:K09` | Charge current vs 250 mAh cell | confirmed |
| `power:K13` | Official sleep sequences and a hardened order | confirmed |
| `power:K18` | AW9364 pulse algorithm details | confirmed |
| `apps:K02` | Energy budget numbers | confirmed |
| `apps:K04` | USB presence detection | confirmed |
| `idf:K01` | Repo role and freshness | partially |
| `idf:K04` | CMD_1 init table: content and how it is sent | confirmed |
| `idf:K06` | Fixed delays and the SLPIN API in IDF 6.1 | confirmed |
| `idf:K11` | PSRAM draw buffers vs pixel clock | confirmed |
| `issues-a:I176-12` | power / PWR_EN forced by VBUS | confirmed |
| `issues-a:I176-14` | power / permanent battery drains | confirmed |
| `issues-a:I176-29` | battery ADC | partially |
| `issues-b:K01` | power path topology | partially |

## display:K01: ST7789 init: CMD_1 table decoded

**Claim (lilygo-sweep-display.md):** LilyGO's vendor table decodes, per the bundled ST7789V v1.4 datasheet, as follows. 11h SLPOUT then 120 ms. 3Ah 05: 16 bpp on the MCU interface. B2h 0B 0B 00 33 33: back and front porch 11/11 lines (default 0C/0C), separate porch control off, idle and partial porches 3/3. B7h 75: VGH 14.97 V, VGL −10.43 V (default 35h gives 13.26/−10.43). BBh 28: VCOM 1.10 V (default 20h = 0.90 V). C0h 2C: LCMCTRL power-on default, so a no-op. C2h 01: VDV/VRH taken from registers (CMDEN=1). C3h 1F: GVDD 5.10 V + offsets (default 0Bh = 4.10 V). C6h 13: dot inversion, RTNA 13h ≈53 Hz (default 0Fh = 60 Hz). D0h A4 A1: AVDD 6.8 V, AVCL −4.8 V, VDS 2.3 V (default A4 81 has AVCL −4.4 V). E0h/E1h: 14-byte vendor positive and negative gamma. The table contains no MADCTL, INVON, RAMCTRL, CASET/RASET or DISPON, so it does not disturb esp_lcd's orientation, inversion or endianness settings.

**Verdict:** partially

**Correction:** The decode skips two of the 14 entries.
- 'D0 A7' (one byte): PWCTRL1's first parameter is documented as fixed A4h, so this write is off-spec. The next 'D0 A4 A1' sets the real values.
- 'D6 A1': D6h has no section anywhere in the v1.4 datasheet, so it is an undocumented vendor register.

Also, C2h 01 equals the documented power-on default (01h/FFh), so like C0h 2C it is effectively a no-op.

**Evidence:** All 8 copies of lcd_st7789v[] are the same 14-entry table, including {0xD0,{0xA7},1} and {0xD6,{0xA1},1}: factory.ino:133-149, lv_demos.ino:74-90, Sleep, nes, tft, touch_test, usb_hid_pad, and LilyGo-Display-IDF display_s3.c:32-47 (normalized comparison). Every register the claim decodes checks out in the bundled ST7789V v1.4 datasheet:
- SLPOUT p.184.
- COLMOD p.224: 05 gives D2-D0=101, 16 bpp on the control interface.
- PORCTRL p.263: default 0C/0C/00/33/33.
- GCTRL p.268: VGHS 7 = 14.97 V, VGLS 5 = -10.43 V; default 35h on p.269.
- VCOMS p.273: 28h = 1.1 V; default 20h on p.274.
- LCMCTRL p.275: default 2Ch.
- VDVVRHEN p.277: CMDEN; default 01h/FFh.
- VRHS p.278: 1Fh = 5.1 V + offsets; default 0Bh on p.279.
- FRCTRL2 p.284: NLA=0 is dot inversion, 13h = 53 Hz. The formula with 0B/0B porch gives 52.8 Hz. Default 0Fh on p.285.
- PWCTRL1 p.290: A1 gives AVDD 6.8 V, AVCL -4.8 V, VDS 2.3 V; default A4/81 on p.291.
- E0h takes 14 parameters (p.294).

The table has no 36h/21h/B0h/2Ah/2Bh/29h. The #229 lewisxhe 2024-03-05 quote is verbatim.

## display:K07: ESP-IDF 5.0+ ST7789 init no longer turns the display on

**Claim (lilygo-sweep-display.md):** In IDF v4.4.x, panel_st7789_init ends with DISPON; from v5.0 it does not, so esp_lcd_panel_disp_on_off(panel, true) is mandatory. This is the display part of commit c9b939f ('Fixed ... display issues in Arduino esp32-core v3.3.11'), which added disp_on_off before the CMD_1 loop. lv_demos.ino still never calls it and is guarded by '#error ... please use a version below Arduino ESP32 3.0'.

**Verdict:** confirmed

**Correction:** Nuance: a1e3c39 (2026-03-09, 'Remove high version warnings') deleted the same #error guard from 14 other examples. lv_demos is the only one that keeps it.

**Evidence:**

- IDF v4.4.6 panel_st7789_init: SLPOUT, 100 ms, MADCTL, COLMOD, then LCD_CMD_DISPON ('turn on display').
- IDF v5.0: init ends after COLMOD.
- IDF v6.1 (esp_lcd_panel_st7789.c:182-201): SLPOUT + 100 ms, MADCTL, COLMOD, RAMCTRL, no DISPON.
- c9b939f (2026-07-28) inserts esp_lcd_panel_disp_on_off(panel_handle, true) directly before the '#if defined(LCD_MODULE_CMD_1)' loop (factory.ino:324 @1cc41eb). Before that commit the factory had neither the call nor a version guard.
- lv_demos.ino:3-5 has '#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0) #error ... below Arduino ESP32 3.0', and lines 202-222 never call disp_on_off.

## display:K10: LilyGO bring-up order

**Claim (lilygo-sweep-display.md):** Factory (2026-07): GPIO15 HIGH → RD (GPIO9) output HIGH → i80 bus (LCD_CLK_SRC_PLL160M) → io (CS6, queue 20, dc_levels 0/0/0/1, 8-bit cmd/param) → st7789 (RST5, RGB, 16 bpp) → reset → init → invert(true) → swap_xy(true) → mirror(false,true) → set_gap(0,35) → disp_on_off(true) → CMD_1 → LVGL → backlight fade. LilyGo-Display-IDF: the same up to gap → CMD_1 → disp_on_off(true) → backlight GPIO HIGH before LVGL has drawn anything. Neither clears GRAM before showing it.

**Verdict:** partially

**Correction:** The ordering point stands, but the IDF repo is not 'the same up to gap' in detail:
- GPIO15 is raised in power_driver_init() (main.cpp:135, before touch_init), not next to the display init.
- The bus uses LCD_CLK_SRC_DEFAULT, which is PLL_F160M on S3 in v5.3 and v6.1, so the clock is the same.
- trans_queue_depth is 10 (factory 20).
- max_transfer_bytes is 34,000 with psram/sram_trans_align 64/4, and there is no user_ctx.

New evidence: the maintainer recommends the opposite of LilyGO's own code. #300, lewisxhe 2025-03-10, comment 2709985067: 'After the screen is initialized, refresh the screen to all black, and then turn on the backlight last.' Espressif's v6.1 i80 example also leaves a stub to flush a pattern before DISPON and backlight. In the factory, the backlight is LOW at fade step 0 and first lights at level 1/16 after one lv_timer_handler(), so stale GRAM can only show dimly and briefly. The IDF repo turns the backlight fully on before lv_init.

**Evidence:** The factory side matches exactly (factory.ino:252-378 @1cc41eb): GPIO15 HIGH, RD output HIGH, PLL160M bus, CS6 / queue 20 / dc 0,0,0,1 / 8-bit cmd and param, RST5 RGB 16 bpp, reset, init, invert, swap, mirror(false,true), gap(0,35), disp_on_off, CMD_1, LVGL, fade. In LilyGo-Display-IDF, display_s3.c:121-153 runs reset, init, invert, swap, mirror, gap, CMD_1, disp_on_off(true), backlight GPIO HIGH, and main.cpp:143-147 calls display_init() before lv_init(). Neither path writes GRAM before DISPON.

## display:K11: Pixel clock values and history

**Claim (lilygo-sweep-display.md):** Factory pin_config: 6.528 MHz (2022-07-01, commented '//(10 * 1000 * 1000)') → 20 MHz (2022-12-08) → 16 MHz (2023-03-03) with the comment 'Too low or too high pixel clock may cause screen mosaic'. lv_demos still uses 6.528 MHz, which is exactly 60 full frames per second (320×170×2 bytes over an 8-bit bus). LilyGo-Display-IDF uses 10 MHz. The nes example's Arduino_ESP32LCD8 bus defaults to 40 MHz. The MicroPython lvgl binding (#243) uses 10 MHz.

**Verdict:** confirmed

**Correction:** Nuance (new): ST7789V v1.4 Table 4 (p.41) specifies the 8080 write cycle TWC >= 66 ns (TWRL/TWRH >= 15 ns), which is about 15 MHz max. LilyGO's 16 MHz (62.5 ns), its earlier 20 MHz and nes's 40 MHz all exceed the datasheet. 10 MHz (100 ns) and 6.528 MHz are within spec.

**Evidence:**

- 758b2a3 (2022-07-01): '(6528000) //(10 * 1000 * 1000)'.
- 58bd641 (2022-12-08): 20 MHz.
- adc038e (2023-03-03): 16 MHz plus the 'Too low or too high pixel clock may cause screen mosaic' comment (examples/factory/pin_config.h:24-25).
- lv_demos/pin_config.h:14 is still 6528000. 320*170*2 = 108,800 B, and x60 = 6,528,000 exactly.
- LilyGo-Display-IDF display_s3.c:23 is 10 MHz.
- Arduino_ESP32LCD8.cpp:45-54 sets 40000000 when speed is GFX_NOT_DEFINED, and nes.ino:73 calls gfx->begin() with no speed. The class writes the LCD_CAM clock registers directly from the 240 MHz source and only compiles for Arduino core < 3 (bundled Arduino_GFX 1.4.2).
- #243 user code: _FREQ = const(10000000).

## display:K13: Orientation, gap and inversion

**Claim (lilygo-sweep-display.md):** esp_lcd settings: invert_color(true) and swap_xy(true). mirror(false,true) means 'The screen faces you, and the USB is on the left'; mirror(true,false) means 'the USB is to the right'. set_gap(0,35) applies to both, because 170 visible columns are centred in the 240-column GRAM. TFT_eSPI: rotation 1 = MX|MV, rotation 3 = MV|MY, CGRAM rowstart 35 in landscape and colstart 35 in portrait. Arduino_GFX uses offsets 35/0/35/0. The LovyanGFX MIDI examples use offset_x 35, offset_rotation 1, invert true, then setRotation(2). Every path enables inversion (IPS panel).

**Verdict:** confirmed

**Correction:** Nuance: the LovyanGFX configs belong to Saulo Verissimo's community music examples from 2026-02/03 (Gingoduino, Queue, Piano, Piano-Debug, BLE-Receiver/Sender, USB-Device). The T-Display-S3-MIDI sketch itself uses TFT_eSPI setRotation(3). Those LovyanGFX examples still drive GPIO38 with lgfx::Light_PWM at 22 kHz on pwm_channel 7 (Piano calls setBrightness(255)), and 02f0e9d did not touch them.

**Evidence:**

- factory.ino:310-322: the two USB-left/right comments were added by 01a297f (2023-06-01). LilyGo-Display-IDF display_s3.c:125-137 is identical.
- TFT_eSPI ST7789_Rotation.h: lines 23-27 portrait colstart 35 for width 170; 57-61 landscape rowstart 35; 68 MX|MV; 124-128 rowstart 35; 135 MV|MY. Setup206 defines CGRAM_OFFSET and TFT_INVERSION_ON.
- Arduino_GFXDemo.ino:16: IPS true, offsets 35/0/35/0.
- T-Display-S3-Gingoduino ST7789_Handler.h:58-61: offset_rotation 1, offset_x 35, invert true; ST7789_Handler.cpp:17 setRotation(2).

esp_lcd swap + mirror(false,true) gives MV|MY, which equals TFT_eSPI rotation 3 as used by the Sleep example. The 35-column window is centred (35+170+35 = 240), so gap (0,35) holds for either mirror.

## display:K23: AW9364 wiring

**Claim (lilygo-sweep-display.md):** U4 AW9364DNR: EN = ESP_LCD_BLK (GPIO38, direct, no external resistor); VIN = V3V with 1 µF; LED1–4 sink pins go to LEDK1–4 (FPC pins 21–24). The LED anodes are on FPC pin 20 = V3V. The 2023-03-13 and 2024-04-29 schematic revisions are identical here.

**Verdict:** confirmed

**Correction:** Nuance (new): the AW9364 datasheet (Awinic, Feb 2024 V2.4) pin table describes EN as 'Active high, with an internal 150kΩ pull-down'. A floating GPIO38 (during boot, or in deep sleep without a hold) therefore keeps the backlight off.

**Evidence:** 600-dpi crops of the schematic at b0a5ecd (4/29/2024) and 811c949 (3/13/2023) are identical:
- U4 AW9364DNR pin 2 EN = ESP_LCD_BLK with no series or pull resistor.
- Pin 3 VIN = V3V with CW1 1uF/6.3V.
- Pins 8/7/6/5 LED1-4 go to LEDK1-4.
- PGND, AGND and EP go to GND.
- CN1 pin 20 = V3V (C20 1uF) and pins 21-24 = LEDK1-4.

## display:K25: LilyGO's pulse-count setBrightness

**Claim (lilygo-sweep-display.md):** Added to the factory on 2024-09-23 after #278. 16 levels. Value 0: BL LOW then delay(3) (shutdown). From off: BL HIGH, delayMicroseconds(30), level = 16. Then (16 + level − value) % 16 LOW→HIGH pulses via back-to-back digitalWrite, with no explicit pulse width. Code comment: 'The adjustable range is 0~16, 0 is the minimum brightness, 16 is the maximum'. The factory fades 0→16 at 50 ms per step while LVGL renders a GIF.

**Verdict:** confirmed

**Correction:** Nuance (new):
- #278 was triggered by tone(), which uses LEDC, blanking the screen: a channel clash with the old LEDC backlight. The factory still plays tone(10,1000,5000) at line 385 as a regression check.
- AW9364 datasheet timing: TON > 20 µs (LilyGO waits 30 µs); THI > 0.5 µs; 0.5 µs < TLO < 500 µs; shutdown > 2.5 ms (LilyGO waits 3 ms).
- Each EN rising edge lowers the current one step (20 mA down to 1.25 mA in 1.25 mA steps) and wraps back to 16/16.
- The pulse width of back-to-back digitalWrite is unspecified and depends on the Arduino core.

**Evidence:**

- factory.ino:223-248 and 371-378 @1cc41eb match the claim exactly. num = (steps+to-from)%steps, which equals (16+level-value)%16.
- 2d3c294 (2024-09-23) adds setBrightness and removes the ledcSetup/ledcAttachPin fade.
- #278 lewisxhe 2024-09-23 (comment 2367079609) quote is verbatim.

## display:K29: V3V is forced on by USB (VBUS diode-OR)

**Claim (lilygo-sweep-display.md):** Q1 (PMBT3904) is driven through a 1N4148 diode-OR (D2) of VBUS and PWR_EN (GPIO15), with R5 10 k and R7 100 k pull-down. Q1 switches Q3 (SI2301, VDD3V3→V3V) and Q2 (the battery-ADC net). Whenever USB is present, V3V is on regardless of GPIO15: the backlight LED anodes, AW9364 VIN and the green LED (2 kΩ, ≈0.6 mA). On battery only GPIO15 enables it. The README says 'In USB mode, the green indicator stays on'; #284 reports the TFT going black only after USB is unplugged. The 2023 schematic has the same circuit.

**Verdict:** confirmed

**Correction:** Nuance: D2 is only the VBUS diode; the PWR_EN diode is D1. The 0.6 mA LED figure assumes Vf of about 2.1 V. The schematic gives no LED Vf, and a 2.8 V InGaN green would draw about 0.25 mA.

**Evidence:** Crops @b0a5ecd:
- VBUS through D2 and PWR_EN through D1 (both 1N4148) join at one node, then R5 10K to the Q1 PMBT3904 base, with R7 100K to GND.
- Q1's collector goes through R6 1K to the gate shared by Q2 (VSYS to BAT) and Q3 (VDD3V3 to V3V), both SI2301, with R8 100K to VSYS.
- V3V feeds a 2K resistor (N5) and LED8 (green) to GND.
- PWR_EN is U2 pin 21 = GPIO15 (XTAL_32K_P).

811c949 (3/13/2023) has the same switch, LED branch and AW9364 wiring. The dashed box around D1/D2/R7 on the 2024 sheet is only a highlight. README.md:253-256 and #284 (TFT black only after USB is unplugged; GPIO4 reads 0 until GPIO15 HIGH) are verified.

## display:K31: LilyGO display sleep sequences

**Claim (lilygo-sweep-display.md):** Factory button1: tx_param(0x10 SLPIN) → GPIO15 LOW → GPIO38 LOW → ext0 wake on GPIO14 low → esp_deep_sleep_start(); no DISPOFF, no delay. Sleep example: fillScreen black, draw 'Sleep', 1 s, GPIO15 LOW, GPIO38 LOW, writecommand(0x10), deep sleep; it also has a 15 s timer-wake loop added for #300. No LilyGO code sends DISPOFF (28h). For comparison, Arduino_GFX displayOff() is SLPIN + 120 ms, and IDF disp_sleep is SLPIN + 100 ms.

**Verdict:** confirmed

**Correction:** Nuance (new, from #300): the reporter's matte-screen units showed white edges and ghosting after minutes of deep sleep on battery. That panel was never sent SLPIN, because their s3lcd MicroPython library has no sleep command. Their fix (2025-03-24) was to drive all LCD pins (5-9, 15, 38-48) LOW before deep sleep, which also holds RESX low. ST7789V p.182: SLPIN stops the DC/DC converter, oscillator and scanning, and GRAM is kept. The panel needs 5 ms after SLPIN before the next command, and 120 ms after SLPOUT before an SLPIN.

**Evidence:**

- factory.ino:395-404: the SLPIN line comes from c9fd58a (2024-11-27, #289); the rest is from 84e8604 (2023-03-21).
- Sleep.ino:77-89 (4b2edc1) and 93-117 (fc466c6, 2025-03-19, #300) run in exactly the stated order.
- A grep of all examples finds no DISPOFF, disp_on_off(false), displayOff or LCD_CMD_DISPOFF.
- Arduino_ST7789.cpp:90-94 with ST7789_SLPIN_DELAY 120 (Arduino_GFX 1.4.2).
- IDF v6.1 panel_st7789_sleep sends SLPIN, then vTaskDelay(100 ms).

## display:K33: Sleep current: panel vs whole board

**Claim (lilygo-sweep-display.md):** The maintainer's bench photo for the Sleep example shows 0.243 mA DC for the whole board. Datasheet: Normal-mode IDD 6.0 mA typ (7.5 max); Sleep-In IDD 15 µA typ (30 max) plus IDDI 5 µA. This matches #289 (6.42 mA without SLPIN, 263 µA with it). In #33 a user found DISPOFF + backlight off still drew ~30 mA more than a never-initialised display.

**Verdict:** confirmed

**Correction:** Nuance: the photo shows neither the supply voltage nor which input was used. The ~30 mA in #33 is about five times the panel's own 6 mA typical, so other activity must account for most of it. DISPOFF alone does not stop the panel's DC/DC converter or scanning; only SLPIN does.

**Evidence:**

- examples/Sleep/IMAGE.jpg (added in 4b2edc1): a VICTOR 8246A meter reads 'DC 00.243 mA' with the board on its leads.
- Datasheet p.40 Table 3 (60 Hz, IC default registers): Normal mode IDD 6.0/7.5 mA typ/max; Sleep-in IDD 0.015/0.03 mA; IDDI 0.005/0.01 mA.
- #289: 6.42 mA vs 263 µA, a difference of about 6.2 mA, which matches the panel's normal-mode IDD.
- #33, wichjan 2023-06-21: about 30 mA more with DISPOFF and the backlight off, verbatim.

## power:K01: Battery ADC topology (GPIO4)

**Claim (lilygo-sweep-power.md):** GPIO4 (ADC1_CH3) reads a 100 kΩ/100 kΩ 1% divider (R2/R4, C13 10 nF) whose top is net BAT, not the cell net VBAT. BAT is the system rail VSYS ('+5V') switched by P-FET Q2 (SI2301). Q2 and the V3V switch Q3 share one gate node (R8 100 kΩ to VSYS, pulled low via R6 1 kΩ by NPN Q1 PMBT3904); Q1's base is diode-ORed from VBUS (D2 1N4148) and PWR_EN/GPIO15 (D1) through R5 10 kΩ with R7 100 kΩ to GND. Result: battery only + GPIO15 HIGH -> (VBAT - I*(R10 fuse + Q4))/2; battery only + GPIO15 LOW -> 0 V; USB present -> (VBUS - D3 drop)/2 regardless of GPIO15.

**Verdict:** confirmed

**Evidence:** Crops @b0a5ecd:
- ADC block: R2 and R4 marked '100K%1', C13 10nF across R4. The top is net BAT; the midpoint BAT_ADC goes to U2 pin 9 = GPIO4 (ADC1_CH3).
- Q2 SI2301 switches VSYS to BAT and shares its gate node with Q3 (R8 100K to VSYS, R6 1K to Q1's collector).
- Q1's base is fed through R5 10K from the D1/D2 1N4148 OR, with R7 100K to GND.
- VSYS comes from VBAT through Q4 SI2307 and the R10 fuse, or from VBUS through D3 1N5819.

Issues: the #190 krupis 2023-10-13 quote ('just measuring the supply voltage') and its ADC_UNIT_1/ADC_CHANNEL_3 code are verified. #284 (GPIO4 reads 0 on battery until GPIO15 is HIGH) is verified.

## power:K09: Charge current vs 250 mAh cell

**Claim (lilygo-sweep-power.md):** TP4065 (SOT-23-5, marking 65b...) programs IBAT via R13 = 2 kΩ: datasheet formula one RPROG = 1050/IBAT (IBAT > 0.1 A) gives ~525 mA; at 2.1 kΩ the spec is 425/500/575 mA min/typ/max; above ~0.4 A thermal regulation lowers the current. The schematic annotation reads 'R13 IBAT (mA) = 580' (a TP4056-table value) and README FAQ 8 says 500 mA. Termination at C/10, float 4.2 V ±1%, auto-recharge at ~80-90% capacity. For 250 mAh that is ~2.1C (up to ~2.3C). R13's location is in the maintainer's PCB image (#230).

**Verdict:** confirmed

**Evidence:** TP4065 REV_2.0, re-downloaded from the cited URL:
- 'Formula one: RPROG = 1050/IBAT (IBAT>0.1A)', giving 525 mA at 2K.
- The electrical table gives 425/500/575 mA at RPROG = 2.1K.
- 'larger than 0.4A ... temperature protection will reduce the charging current'.
- C/10 termination (ITERM 40/50/55 mA at 2.1K); VFLOAT 4.158/4.2/4.242 V.
- Recharge point 'roughly corresponds to 80 to 90 percent of the battery capacity'.
- SOT23-5, marking 65bYWW.

The charger block @b0a5ecd shows U6 TP4065, R13 2K, the note 'R13 IBAT (mA) = 580' and the R10 '5V/2A' fuse. README.md:238 ('500mA', blame 6fe4d51) and #230 comment 1977898759 (photo of R13's location; the user read the marking as 65b311) are verified. On 250 mAh this is 1.7-2.3C (2.1C nominal).

## power:K13: Official sleep sequences and a hardened order

**Claim (lilygo-sweep-power.md):** factory: SLPIN (0x10 via esp_lcd_panel_io_tx_param) -> GPIO15 LOW -> BL LOW -> ext0 GPIO14 LOW -> esp_deep_sleep_start. Sleep example: GPIO15 LOW -> BL LOW -> SLPIN -> ext0 GPIO14 or a 15 s timer -> deep sleep, followed by full re-init in setup() after wake. Neither waits 5 ms after SLPIN, holds any pin, or stops Wi-Fi; touch_test only holds the touch reset pin (gpio_hold_dis at boot, gpio_hold_en before sleep). No example uses light sleep or esp_pm. IDF v6 docs: on ESP32-S3 gpio_hold_en does not keep digital pads (GPIO22+) in deep sleep without gpio_deep_sleep_hold_en; rtc_gpio_isolate minimizes leakage on RTC pads.

**Verdict:** confirmed

**Correction:** Nuance: see display:K31 for #300's ghosting root cause (no SLPIN, undefined LCD pins during deep sleep). GPIO38 needs no hold because AW9364 EN has an internal 150 kΩ pull-down.

**Evidence:**

- factory.ino:395-404 and Sleep.ino:77-89 / 93-117 run in the stated orders. Neither waits after SLPIN, holds a pin, or stops Wi-Fi.
- touch_test.ino:66 gpio_hold_dis and :152 gpio_hold_en(PIN_TOUCH_RES) (blame 9ceb658). Its sleep path never sends SLPIN to the panel.
- A grep of examples finds no esp_light_sleep_start, esp_pm_*, setCpuFrequencyMhz, WiFi off or rtc_gpio_isolate.
- IDF v6.1 components/esp_driver_gpio/include/driver/gpio.h:437-439: 'For ESP32/S2/C3/S3/C2, this function cannot be used to hold the state of a digital GPIO during Deep-sleep ... please call gpio_deep_sleep_hold_en'.
- IDF v6.1 rtc_io.h: rtc_gpio_isolate is meant 'to minimize leakage current'.
- S3 RTC pads are GPIO0-21, so GPIO5-9 and GPIO15 can be held, while GPIO38-48 need gpio_deep_sleep_hold_en.

## power:K18: AW9364 pulse algorithm details

**Claim (lilygo-sweep-power.md):** factory setBrightness(0..16): 0 = EN LOW + delay(3) ms; from off, EN HIGH + 30 µs and the chip starts at max (16); then (16 + (16-target) - (16-current)) % 16 LOW->HIGH pulses, each one step down with wrap 1->16 (going 1->2 takes 15 pulses). Pulses are back-to-back digitalWrite calls with no explicit delay. It was added on 2024-09-23 for #278 (maintainer: 'The method of ledc controlling the backlight of display-S3 is wrong'), 18 months before the LEDC clean-up in 02f0e9d. The boot ramp steps 0..16 every 50 ms.

**Verdict:** confirmed

**Correction:** Minor: 2024-09-23 to 02f0e9d (2026-03-09) is about 17.5 months, not 18.

**Evidence:**

- factory.ino:223-248 (2d3c294, 2024-09-23): from = 16-level, to = 16-value, num = (16+to-from)%16. Going from level 1 to 2 needs 15 pulses.
- The AW9364 datasheet confirms each rising edge drops one step, wrapping from 1/16 back to 16/16.
- #278 comment 2367079609 quote is verbatim.
- The boot ramp is i = 0..16 with lv_timer_handler() and delay(50) per step.

## apps:K02: Energy budget numbers

**Claim (lilygo-sweep-apps.md):** Reported currents: 147.7 mA awake with screen on (user, ESP-IDF code, same thread as the known 6.42 mA/263 uA sleep numbers); maintainer: average current with Wi-Fi on is about 150 mA+ and the heat centre should be the ESP32-S3.

**Verdict:** confirmed

**Correction:** Nuance: the 147.7 mA figure is a code comment; its Wi-Fi state and brightness are not stated. Inference: at 16/16 the backlight alone is about 4 x 20 mA = 80 mA (AW9364 datasheet), likely about half of that figure. The #295 reporter measured 4.62 V on the 5V/VSYS pin on USB.

**Evidence:**

- #289 (BootsSiR 2024-11-26) code comment: '// draws 6.42mA asleep and 147.7mA awake'. The code uses ESP-IDF-style gpio_* and ESP_LOGI calls.
- #295 lewisxhe 2024-12-19 (comment 2552567520): 'When WiFi is turned on normally, the average current is about 150mA+, and the heat center is in the ESP32 chip.'

## apps:K04: USB presence detection

**Claim (lilygo-sweep-apps.md):** There is no digital VBUS-detect line. Maintainer: detecting USB cable connect/disconnect without the ADC 'is not possible with current hardware'; USB is inferred only from the battery ADC reading above 4.2 V (charging state cannot be read; only the red LED shows it).

**Verdict:** confirmed

**Correction:** Nuance (new):
- IDF v6.1 usb_serial_jtag_is_connected() reports a USB host via SOF packets while awake. Its header says a power bank 'will never be considered as connected'.
- Because VBUS also turns Q1/Q2 on, GPIO4 reads about 2.3 V with USB even when GPIO15 is LOW, and 0 V on battery with GPIO15 LOW. That is a cleaner test than the 4.2 V threshold, but GPIO15 LOW also cuts V3V and the backlight.

**Evidence:**

- #292 lewisxhe 2024-12-04: 'This is not possible with current hardware.'
- #254 lewisxhe 2024-06-03 quote is verbatim.
- #260 has only a community reply (teastainGit) suggesting watching the GPIO4 jump.
- In the schematic, VBUS reaches only D2, D3, TP4065 VCC, the R9/red LED, the C21/C22 caps and the Q4 gate/R14. No GPIO senses it.

## idf:K01: Repo role and freshness

**Claim (lilygo-sweep-idf.md):** LilyGo-Display-IDF (MIT, 15 commits 2024-01-31..2025-11-03, 70 stars, 19 forks, 9 open issues) is the IDF reference LilyGo points T-Display-S3 users to, but every T-Display-S3 file (display_s3.c, main.cpp, tft_driver.h, i2c_driver.c, touch_driver.cpp, sdkconfig.defaults.t-display-s3, the S3 block of product_pins.h) is unchanged since the first commit; later commits only added T-Watch S3/2019, a T4-S3 offset, a T-RGB touch fix, XPowersLib bumps and setup.py fixes. README targets IDF v5.3.0 (then the master branch, per #5) on Ubuntu 22.04; LVGL comes from add-dependency lvgl/lvgl^8.3.11. Maintainer stance: "I can't do more, all I can do is demonstrate how to use IDF" (#3); the Component Registry BSP request (#16) is unanswered. The ESP Component Registry has no T-Display-S3 BSP; the only LilyGo-engineer component (llgok = 'LILYGO_L', 24 commits in T-Display-S3), llgok/lilygo_device_driver 2.1.0, is GPL-3.0, needs IDF>=5.5.3 and only enables T-Display-P4.

**Verdict:** partially

**Correction:** llgok publishes 6 registry components, not one: cpp_bus_driver 2.1.0, lilygo_device_driver 2.1.0, icm20948_we, kode_bq25896, arduino_cpp_bus_driver and radiolib_cpp_bus_driver. lilygo_device_driver 2.1.0 (GPL-3.0, idf >=5.5.3) offers four Kconfig devices: T-Glasses-P4 (the default), T-Display-P4-Air, T-Spe and T-Display-P4. Only T-Display-P4 pulls the extra llgok dependencies, and none targets the T-Display-S3, so the conclusion holds. The README pointer to LilyGo-Display-IDF dates from 7cf26a4 (2024-01-31); 6fe4d51 only rewrote it. Later commits also include 09b8f14 (T-Display-S3-AMOLED define) and 8e1623e (README typo).

**Evidence:**

- gh api: MIT, 70 stars, 19 forks, 9 open issues (all issues, no PRs); 15 commits from 2024-01-31 to 2025-11-03.
- Per-file logs: display_s3.c, main.cpp, tft_driver.h, i2c_driver.c, touch_driver.cpp and sdkconfig.defaults.t-display-s3 show only c465092. The product_pins.h edits (a716e0f, 6fe03b1) only append new board blocks, and power_driver.cpp's GPIO15 path is unchanged since c465092.
- README.md:57-58: Ubuntu 22.04, IDF V5.3.0. setup.py:182: add-dependency lvgl/lvgl^8.3.11.
- #3 comment 1953724257 quote is verbatim. #5 (2024-03-16 and 03-20) says 'master branch'. #16 has no comments.
- Registry searches find no T-Display-S3 BSP; espp only has t-deck, t-dongle-s3 and t_keyboard.
- users/Llgok name is 'LILYGO_L', with 24 commits in T-Display-S3.

## idf:K04: CMD_1 init table: content and how it is sent

**Claim (lilygo-sweep-idf.md):** display_s3.c lcd_st7789v[] is byte-identical to T-Display-S3 factory.ino and lv_demos.ino: 0x11 SLPOUT (+120 ms), 3A=05, B2=0B 0B 00 33 33 (porch), B7=75, BB=28 (VCOM), C0=2C, C2=01, C3=1F, C6=13 (frame rate), D0=A7, D0=A4 A1, D6=A1, E0/E1 14-byte gamma. Bit7 of len means wait 120 ms. The IDF repo sends it unconditionally (no LCD_MODULE_CMD_1 switch) with esp_lcd_panel_io_tx_param after reset -> init -> invert(true) -> swap_xy(true) -> mirror(false,true) -> set_gap(0,35), then esp_lcd_panel_disp_on_off(true). The table repeats SLPOUT and rewrites COLMOD to 0x05 after the driver's SLPOUT(+100 ms)/COLMOD 0x55, and never touches MADCTL 0x36 or RAMCTRL 0xB0, so driver-managed orientation and endianness survive. On IDF 6.1, tx_param is synchronous, first drains in-flight colour DMA, and is limited to 32 parameter bytes.

**Verdict:** confirmed

**Correction:** Nuance (new): datasheet p.184 says SLPOUT 'has no effect when module is already in sleep out mode'. CMD_1's leading 11h and its 120 ms are therefore redundant after esp_lcd_panel_init(). Dropping them would save about 120 ms per wake (inference, needs a hardware test). The drain covers only that IO device's queue, which is the single panel here.

**Evidence:**

- A normalized comparison shows all 8 table copies are identical.
- display_s3.c:139-145 sends the table with no #if: len & 0x7f is the length, bit 0x80 triggers vTaskDelay(120). Then it calls disp_on_off(true).
- IDF v6.1 st7789: COLMOD 0x55 for 16 bpp (line 99); init at lines 182-201 sends SLPOUT, 100 ms, MADCTL, COLMOD, then RAMCTRL B0 00 F0.
- esp_lcd_panel_io_i80.c:487-559: param_size must be <= max_transfer_bytes and <= LCD_I80_IO_FORMAT_BUF_SIZE (esp_lcd_common.h:23 = 32). It drains the device's num_trans_inflight, then busy-polls TRANS_DONE.

## idf:K06: Fixed delays and the SLPIN API in IDF 6.1

**Claim (lilygo-sweep-idf.md):** Cold bring-up waits: esp_lcd_panel_reset() 10 ms low + 10 ms high; esp_lcd_panel_init() SLPOUT + 100 ms; CMD_1's SLPOUT + 120 ms. That is about 240 ms before the first pixel. esp_lcd_panel_disp_sleep(panel,true) sends SLPIN and always waits 100 ms (false: SLPOUT + 100 ms). esp_lcd_panel_disp_on_off(false) sends DISPOFF. Without a reset GPIO the driver uses SWRESET + 20 ms. LilyGo-Display-IDF has no sleep code at all.

**Verdict:** confirmed

**Correction:** Nuance: the 100 ms in disp_sleep(true) is IDF's choice. The datasheet needs only 5 ms after SLPIN before another command.

**Evidence:**

- IDF v6.1 esp_lcd_panel_st7789.c:162-180: reset drives reset_level (active-low by default) for 10 ms, then the inverse for 10 ms. Without a reset GPIO it sends SWRESET + 20 ms.
- Lines 186-189: SLPOUT + 100 ms.
- Lines 304-317: disp_on_off sends DISPON or DISPOFF.
- Lines 319-334: sleep sends SLPIN or SLPOUT, then 100 ms in both cases.
- 10+10+100+120 = 240 ms.
- LilyGo-Display-IDF main/ has no deep sleep, light sleep, SLPIN or esp_pm code; the only 'sleep' hits are touch disableAutoSleep.

## idf:K11: PSRAM draw buffers vs pixel clock

**Claim (lilygo-sweep-idf.md):** LilyGo-Display-IDF puts both LVGL buffers (2 x 17,000 px = 2 x 34,000 B) in PSRAM and clocks the bus at 10 MHz with the default 40 MHz PSRAM. Espressif's IDF 6.1 i80_controller example defaults PCLK to 2 MHz when buffers are in PSRAM on ESP32-S3 (10 MHz otherwise). Its README warns: "Can't get a stable UI when EXAMPLE_LCD_I80_COLOR_IN_PSRAM is enabled... because of the limited PSRAM bandwidth". The suggested fixes are a lower PCLK, faster SPIRAM_SPEED, SPIRAM_XIP_FROM_PSRAM and dma_burst_size 64. LilyGo's Arduino lv_demos uses an internal DMA buffer (MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL).

**Verdict:** confirmed

**Correction:** Nuance: v6.1's sdkconfig.defaults.esp32s3 itself enables COLOR_IN_PSRAM, SPIRAM_SPEED_80M and SPIRAM_XIP_FROM_PSRAM, so on S3 the example's default PCLK is 2 MHz. The README also suggests lowering the LVGL FPS. The #4 boot log comes from an AMOLED-variant build (I2C on GPIO2/3), though the 40 MHz default applies to the LCD config too.

**Evidence:** LilyGo-Display-IDF:
- main.cpp:152-170: with CONFIG_SPIRAM, two heap_caps_malloc(DISPLAY_BUFFER_SIZE*2, MALLOC_CAP_SPIRAM) buffers.
- product_pins.h:229: 170*100 = 17,000 px. display_s3.c:23: 10 MHz; :89-91: max_transfer_bytes 34,000.
- sdkconfig.defaults.t-display-s3 sets SPIRAM=y and OCT but no SPIRAM_SPEED. Kconfig.spiram defaults to SPIRAM_SPEED_40M in both v5.3 and v6.1.

Espressif v6.1 i80_controller example:
- Kconfig.projbuild:10-13: 'default 2000000 if EXAMPLE_LCD_I80_COLOR_IN_PSRAM && IDF_TARGET_ESP32S3', else 10 MHz.
- README.md:128-130 quote is verbatim.
- main.c:79: EXAMPLE_DMA_BURST_SIZE 64; :323-330: esp_lcd_i80_alloc_draw_buffer(..., MALLOC_CAP_SPIRAM).

T-Display-S3 lv_demos.ino:225 uses MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL.

## issues-a:I176-12: power / PWR_EN forced by VBUS

**Claim (lilygo-sweep-issues-a.md):** PWR_EN is a diode-OR of VBUS (D2) and GPIO15 (D1), feeding R5 10 kΩ into Q1 (PMBT3904, with R7 100 kΩ pull-down). Q1 drives Q2 (+5V to the BAT net that feeds the ADC divider) and Q3 (VDD3V3 to V3V, which feeds the green LED, AW9364 VIN and the LCD LED anode on FPC pin 20). So on USB the rail, green LED and divider are on whatever GPIO15 does, and GPIO4 measures the +5V net (VBUS minus the Schottky drop), not the cell.

**Verdict:** confirmed

**Correction:** Nuance: PWR_EN is the GPIO15 net itself; the OR node after D1/D2 has no name.

**Evidence:**

- The 2023 and 2024 schematic crops match the description.
- #51 mmMicky 2022-11-21 (comment 1321500571) quote is verbatim, plus 1321505149: 'This label corresponds to GPIO15'.
- The 4/29/2024 sheet carries the note 'Battery power must set IO15 to HIGH'.
- CN1 pin 20 = V3V; U4 VIN = V3V.
- Q2 feeds BAT, the top of R2/R4, from VSYS ('+5V'), which on USB is VBUS minus the D3 1N5819 drop.

## issues-a:I176-14: power / permanent battery drains

**Claim (lilygo-sweep-issues-a.md):** R12 (100 kΩ) sits directly across VBAT, drawing about 37–42 µA all the time. The 3.3 V regulator (AP2112K-3.3, alternate RT9013) is always enabled (EN pulled to +5V through R11 100 kΩ). Together they set a deep-sleep floor that firmware cannot remove. LilyGO quoted about 200 µA in deep sleep for the non-touch board.

**Verdict:** confirmed

**Correction:** The DrJaymz R12 quote is dated 2022-10-11 (comment 1274421469), not 2022-10-20; his 10-20 comment is about the green LED. Quantified: R12 (about 40 µA) plus AP2112K IQ (55 µA typ, 80 max) is about 95 µA that firmware cannot remove. Whole-board measurements are 243 µA (maintainer photo) and 263 µA (#289).

**Evidence:**

- Power block (both revisions): R12 100K sits from VBAT to GND, drawing 37-42 µA at 3.7-4.2 V.
- U7 'AP2112K-3.3V(RT9013)' has EN tied to its +5V/VSYS input through R11 100K.
- #12 LilyGO 2022-10-13 (comment 1276916865): 'The T-Display-S3 without touch when entering deep sleep is about 200uA.'
- Diodes AP2112 datasheet: IQ 55 µA typ, 80 µA max.

## issues-a:I176-29: battery ADC

**Claim (lilygo-sweep-issues-a.md):** Battery sense goes from the BAT net (switched by PWR_EN) through a 100 kΩ 1 % / 100 kΩ 1 % divider with 10 nF to GPIO4: exactly ÷2, about 50 kΩ source impedance, about 0.5 ms time constant. LilyGO staff say the high-value divider makes readings inaccurate and too high, and users report ADC noise that depends on CPU load.

**Verdict:** partially

**Correction:** The staff remarks are hedged guesses, not measurements: 'I think the current of the sampling resistor is too small...' and 'I feel that the overall voltage is too high. It is possible that...'. #152 is about the ESP32 ADC in general with the user's own divider (a reply asks what resistors 'you' are using), not GPIO4's divider. The ÷2 ratio is nominal: two 1% resistors give about ±1% ratio error.

**Evidence:**

- ADC block: R2 and R4 are 100K 1% with C13 10nF across R4 and BAT at the top. That gives a 50 kΩ Thevenin source and tau = 50k x 10nF = 0.5 ms.
- #22 LilyGO 2022-10-10 formula quote, #57 mmMicky 2022-12-03 and #59 mmMicky 2022-12-12 quotes are all verbatim.
- #152 (369Martin369, 2023-05-20) got community replies only.

## issues-b:K01: power path topology

**Claim (lilygo-sweep-issues-b.md):** v1.2 power path: VBUS -> D3 1N5819 Schottky -> '+5V' (really VSYS); battery JST (P3) -> R10 '5V/2A' fuse -> VBAT -> Q4 SI2307 P-FET -> VSYS. Q4 gate is tied to the VBUS node (R14 100k to GND), so Q4 is ON on battery and OFF when USB is present (battery then isolated from the load and only charged by TP4065). VSYS -> U7 3.3 V LDO -> VDD3V3 (ESP32-S3, flash, panel logic VDD on CN1 pins 2-3, header 3V pins, touch pull-ups).

**Verdict:** partially

**Correction:** Only CN1 pin 2 is VDD3V3 (with C17 1uF and C18 100nF). CN1 pins 1, 3 and 4 tie to the GND bus, identically on the 2023 and 2024 sheets. The rest is confirmed. Extra from #270: the maintainer says DC 5 V may be fed into that pin only when no battery is connected.

**Evidence:** Power and Charger crops @b0a5ecd:
- P3 connects through R10 ('5V/2A保险丝', a fuse) to VBAT, which has C26/C27 and R12.
- VBAT goes through Q4 SI2307 to '+5V'. Its body diode points VBAT to VSYS, so the source is on the VSYS side. The gate is on the VBUS node with R14 100K to GND: on with battery only, off with USB present, and the body diode is then reverse-biased.
- D3 1N5819 runs from VBUS to +5V; +5V feeds U7 AP2112K-3.3, which makes VDD3V3.
- Title block: 'Date: 4/29/2024'.
- #270 lewisxhe 2024-08-02: '...The correct label should be VSYS, not 5V', verbatim.

## Verifier's notes

Result: 19 confirmed (most with added nuance), 5 partially, 0 refuted. Evidence files (schematic crops, datasheet text, IDF v6.1 sources) were kept in a temporary folder and are not part of this repo. The clones were not modified.

Errors and contradictions in the claim set:
1. display:K01 leaves out two table entries that idf:K04 lists: 'D0 A7' (off-spec) and 'D6 A1' (undocumented in v1.4). Copy all 14 entries byte-for-byte.
2. issues-b:K01: VDD3V3 goes only to CN1 pin 2; pins 1, 3 and 4 are GND.
3. idf:K01: llgok has 6 registry components, and lilygo_device_driver supports 4 boards (P4 family and T-Spe), none of them the T-Display-S3.
4. display:K10: LilyGo-Display-IDF's bring-up differs in details (queue depth 10, LCD_CLK_SRC_DEFAULT, GPIO15 set in power_driver_init).
5. The R12 quote in issues-a:I176-14 is dated 2022-10-11, not 2022-10-20.
6. Against the known facts: 02f0e9d did not remove all LEDC backlight code. Eight community LovyanGFX examples added 2026-02-24 to 03-05 still drive GPIO38 with lgfx::Light_PWM at 22 kHz.

What this changes for Gogadget:
- Display bring-up: draw a first frame (or black) before DISPON and the backlight. The maintainer gave this advice in #300 (2025-03-10), but none of LilyGO's own code does it.
- CMD_1's leading SLPOUT has no effect after esp_lcd_panel_init (datasheet p.184). Dropping it should save about 120 ms per wake; test on hardware first.
- Porting LilyGo-Display-IDF code to IDF 6.1 needs edits: .color_space becomes rgb_ele_order, and .psram_trans_align/.sram_trans_align are replaced by dma_burst_size. The v6.1 driver also writes RAMCTRL B0 00 F0 during init, and CMD_1 never overrides it.
- Pixel clock: 10 MHz is within the ST7789V spec (write cycle at least 66 ns). LilyGO's 16, 20 and 40 MHz all exceed the datasheet.
- Backlight driver (AW9364 datasheet):
  - EN high for more than 20 µs to start.
  - Each high and low phase at least 0.5 µs; each low phase under 500 µs.
  - Low for more than 2.5 ms to shut down.
  - EN has an internal 150 kΩ pull-down, so GPIO38 needs no hold in deep sleep.
  - Inference: use explicit 1 µs or longer phases and send the pulse train inside a critical section, so preemption cannot stretch a low phase past 500 µs.
  - Full brightness is 4 x 20 mA, about 80 mA, likely about half of the ~148 mA awake current. A dimmer glance level saves a lot (inference).
- Sleep:
  - SLPIN before every deep sleep is needed for current (6.4 mA down to about 0.25 mA) and against the #300 ghosting on matte panels that were left scanning.
  - esp_lcd_panel_disp_sleep(true) waits a fixed 100 ms, but the datasheet needs only 5 ms after SLPIN. tx_param(0x10) plus about 5 ms is enough before deep sleep.
  - Never send SLPIN within 120 ms of SLPOUT, for example on an aborted wake.
- Power floor: R12 (about 40 µA) plus the AP2112K's quiescent current (55 µA typical) gives about 95 µA that firmware cannot remove. The whole board measures 243-263 µA, which is roughly 40 days of pure sleep on 250 mAh (estimate).
- GPIO15 HIGH on battery costs about 0.5-0.9 mA before any backlight. That is the Q1 base drive (~0.2 mA), R8 (~45 µA), the green LED (0.25-0.6 mA) and the divider (~21 µA); this is an estimate from schematic values. Keep GPIO15 LOW except while displaying or sampling.
- USB detection:
  - With GPIO15 LOW, GPIO4 reads 0 V on battery but about 2.3 V on USB, because VBUS also switches Q1 on. That is more robust than the 4.2 V threshold, but GPIO15 LOW also cuts V3V and the backlight.
  - usb_serial_jtag_is_connected() in v6.1 only detects data hosts, not chargers.
  - Noticing a plug-in during deep sleep needs a timer wake or ULP ADC polling of GPIO4 (ADC1_CH3). A level wake on GPIO4 would sit near the logic-high threshold; the ESP32-S3 VIH of 0.75 x VDD is from memory, not checked here.
- Charging: 525 mA nominal (425-575 mA) into 250 mAh is 1.7-2.3C, with thermal derating above 0.4 A. This confirms the known fact; there is no new data.
