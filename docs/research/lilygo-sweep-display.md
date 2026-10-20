# LilyGO sweep: display, backlight and LVGL

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive): one of seven parallel sweep agents, reading Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5

Status: raw sweep notes; 10 of 48 items were verified in lilygo-verification.md (8 confirmed / 2 partially / 0 refuted); the rest are unverified

Notes: some actions in this raw sweep were superseded by the synthesis (lilygo-decision-impacts.md) and the settled decisions (docs/adr/): CMD_1 is sent byte-for-byte, including `3A 05` (the summary and reusable-code item 1 suggest `3A 55`); the pixel clock stays at 10 MHz; RGB565 bytes are swapped in i80 hardware (`swap_color_bytes = 1`, esp_lvgl_port `swap_bytes = 0`).

Superseded by decision: in deep sleep every LCD pin is parked (CS, RD, WR and RST HIGH; DC and D0-D7 LOW, with holds), not only CS, RST and RD as the summary's sleep order says ([power-budget.md](../hardware/power-budget.md#pin-parking)).

Scope: the vendor init table, esp_lcd bring-up, pixel clock, byte order, orientation, AW9364 backlight, panel sleep and LVGL buffers. Novelty is relative to the facts the sweep agents were given as already known (new, refines, confirms or contradicts).

## Summary

**Init sequence.** LilyGO has one vendor-specific sequence, `LCD_MODULE_CMD_1`. The same values appear in the factory example, every TFT_eSPI example, LilyGo-Display-IDF and TFT_eSPI `INIT_SEQUENCE_3`. Decoded against the bundled ST7789V datasheet it sets:
- porch 11/11
- VGH 14.97 V, VCOM 1.10 V, GVDD 5.1 V, AVCL −4.8 V
- about 53 Hz frame rate
- vendor gamma

`C0 2C` changes nothing (it is the power-on default). `D0 A7` and `D6 A1` are undocumented. `3A 05` should be `3A 55`. LilyGO has sent the table on every unit since 2023; the maintainer says that without it 'the pixels will be blurred'. The two panel versions (glossy and matte) can only be told apart by eye. No read-back is possible over esp_lcd i80.

**ESP-IDF pitfalls.**
- From IDF 5.0 the ST7789 init no longer sends DISPON. That is the real cause of the factory example's 2026-07 'display issues' fix.
- The IDF 6.1 driver also writes RAMCTRL and has `disp_sleep`.
- LilyGo-Display-IDF targets IDF 5.3 with LVGL 8 and uses struct fields that were removed in 6.1.
- RD (GPIO9) must be driven high (#103).

**Clock, byte order, orientation.**
- 10 MHz is the only LilyGO pixel clock inside the ST7789V spec (66 ns write cycle, about 15 MHz max). The factory uses 16 MHz and the nes example 40 MHz.
- LilyGO always swaps RGB565 bytes in software. On IDF 6.1 the i80 `swap_color_bytes` hardware swizzle (or RAMCTRL little-endian) does it at no CPU cost. The device and the desktop simulator can then share one LVGL config.
- `swap_xy` with `mirror(false,true)` puts USB on the left; `mirror(true,false)` puts it on the right. Gap is (0,35), inversion on, RGB order.
- The historic RGB/BGR confusion is most likely TFT_eSPI writing LCMCTRL=0x0C (my inference).

**Hardware facts.**
- No TE line reaches the ESP32-S3 (FPC pin 19 is not connected), so tear-free sync is impossible.
- Panel logic runs from the always-on VDD3V3 rail, so the panel's ~6 mA can only be saved with SLPIN.
- V3V (backlight, AW9364, green LED) is diode-OR'd with **VBUS**. On USB, GPIO15 cannot switch the backlight off; only AW9364 EN can.

**Backlight (AW9364 datasheet).**
- EN has an internal 150 kΩ pull-down; wait at least 20 µs after enabling.
- Pulses need THI ≥0.5 µs and TLO between 0.5 and 500 µs. Holding EN low for 0.8–2.5 ms shuts the chip down.
- 16 steps of 1.25 mA per LED: about 5 mA per step for the four LEDs, about 80 mA at full.
- LilyGO's `setBrightness` (2024-09) is the reference algorithm but has no explicit pulse widths.
- PWM is meaningless on this chip. Even so, LEDC PWM code is still present in the 2026 LovyanGFX MIDI examples.

**Sleep.**
- The panel draws 6 mA in normal mode and 15 µA in sleep-in. LilyGO's bench photo shows 0.243 mA for the whole board.
- Matte panels showed a white border and stale image after long deep sleep when the panel was left awake (#300).
- Turn the backlight on only after the first frame has been written (#232, #300).
- Suggested order: AW9364 off → SLPIN (at least 5 ms) → GPIO15 low → hold CS/RST/RD → deep sleep.
- A faster wake that skips reset and CMD_1 should work, since GRAM survives SLPIN, but it is untested.

**LVGL and buffers.**
- The factory used one 108.8 KB full-frame buffer in internal DMA RAM at 16 MHz.
- LilyGo-Display-IDF used two 34 KB PSRAM buffers and an idle-priority LVGL task; an open issue there (#8) reports slow LVGL under ESP-IDF.
- `lv_conf`: 16-bit with byte swap, 16 ms refresh, Montserrat 14 only.
- The factory clock font is a 120 px 4-bpp digits-only font of about 25 KB. None of LilyGO's fonts have Vietnamese glyphs.

**Still open.** What the undocumented commands do; whether the panel accepts RAMCTRL little-endian; what draws the remaining ~0.22 mA in sleep; the cause of the matte-panel ghosting; what the factory comment 'Too low or too high pixel clock may cause screen mosaic' (2023-03) refers to.

## Knowledge items

| ID | Topic | Relevance | Novelty | Verified |
|---|---|---|---|---|
| K01 | ST7789 init: CMD_1 table decoded | high | refines | partially |
| K02 | CMD_1: non-standard entries | medium | new |  |
| K03 | CMD_1: history and default status | medium | refines |  |
| K04 | Panel revisions: how to tell them apart | medium | new |  |
| K05 | TFT_eSPI INIT_SEQUENCE_3 equals CMD_1 | low | new |  |
| K06 | Other init sequences that also drive this panel | low | new |  |
| K07 | ESP-IDF 5.0+ ST7789 init no longer turns the display on | high | refines | confirmed |
| K08 | ESP-IDF v6.1 built-in ST7789 driver behaviour | high | new |  |
| K09 | Porting LilyGo-Display-IDF code to IDF v6.1 | high | new |  |
| K10 | LilyGO bring-up order | high | refines | partially |
| K11 | Pixel clock values and history | high | refines | confirmed |
| K12 | Pixel clock limits | high | new |  |
| K13 | Orientation, gap and inversion | high | refines | confirmed |
| K14 | Button positions per orientation (derived) | medium | new |  |
| K15 | Colour order and the LCMCTRL XOR | medium | new |  |
| K16 | RGB565 byte order: avoid the CPU swap | high | new |  |
| K17 | No TE line; tearing handling | high | new |  |
| K18 | Panel frame rate under CMD_1 | low | new |  |
| K19 | Draw buffers and DMA settings in LilyGO code | medium | refines |  |
| K20 | LVGL config in lib/lv_conf.h | medium | new |  |
| K21 | LVGL in LilyGo-Display-IDF | medium | refines |  |
| K22 | Fonts and the factory clock face | medium | new |  |
| K23 | AW9364 wiring | high | refines | confirmed |
| K24 | AW9364 timing and current (datasheet) | high | new |  |
| K25 | LilyGO's pulse-count setBrightness | high | refines | confirmed |
| K26 | Backlight control history | medium | refines |  |
| K27 | Why PWM is invalid on the AW9364 (inference) | medium | new |  |
| K28 | LEDC/PWM backlight code still present in 2026 examples | low | contradicts |  |
| K29 | V3V is forced on by USB (VBUS diode-OR) | high | refines | confirmed |
| K30 | Panel supply and deep sleep | medium | refines |  |
| K31 | LilyGO display sleep sequences | high | refines | confirmed |
| K32 | ST7789 sleep and wake timing rules | high | new |  |
| K33 | Sleep current: panel vs whole board | high | refines | confirmed |
| K34 | Ghosting / white border after long deep sleep (#300) | high | new |  |
| K35 | Boot noise: backlight on before GRAM is written | high | new |  |
| K36 | Reset timing | medium | new |  |
| K37 | Optional fast wake (inference) | medium | new |  |
| K38 | RD (GPIO9) must be held high | high | new |  |
| K39 | Colour accuracy | medium | new |  |
| K40 | Unresolved intermittent colour flip | low | new |  |
| K41 | Mechanical and handling glitches | low | new |  |
| K42 | Bug in CMD_1 send loops | low | new |  |
| K43 | LilyGo-Display-IDF latent issues | low | new |  |
| K44 | Strapping pins on the LCD data bus | low | new |  |
| K45 | Diagnostic firmware | low | new |  |
| K46 | Arduino reference versions | low | refines |  |
| K47 | Bundled TFT_eSPI patch history | low | new |  |
| K48 | Consolidated display sequence for Gogadget (synthesis) | high | new |  |

### K01: ST7789 init: CMD_1 table decoded

Relevance: high. Novelty: refines. Verification: partially (see lilygo-verification.md, `display:K01`).

**Fact:** LilyGO's vendor table decodes, per the bundled ST7789V v1.4 datasheet, as follows. 11h SLPOUT then 120 ms. 3Ah 05: 16 bpp on the MCU interface. B2h 0B 0B 00 33 33: back and front porch 11/11 lines (default 0C/0C), separate porch control off, idle and partial porches 3/3. B7h 75: VGH 14.97 V, VGL −10.43 V (default 35h gives 13.26/−10.43). BBh 28: VCOM 1.10 V (default 20h = 0.90 V). C0h 2C: LCMCTRL power-on default, so a no-op. C2h 01: VDV/VRH taken from registers (CMDEN=1). C3h 1F: GVDD 5.10 V + offsets (default 0Bh = 4.10 V). C6h 13: dot inversion, RTNA 13h ≈53 Hz (default 0Fh = 60 Hz). D0h A4 A1: AVDD 6.8 V, AVCL −4.8 V, VDS 2.3 V (default A4 81 has AVCL −4.4 V). E0h/E1h: 14-byte vendor positive and negative gamma. The table contains no MADCTL, INVON, RAMCTRL, CASET/RASET or DISPON, so it does not disturb esp_lcd's orientation, inversion or endianness settings.

**Evidence:** LilyGo-Display-IDF main/display_s3.c:32-47@b1a1cc5; T-Display-S3 examples/factory/factory.ino:133-149@1cc41eb; datasheet/ST7789V_SPEC_V1.4.pdf pp.184,224,263,268,273,275,277-278,284,290,294; #229 lewisxhe 2024-03-05: 'This part of initialization is provided by the screen manufacturer. If this part of the initialization command is not added, the pixels will be blurred.'

**Action:** Send the table verbatim after esp_lcd_panel_init() and invert/swap/mirror. Put these decodes in code comments so nobody 'cleans up' the VCOM or gamma values.

### K02: CMD_1: non-standard entries

Relevance: medium. Novelty: new.

**Fact:** Three entries deviate from the datasheet. (1) D0h A7 is sent as a single byte, but PWCTRL1's first parameter is a fixed A4h key; A7 is undocumented and is immediately overridden by D0h A4 A1. (2) D6h A1 is not in the ST7789V v1.4 command list. (3) 3Ah 05 leaves the RGB-interface field at 000, while the datasheet says COLMOD 'should be set at 55h when writing 16-bit/pixel data'. History: the first vendor table (2022-12-20) had '// {0x3A, {0X06}, 1}' commented out; 3A 05 was added 2023-01-14. TFT_eSPI 2.5.43's copy of the sequence now uses COLMOD 55h followed by 10 ms.

**Evidence:** datasheet p.290 (PWCTRL1 1st param fixed A4h), TOC pp.5-7 (no D6h), p.224 note 2; commits 772b8d8, 65db476; lib/TFT_eSPI/TFT_Drivers/ST7789_Init.h:149-151@1cc41eb (f15652b)

**Action:** Keep D0 A7 and D6 A1 as vendor-provided (LilyGO-validated on all units, possibly ST7789V2/V3/P3-specific). Drop 3Ah from the table (esp_lcd init already sends 55h) or send 55h.

### K03: CMD_1: history and default status

Relevance: medium. Novelty: refines.

**Fact:** LCD_MODULE_CMD_1 was added 2022-12-20 with the note 'The product now has two screens, and the initialization code needs a small change in the new version'. On 2023-03-21 it was disabled in all examples ('Modify the example to the default display version'). It was then re-enabled: factory 2023-04-17; lv_demos and tft 2023-05-22; GetBatteryVoltage 2023-10-12; nes and touch_test 2024-02-01; Sleep 2024-11-27. Every current example and LilyGo-Display-IDF sends it unconditionally.

**Evidence:** git log -G 'define LCD_MODULE_CMD_1': 772b8d8, 2d23043, 84e8604, 5af4f76, fafcb0b, 4b8e1ee, 76ca887, 4b2edc1

**Action:** Send CMD_1 on every unit. Keep a compile-time switch to omit it, only for A/B testing on a matte panel.

### K04: Panel revisions: how to tell them apart

Relevance: medium. Novelty: new.

**Fact:** LilyGO has no electronic way to detect the panel revision. mmMicky (#87, 2023-01-30): CMD_1 'is suitable for optimizing screen display. This can only be observed intuitively... if [the surface] is smooth, it needs to be defined. If it is frosted, no definition is required.' lewisxhe (#122, 2023-03-21): 'There are two versions of the screen'. #300 (2025): the K204 black-shell kit had a glossy screen, an H569 board a matte one, and the post-sleep ghosting happened only on matte units. The 'ScreenDetection' firmware only prints the configured TFT_eSPI pins (e.g. 'DRIVER:%X - should be 0x7789'); it does not read a panel ID.

**Evidence:** issues #87, #122, #300; firmware/README.MD 'Quick Hardware Diagnosis'; strings in firmware/t-display-s3-screen-detect-20230315_0x0.bin; firmware/images/log.png

**Action:** Record glossy or matte for each Gogadget unit. esp_lcd i80 is write-only, so RDDID/RDID1-3 cannot be read; handle revisions by inspection or config.

### K05: TFT_eSPI INIT_SEQUENCE_3 equals CMD_1

Relevance: low. Novelty: new.

**Fact:** TFT_eSPI's INIT_SEQUENCE_3 ('TTGO ESP32 S3 T-Display') is the same vendor sequence, wrapped as: SLPOUT 120 ms, NORON, MADCTL, COLMOD, the CMD_1 values, 120 ms, DISPON, 120 ms, then backlight HIGH inside init. The bundled Setup206 enabled it only on 2026-03-02 (together with TFT_RGB and USE_HSPI_PORT). From 2023-01 to 2026-03 Setup206 used TFT_eSPI's generic ST7789 init with TFT_BGR, and the examples appended CMD_1 by hand.

**Evidence:** lib/TFT_eSPI/TFT_Drivers/ST7789_Init.h:133-237@1cc41eb; lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h:5-14@1cc41eb; commits 04f26b4 (2023-01-14), f15652b (2026-03-02)

**Action:** No action. This confirms CMD_1 is the only vendor-specific sequence; there is no further TFT_eSPI patch to port.

### K06: Other init sequences that also drive this panel

Relevance: low. Novelty: new.

**Fact:** (a) TFT_eSPI generic ST7789 init (JLX240 values): porch 0C/0C, GCTRL 35h, VCOM 28h, LCMCTRL 0Ch, VDVVRHEN 01 FF, VRHS 10h, VDVS 20h, FRCTRL2 0Fh, PWCTRL1 A4 A1, different gamma, INVON, DISPON. (b) Arduino_GFX 1.4.2 generic: HW reset plus SWRESET, SLPOUT 120 ms, COLMOD 55h, MADCTL 00, RAMCTRL 00 F0 (comment '0xF0 MSB first, 0xF8 LSB first'), porch 0C, GCTRL 35h, VCOM 19h, LCMCTRL 2Ch, VRHS 12h, VDVS 20h, FRCTRL2 0Fh, PWCTRL1 A4 A1, gamma, NORON, 10 ms, DISPON. (c) The esp_lcd built-in init alone. README FAQ #2 uses the Arduino_GFX demo as the hardware sanity check, so generic sequences do show an image.

**Evidence:** lib/TFT_eSPI/TFT_Drivers/ST7789_Init.h:8-130; lib/GFX Library for Arduino/src/display/Arduino_ST7789.h:54-120, Arduino_ST7789.cpp:98-120; README.md:202-204@1cc41eb

**Action:** Production uses (c) plus CMD_1. Keep (a)/(b) as diagnostic fallbacks if a unit misbehaves.

### K07: ESP-IDF 5.0+ ST7789 init no longer turns the display on

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K07`).

**Fact:** In IDF v4.4.x, panel_st7789_init ends with DISPON; from v5.0 it does not, so esp_lcd_panel_disp_on_off(panel, true) is mandatory. This is the display part of commit c9b939f ('Fixed ... display issues in Arduino esp32-core v3.3.11'), which added disp_on_off before the CMD_1 loop. lv_demos.ino still never calls it and is guarded by '#error ... please use a version below Arduino ESP32 3.0'.

**Evidence:** esp-idf v4.4.6 vs v5.0 components/esp_lcd/src/esp_lcd_panel_st7789.c panel_st7789_init (DISPON only in 4.4); T-Display-S3 c9b939f diff (factory.ino line 324); examples/lv_demos/lv_demos.ino:3-5,204-222@1cc41eb

**Action:** Call esp_lcd_panel_disp_on_off(true) explicitly, after the first frame has been flushed.

### K08: ESP-IDF v6.1 built-in ST7789 driver behaviour

Relevance: high. Novelty: new.

**Fact:** In v6.1: reset drives RST low 10 ms, then high and waits 10 ms. Init sends SLPOUT, waits 100 ms, then MADCTL, COLMOD 55h and RAMCTRL 00 F0 (F8 when data_endian = LCD_RGB_DATA_ENDIAN_LITTLE). disp_sleep(true/false) sends SLPIN/SLPOUT and waits 100 ms. set_brightness() sends WRDISBV 51h, which does nothing on this board because brightness is controlled by the AW9364. RAMCTRL and disp_sleep exist since v5.2 (absent in v5.1).

**Evidence:** esp-idf v6.1 components/esp_lcd/src/esp_lcd_panel_st7789.c:112-117,162-201,292-334; tag comparison v5.1 vs v5.2..v6.1

**Action:** Use esp_lcd_panel_disp_sleep() for SLPIN/SLPOUT. Never use esp_lcd_panel_set_brightness(). Budget 10+100 ms of driver delays in wake latency.

### K09: Porting LilyGo-Display-IDF code to IDF v6.1

Relevance: high. Novelty: new.

**Fact:** display_s3.c uses fields removed in v6.1. esp_lcd_panel_dev_config_t.color_space (ESP_LCD_COLOR_SPACE_RGB) is now rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB. esp_lcd_i80_bus_config_t.psram_trans_align/sram_trans_align are replaced by dma_burst_size; v6.1 also adds flags.allow_pd and esp_lcd_i80_alloc_draw_buffer(). The repo targets IDF 5.3 and lvgl ^8.3.11.

**Evidence:** LilyGo-Display-IDF main/display_s3.c:88-91,113-118@b1a1cc5, setup.py:182, README 'relies on esp-idf V5.3.0'; esp-idf v6.1 components/esp_lcd/include/esp_lcd_panel_dev.h and esp_lcd_io_i80.h:25-38,116

**Action:** Port by hand. Allocate draw buffers with esp_lcd_i80_alloc_draw_buffer(io, size, MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL), or let esp_lvgl_port allocate them with buff_dma.

### K10: LilyGO bring-up order

Relevance: high. Novelty: refines. Verification: partially (see lilygo-verification.md, `display:K10`).

**Fact:** Factory (2026-07): GPIO15 HIGH → RD (GPIO9) output HIGH → i80 bus (LCD_CLK_SRC_PLL160M) → io (CS6, queue 20, dc_levels 0/0/0/1, 8-bit cmd/param) → st7789 (RST5, RGB, 16 bpp) → reset → init → invert(true) → swap_xy(true) → mirror(false,true) → set_gap(0,35) → disp_on_off(true) → CMD_1 → LVGL → backlight fade. LilyGo-Display-IDF: the same up to gap → CMD_1 → disp_on_off(true) → backlight GPIO HIGH before LVGL has drawn anything. Neither clears GRAM before showing it.

**Evidence:** examples/factory/factory.ino:252-378@1cc41eb; LilyGo-Display-IDF main/display_s3.c:60-154@b1a1cc5

**Action:** Gogadget order: GPIO15 (battery) → RD high → bus, io, panel → reset and init → invert, MADCTL, gap → CMD_1 → render and flush the first frame → DISPON → backlight.

### K11: Pixel clock values and history

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K11`).

**Fact:** Factory pin_config: 6.528 MHz (2022-07-01, commented '//(10 * 1000 * 1000)') → 20 MHz (2022-12-08) → 16 MHz (2023-03-03) with the comment 'Too low or too high pixel clock may cause screen mosaic'. lv_demos still uses 6.528 MHz, which is exactly 60 full frames per second (320×170×2 bytes over an 8-bit bus). LilyGo-Display-IDF uses 10 MHz. The nes example's Arduino_ESP32LCD8 bus defaults to 40 MHz. The MicroPython lvgl binding (#243) uses 10 MHz.

**Evidence:** examples/factory/pin_config.h:24-25@1cc41eb; examples/lv_demos/pin_config.h:14; LilyGo-Display-IDF main/display_s3.c:23; lib/GFX Library for Arduino/src/databus/Arduino_ESP32LCD8.cpp:45-54; git log -G PIXEL_CLOCK (758b2a3, 58bd641, adc038e); #243

**Action:** Keep 10 MHz. If 'mosaic' appears, step down (8 or 6.67 MHz) rather than up.

### K12: Pixel clock limits

Relevance: high. Novelty: new.

**Fact:** The ST7789V 8080 write cycle TWC is at least 66 ns (TWRL/TWRH at least 15 ns), i.e. at most 15.15 MHz. The factory's 16 MHz is slightly out of spec; 20 and 40 MHz are well out. On the ESP32-S3 with IDF v6.1 the i80 clock is PLL_F160M/2 = 80 MHz divided by an integer. 10 MHz is exact (÷8) and 16 MHz is ÷5; a 6.528 MHz request becomes 6.67 MHz (÷12). At 10 MHz a full 320×170 frame takes about 10.9 ms.

**Evidence:** datasheet p.41 Table 4; esp-idf v6.1 components/esp_lcd/i80/esp_lcd_panel_io_i80.c:343-364,645-660; priv_include/esp_lcd_common.h:28 (LCD_PERIPH_CLOCK_PRE_SCALE 2); soc/esp32s3/include/soc/clk_tree_defs.h:186-191

**Action:** 10 MHz is in spec with margin and divides exactly; keep it.

### K13: Orientation, gap and inversion

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K13`).

**Fact:** esp_lcd settings: invert_color(true) and swap_xy(true). mirror(false,true) means 'The screen faces you, and the USB is on the left'; mirror(true,false) means 'the USB is to the right'. set_gap(0,35) applies to both, because 170 visible columns are centred in the 240-column GRAM. TFT_eSPI: rotation 1 = MX|MV, rotation 3 = MV|MY, CGRAM rowstart 35 in landscape and colstart 35 in portrait. Arduino_GFX uses offsets 35/0/35/0. The LovyanGFX MIDI examples use offset_x 35, offset_rotation 1, invert true, then setRotation(2). Every path enables inversion (IPS panel).

**Evidence:** LilyGo-Display-IDF main/display_s3.c:125-137; examples/factory/factory.ino:310-322 (comments added 01a297f 2023-06-01); lib/TFT_eSPI/TFT_Drivers/ST7789_Rotation.h:23-27,57-61,68,124-128,135; examples/Arduino_GFXDemo/Arduino_GFXDemo.ino:16; examples/T-Display-S3-Gingoduino/ST7789_Handler.h:58-66

**Action:** Rotate with MADCTL (swap_xy + mirror), not LVGL software rotation. The (0,35) gap stays valid for a 180° flip.

### K14: Button positions per orientation (derived)

Relevance: medium. Novelty: new.

**Fact:** The pinmap drawing shows BOOT (GPIO0) bottom-left and IO14 bottom-right below the screen, with USB-C at the bottom. Rotating the drawing gives: in 'USB on the left' landscape (LilyGO's esp_lcd default), GPIO0 is the upper button and GPIO14 the lower one, both on the left edge. With USB on the right, GPIO14 is upper-right and GPIO0 lower-right.

**Evidence:** image/T-DISPLAY-S3.jpg@1cc41eb (derived by rotating the drawing)

**Action:** Place the Vietnamese UI hints next to the physical buttons for the chosen orientation; verify on hardware.

### K15: Colour order and the LCMCTRL XOR

Relevance: medium. Novelty: new.

**Fact:** The power-on default of LCMCTRL (C0h) is 2Ch, with XBGR=1 and XMX=1; these XOR the MADCTL BGR and MX bits. esp_lcd never writes C0h and CMD_1 writes the default 2Ch, so LilyGO's IDF/esp_lcd code uses RGB order. TFT_eSPI's generic ST7789 init writes C0h=0Ch (XBGR=0). That is why the bundled TFT_eSPI setup used TFT_BGR from 2022 to 2026 and switched to TFT_RGB only when INIT_SEQUENCE_3 (C0h=2Ch) was enabled. This also explains #121 (upstream TFT_eSPI works with TFT_BGR) and #122 (a user needed RGB/BGR swapped; maintainer: 'There are two versions of the screen'). This explanation is my inference.

**Evidence:** datasheet p.275; lib/TFT_eSPI/TFT_Drivers/ST7789_Init.h:49-50 vs 168-169; Setup206 at 04f26b4 (TFT_BGR) vs f15652b (TFT_RGB); git show 758b2a3:lib/TFT_eSPI/User_Setups/Setup24_ST7789.h (TFT_BGR); issues #121, #122

**Action:** Use LCD_RGB_ELEMENT_ORDER_RGB with CMD_1. Keep a Kconfig or NVS switch to flip RGB/BGR if a unit shows red and blue swapped.

### K16: RGB565 byte order: avoid the CPU swap

Relevance: high. Novelty: new.

**Fact:** Every LilyGO LVGL setup swaps bytes in software: lib/lv_conf.h LV_COLOR_16_SWAP 1, IDF CONFIG_LV_COLOR_16_SWAP=y, TFT_eSPI setSwapBytes(true). LVGL 9 has no LV_COLOR_16_SWAP. IDF v6.1 offers two zero-CPU alternatives. (a) esp_lcd_panel_io_i80_config_t.flags.swap_color_bytes: the LCD_CAM swizzle, applied only to colour transfers. It appears in #103's IDF-5 test code ('Swap can be done in LvGL (default) or DMA'); that display worked after the RD fix, though colour correctness was not shown. (b) data_endian = LCD_RGB_DATA_ENDIAN_LITTLE: sets the ST7789 RAMCTRL ENDIAN bit ('Little Endian only can be supported in 65K 8-bit and 9-bit interface'); CMD_1 does not touch B0h. esp_lvgl_port 2.9's flags.swap_bytes is a CPU swap.

**Evidence:** lib/lv_conf.h:30@1cc41eb; LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:3; esp-idf v6.1 components/esp_lcd/i80/esp_lcd_panel_io_i80.c:519,884-886 and src/esp_lcd_panel_st7789.c:112-117; datasheet p.258; issue #103; espressif/esp-bsp components/esp_lvgl_port/include/esp_lvgl_port_disp.h:69 (v2.9.0)

**Action:** Set i80 swap_color_bytes=1 (or data_endian LITTLE, untested by LilyGO) and leave esp_lvgl_port swap_bytes=0. The desktop simulator and device then render identical RGB565, so screenshot tests compare directly. Never enable two swaps.

### K17: No TE line; tearing handling

Relevance: high. Novelty: new.

**Fact:** The 30-pin LCD FPC (CN1, KH-FG0.5-H2.0-30PIN) routes only RES, CS, DC, WR, RD, D0–D7, VDD3V3 (pin 2), V3V (pin 20), LEDK1–4 (pins 21–24) and touch (26–29). Pin 19 is unconnected, and no TE GPIO exists; product_pins.h has no TE define for T-Display-S3, unlike the AMOLED boards. The repo's only anti-tearing measure is in the 2026 LovyanGFX MIDI examples: one full-frame sprite push, 'eliminating the diagonal tearing caused by the ST7789 scan line crossing multiple separate write transactions'.

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd Display block (identical @811c949, 2023-03-13); LilyGo-Display-IDF main/product_pins.h:202-231 vs :28 (BOARD_DISP_TE for AMOLED Lite); examples/T-Display-S3-Piano/PianoDisplay.h:8-11

**Action:** There is no vsync. Keep updates small (digits) or flush each changed region in one transaction, and avoid full-screen animation or scrolling in the glance clock.

### K18: Panel frame rate under CMD_1

Relevance: low. Novelty: new.

**Fact:** Frame rate = 10 MHz / ((320 + FPA + BPA) × (250 + 16·RTNA)). With CMD_1 (FPA=BPA=0Bh, RTNA=13h) this is about 52.8 Hz (18.9 ms per frame) with dot inversion; the defaults give about 60 Hz. A 10 MHz full-frame write (10.9 ms) is faster than one panel scan, so an unsynchronised full-frame update tears at most once.

**Evidence:** datasheet p.284 (formula; table 13h = 53 Hz); CMD_1 entries B2h and C6h

**Action:** No change needed. Do not alter C6h; it is vendor-tuned.

### K19: Draw buffers and DMA settings in LilyGO code

Relevance: medium. Novelty: refines.

**Fact:** factory and lv_demos: one full-screen buffer (320×170×2 = 108 800 B) in MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL; max_transfer_bytes 108 800; trans_queue_depth 20; flush-ready callback in on_color_trans_done (single buffer, so LVGL waits for each DMA). LilyGo-Display-IDF with PSRAM: two 170×100-px buffers (34 000 B each) in MALLOC_CAP_SPIRAM, psram_trans_align 64, max_transfer_bytes 34 000, queue depth 10, full_refresh false. Without PSRAM: two 320×20-px DMA buffers. LovyanGFX MIDI examples: a ~108 KB full-frame sprite in PSRAM.

**Evidence:** examples/factory/factory.ino:257-297,333-335 and pin_config.h:27-29@1cc41eb; LilyGo-Display-IDF main/display_s3.c:72-111, main/main.cpp:150-186, product_pins.h:229-231

**Action:** Two internal DMA buffers of 320×20–40 lines are enough. The factory proves a 108.8 KB internal full-frame buffer fits, if full-frame rendering is needed to reduce tearing.

### K20: LVGL config in lib/lv_conf.h

Relevance: medium. Novelty: new.

**Fact:** LVGL 8.3.0-dev, unchanged since 2022-07-01. Colour: LV_COLOR_DEPTH 16, LV_COLOR_16_SWAP 1. Memory: LV_MEM_CUSTOM 1 (malloc; the 48 KB pool is unused). Timing: refresh period 16 ms, indev read 30 ms, LV_TICK_CUSTOM millis(). Drawing: LV_DPI_DEF 130, LV_DRAW_COMPLEX 1, shadow cache 0, circle cache 4, layer buffers 24/3 KB, image cache 0, gradient stops 2, rotation buffer 10 KB. Logging off; NULL and MALLOC asserts on. Fonts: only Montserrat 14 (default), UTF-8. Features: GIF and MSG on, dark default theme, benchmark demo enabled.

**Evidence:** lib/lv_conf.h:27,30,49-66,81,84,88-91,96,108-170,222,256-257,356,391,418,566,639,692,727@1cc41eb; git log lib/lv_conf.h (single commit b12c08a)

**Action:** Don't copy it; LVGL 9 option names differ. LilyGO ran a 16 ms refresh with a full-frame buffer. In battery mode a longer period, or pausing LVGL when idle, is fine.

### K21: LVGL in LilyGo-Display-IDF

Relevance: medium. Novelty: refines.

**Fact:** sdkconfig.defaults.t-display-s3: CONFIG_LV_COLOR_16_SWAP=y, 16-bit, 48 KB LV_MEM pool, Montserrat 12–48 all enabled (default 14), demos, octal SPIRAM, 240 MHz, USB-Serial-JTAG console. main.cpp: lv_tick_inc every 2 ms from an esp_timer. The LVGL task has a 4 KB stack, a delay clamped to 1–500 ms, and runs at tskIDLE_PRIORITY (the priority-2 line is commented out). Its issue #8, 'Runs very slow compared to pio Arduino version', is still open.

**Evidence:** LilyGo-Display-IDF sdkconfig.defaults.t-display-s3:1-38; main/main.cpp:34-38,105-123,190-199,230-233@b1a1cc5; LilyGo-Display-IDF issue #8

**Action:** Don't copy the idle-priority task or the PSRAM draw buffers. Use esp_lvgl_port defaults with internal DMA buffers.

### K22: Fonts and the factory clock face

Relevance: medium. Novelty: new.

**Fact:** The factory clock uses a custom 'font_Alibaba': size 120 px, 4 bpp, only U+0030–U+003A, about 25 KB of glyph bitmaps, line height 86 px. Layout: two 140×140 boxes in #282828 at ±85 px; the colon blinks via a 500 ms lv_timer; a 3-page tileview is switched by GPIO14. Montserrat, which LilyGO enables at size 14 or 12–48, has no Vietnamese diacritics.

**Evidence:** examples/factory/font_Alibaba.c:1-5,3203-3229,3301; examples/factory/factory_gui.cpp:23-80,137-149; factory_gui.h:3-6@1cc41eb

**Action:** Generate fonts with lv_font_conv covering Latin plus the Vietnamese U+00C0–U+1EF9 subset. A digits-only 120 px 4-bpp clock font costs only about 25 KB. Check the source font's licence.

### K23: AW9364 wiring

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K23`).

**Fact:** U4 AW9364DNR: EN = ESP_LCD_BLK (GPIO38, direct, no external resistor); VIN = V3V with 1 µF; LED1–4 sink pins go to LEDK1–4 (FPC pins 21–24). The LED anodes are on FPC pin 20 = V3V. The 2023-03-13 and 2024-04-29 schematic revisions are identical here.

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd Display block; same @811c949

**Action:** The driver should assume four LED strings on a switched 3.3 V rail.

### K24: AW9364 timing and current (datasheet)

Relevance: high. Novelty: new.

**Fact:** Awinic AW9364 V2.4. EN is active high with an internal 150 kΩ pull-down. Start-up TON ≈20 µs ('ready time recommended to be greater than 20 µs'). THI ≥0.5 µs; TLO between 0.5 and 500 µs; shutdown delay TOFF 0.8–2.5 ms. Each EN rising edge steps the current down by 1.25 mA per channel: edge 1 = 20 mA, 18.75, …, edge 16 = 1.25 mA, edge 17 wraps back to 20 mA. Quiescent IQ 330 µA typ when enabled; shutdown ISD 0.1 µA typ (1 µA max); VIN 2.8–5.5 V. With four LEDs that is about 5 mA per level and about 80 mA at level 16.

**Evidence:** https://doc.awinic.com/doc/202402/d1a46d65-a45d-494f-bfc9-3d6a6a93a860.pdf p.5 (electrical characteristics), p.6 (EN 150 kΩ pull-down), p.8 (Fig. 7), p.9 (Table 1)

**Action:** Driver: EN high then wait ≥30 µs (level 16). Each step is LOW 1–2 µs then HIGH ≥1 µs, inside a critical section so TLO stays under 500 µs. Off means LOW for ≥3 ms. Budget about 5 mA per level for the 10 s glance (e.g. level 3–6). GPIO38 needs no hold in deep sleep because of the internal pull-down.

### K25: LilyGO's pulse-count setBrightness

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K25`).

**Fact:** Added to the factory on 2024-09-23 after #278. 16 levels. Value 0: BL LOW then delay(3) (shutdown). From off: BL HIGH, delayMicroseconds(30), level = 16. Then (16 + level − value) % 16 LOW→HIGH pulses via back-to-back digitalWrite, with no explicit pulse width. Code comment: 'The adjustable range is 0~16, 0 is the minimum brightness, 16 is the maximum'. The factory fades 0→16 at 50 ms per step while LVGL renders a GIF.

**Evidence:** examples/factory/factory.ino:223-248,371-378@1cc41eb; commit 2d3c294; #278 lewisxhe 2024-09-23: 'The method of ledc controlling the backlight of display-S3 is wrong. I have modified it to the correct control method.'

**Action:** Port the modular step maths with explicit µs timing. Reset the cached level after any EN-low period ≥0.8 ms, and after V3V loss (GPIO15 low on battery).

### K26: Backlight control history

Relevance: medium. Novelty: refines.

**Fact:** 2022–2024: every example drove GPIO38 with LEDC PWM. Factory used a 10 kHz fade 0→255. tft, touch_test, usb_hid_pad, GetBatteryVoltage and nes used 2 kHz at 255; tft's IDF5 path used ledcAttach at 200 Hz. PCBClock used 10 kHz with 7 brightness steps; PokerS3 160/255; nofrendo 12 kHz. A community post (#74) recommended LEDC at 5 kHz. In #248/#278, tone() reused LEDC channel 0 and blanked the screen. 2024-09-23: the factory moved to pulse counting. 2026-03-09 (02f0e9d): all other examples moved to a plain digitalWrite HIGH, so PCBClock's brightness button is now a no-op.

**Evidence:** git show 02f0e9d; commit 2d3c294; issues #74, #248, #278; examples/PCBClock/PCBClock.ino:31-32,116-123@1cc41eb

**Action:** Use only pulse counting or plain on/off.

### K27: Why PWM is invalid on the AW9364 (inference)

Relevance: medium. Novelty: new.

**Fact:** Any PWM whose low phase lasts 0.5–500 µs is counted as a dimming pulse. The current therefore steps down once per PWM period and wraps every 16 periods; at intermediate duty a 200 Hz PWM cycles through all levels 12.5 times per second. Low phases longer than 0.8–2.5 ms shut the chip down. Only 0 % and 100 % duty behave predictably.

**Evidence:** AW9364 datasheet pp.5,8-9 combined with the LEDC settings removed in 02f0e9d

**Action:** Never attach LEDC to GPIO38. Keep EN glitch-free while other peripherals reconfigure pins.

### K28: LEDC/PWM backlight code still present in 2026 examples

Relevance: low. Novelty: contradicts.

**Fact:** Seven LovyanGFX MIDI examples merged Feb–Mar 2026 (PRs #328–#336) still configure lgfx::Light_PWM on pin 38 at 22 kHz (channel 7) and call setBrightness(255). Commit 02f0e9d did not touch them. They then digitalWrite(38, HIGH), so only full brightness is ever used.

**Evidence:** examples/T-Display-S3-Piano/PianoDisplay.h:113 and PianoDisplay.cpp:14-20; examples/T-Display-S3-Gingoduino/ST7789_Handler.h:70-79; same pattern in BLE-Receiver, BLE-Sender, Queue, USB-Device, Piano-Debug

**Action:** Do not take these examples as evidence that PWM dimming works.

### K29: V3V is forced on by USB (VBUS diode-OR)

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K29`).

**Fact:** Q1 (PMBT3904) is driven through a 1N4148 diode-OR (D2) of VBUS and PWR_EN (GPIO15), with R5 10 k and R7 100 k pull-down. Q1 switches Q3 (SI2301, VDD3V3→V3V) and Q2 (the battery-ADC net). Whenever USB is present, V3V is on regardless of GPIO15: the backlight LED anodes, AW9364 VIN and the green LED (2 kΩ, ≈0.6 mA). On battery only GPIO15 enables it. The README says 'In USB mode, the green indicator stays on'; #284 reports the TFT going black only after USB is unplugged. The 2023 schematic has the same circuit.

**Evidence:** schematic/T_Display_S3.pdf@b0a5ecd Power block (Q1, D2, R5, R7, Q2, Q3); @811c949; README.md:253-256; issue #284

**Action:** On USB, turn the backlight off with GPIO38 only (AW9364 shutdown, EN low ≥3 ms). On battery, drive GPIO15 low before deep sleep. The green LED stays lit whenever V3V is on.

### K30: Panel supply and deep sleep

Relevance: medium. Novelty: refines.

**Fact:** Panel logic (FPC pin 2) runs from VDD3V3, the always-on LDO output, so GRAM and registers survive deep sleep. Datasheet: in Sleep-In the 'MCU interface and memory are still working and the memory keeps its contents'. Power-off rules: in Sleep-In, VDD can drop immediately after RESX; in Sleep-Out, VDD must stay up ≥120 ms after RESX release.

**Evidence:** schematic CN1 pin 2 = VDD3V3; datasheet pp.136,138,182

**Action:** Panel power cannot be cut, so SLPIN is the only way to save its ~6 mA.

### K31: LilyGO display sleep sequences

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K31`).

**Fact:** Factory button1: tx_param(0x10 SLPIN) → GPIO15 LOW → GPIO38 LOW → ext0 wake on GPIO14 low → esp_deep_sleep_start(); no DISPOFF, no delay. Sleep example: fillScreen black, draw 'Sleep', 1 s, GPIO15 LOW, GPIO38 LOW, writecommand(0x10), deep sleep; it also has a 15 s timer-wake loop added for #300. No LilyGO code sends DISPOFF (28h). For comparison, Arduino_GFX displayOff() is SLPIN + 120 ms, and IDF disp_sleep is SLPIN + 100 ms.

**Evidence:** examples/factory/factory.ino:395-404 (c9fd58a 2024-11-27); examples/Sleep/Sleep.ino:77-117 (4b2edc1, fc466c6); lib/GFX Library for Arduino/src/display/Arduino_ST7789.cpp:84-94

**Action:** Gogadget order: AW9364 off (EN low) → esp_lcd_panel_disp_sleep(true) (≥5 ms; IDF waits 100 ms) → GPIO15 low → define and hold LCD pins → deep sleep. DISPOFF is optional because SLPIN blanks automatically.

### K32: ST7789 sleep and wake timing rules

Relevance: high. Novelty: new.

**Fact:** SLPIN stops the DC/DC converter, oscillator and scanning, drains panel charge, and needs 5 ms before the next command. SLPOUT needs 5 ms before the next command, shows a blank screen for 2 frames, runs self-diagnostics, and must precede any later SLPIN by ≥120 ms. DISPOFF inserts a blank page and leaves frame memory unchanged. SWRESET keeps frame memory; wait 5 ms after it (120 ms before SLPOUT if sent in Sleep-In).

**Evidence:** datasheet pp.163, 182-185, 194

**Action:** If sleep is requested less than 120 ms after wake, delay the SLPIN. CMD_1's 120 ms after SLPOUT is conservative; the minimum is 5 ms.

### K33: Sleep current: panel vs whole board

Relevance: high. Novelty: refines. Verification: confirmed (see lilygo-verification.md, `display:K33`).

**Fact:** The maintainer's bench photo for the Sleep example shows 0.243 mA DC for the whole board. Datasheet: Normal-mode IDD 6.0 mA typ (7.5 max); Sleep-In IDD 15 µA typ (30 max) plus IDDI 5 µA. This matches #289 (6.42 mA without SLPIN, 263 µA with it). In #33 a user found DISPOFF + backlight off still drew ~30 mA more than a never-initialised display.

**Evidence:** examples/Sleep/IMAGE.jpg (added 4b2edc1, 2024-11-27); datasheet p.40 Table 3; issues #289, #33 (wichjan 2023-06-21)

**Action:** Budget ≈0.25 mA asleep (≈6 weeks on 250 mAh before any wake time). The remaining ~0.22 mA is not the panel; investigate it separately. Awake budget: panel ≈6 mA + backlight ≈5 mA per level + green LED ≈0.6 mA + MCU.

### K34: Ghosting / white border after long deep sleep (#300)

Relevance: high. Novelty: new.

**Fact:** A user saw a white border and stale, smeared text after waking from deep sleep of several minutes on battery, only on matte panels; their MicroPython s3lcd driver did not send SLPIN. Maintainer: 'After the screen is initialized, refresh the screen to all black, and then turn on the backlight last.' The user's fix: drive all LCD pins (38–48, 15, 5–9) LOW before deep sleep. LilyGO updated the Sleep example in response (backlight after the first image, timer-wake loop).

**Evidence:** issue #300 (sgbmzm 2025-03-14/18/24; lewisxhe 2025-03-10/19); commit fc466c6

**Action:** Always SLPIN before sleep. Hold CS, RD and RST in a defined state during sleep. At wake, draw a full frame before the backlight. Run a multi-hour sleep/wake soak test on a matte unit.

### K35: Boot noise: backlight on before GRAM is written

Relevance: high. Novelty: new.

**Fact:** #232: white noise was visible at boot, especially after the board had been unpowered for over a minute, because TFT_eSPI turns the backlight on inside init. Maintainer's fix: remove TFT_BL from Setup206 and turn the backlight on after tft.begin(). LilyGo-Display-IDF also raises the backlight GPIO at the end of display_init(), before LVGL has drawn anything.

**Evidence:** issue #232 lewisxhe 2024-03-13; lib/TFT_eSPI/TFT_eSPI.cpp:786-790; LilyGo-Display-IDF main/display_s3.c:145-153

**Action:** Enable the AW9364 only after the first complete frame has been flushed and DISPON sent.

### K36: Reset timing

Relevance: medium. Novelty: new.

**Fact:** Datasheet: reset pulse ≥10 µs. Reset cancel takes ≤5 ms when reset in Sleep-In, but up to 120 ms when reset in Sleep-Out (the display blanks and NVM reloads). IDF waits only 10 ms after releasing RST before SLPOUT. TFT_eSPI waits 150 ms; Arduino_GFX waits 120 ms, then SWRESET and another 120 ms. CMD_1's leading SLPOUT + 120 ms therefore also recovers a warm reboot in which the first SLPOUT fell inside the 120 ms window.

**Evidence:** datasheet p.48 Table 9; esp-idf v6.1 esp_lcd_panel_st7789.c:162-180; lib/TFT_eSPI/TFT_eSPI.cpp:693-704; Arduino_ST7789.cpp:98-115

**Action:** Keep CMD_1's SLPOUT. After a deep-sleep wake (panel was in SLPIN), the 10 ms IDF reset is sufficient.

### K37: Optional fast wake (inference)

Relevance: medium. Novelty: new.

**Fact:** VDD3V3 stays on and SLPIN keeps GRAM and registers. A deep-sleep wake could therefore skip reset, init and CMD_1 (≈230 ms: 10+10+100+120): send SLPOUT, wait ≥5 ms, flush the new time, DISPON, then backlight. Preconditions: RST (GPIO5) and CS (GPIO6) were held during sleep, and the panel is created with reset_gpio_num = -1, because esp_lcd_new_panel_st7789 calls gpio_config on the RST pin and would pull it low. esp_lcd i80 cannot read back registers to verify the panel state.

**Evidence:** datasheet pp.138,182; esp-idf v6.1 esp_lcd_panel_st7789.c:76-82; schematic CN1 pin 2

**Action:** Prototype only after the safe path works. Use full init for any reset reason other than ESP_RST_DEEPSLEEP.

### K38: RD (GPIO9) must be held high

Relevance: high. Novelty: new.

**Fact:** All LilyGO code drives RD as an output HIGH before creating the i80 bus; esp_lcd itself does not use RD. In #103 an IDF-5 T-Display-S3 port stayed black or showed garbage until RD was pulled up ('if it is driven low, it would impact the transfer'); two users confirmed.

**Evidence:** LilyGo-Display-IDF main/display_s3.c:64-69; examples/factory/factory.ino:257-258@1cc41eb; issue #103 (hoffmajs 2023-05-02)

**Action:** Configure RD as output level 1 before esp_lcd_new_i80_bus(), and hold it high in deep sleep.

### K39: Colour accuracy

Relevance: medium. Novelty: new.

**Fact:** #113: 0xF800 red renders as orange, and light grey 0xCDF9 turns greenish off-axis. Maintainer: 'the current initial gamma value is provided by the screen manufacturer. I don't know how to calibrate the gamma value'; he raised it with the vendor, with no fix.

**Evidence:** issue #113 lewisxhe 2023-03-13

**Action:** Choose the palette on real hardware and don't rely on pure red for alerts. Desktop screenshots will not show this shift.

### K40: Unresolved intermittent colour flip

Relevance: low. Novelty: new.

**Fact:** #118: on a custom carrier board with esp_lcd i80 + ST7789, colours briefly flipped (yellow-on-red became purple-on-green) for up to 1 s, as if INVON had reverted. Unresolved.

**Evidence:** issue #118 (2023-03-10)

**Action:** For robustness, re-send INVON and MADCTL on each wake or minute update.

### K41: Mechanical and handling glitches

Relevance: low. Novelty: new.

**Fact:** #128: half the screen showed garbage. Maintainer: 'it may be a screen burning phenomenon similar to OLED'; other users broke the FPC or lower data lines when pushing the board into breadboards. #65: the shipping foam placed behind the screen inside a case is conductive and caused a barely-glowing or garbage screen.

**Evidence:** issues #128 (lewisxhe 2023-03-24), #65 (jessedmatlock 2023-04-25)

**Action:** Enclosure: no conductive foam behind the PCB or FPC; add strain relief for the FPC.

### K42: Bug in CMD_1 send loops

Relevance: low. Novelty: new.

**Fact:** nes.ino:80, usb_hid_pad.ino:94, touch_test.ino:83 and GetBatteryVoltage.ino:57 use 'j < lcd_st7789v[i].len & 0x7f', which parses as (j < len) & 0x7f. For the SLPOUT entry (len 0x80) this writes 128 bytes, reading past the 14-byte data array. tft.ino:53 and Sleep.ino:59 use the correct '(len & 0x7f)'.

**Evidence:** examples/{nes,usb_hid_pad,touch_test,GetBatteryVoltage,tft,Sleep}/*.ino at the cited lines @1cc41eb

**Action:** In the port, use an explicit {cmd, len, delay_ms, data[]} struct instead of flag bits.

### K43: LilyGo-Display-IDF latent issues

Relevance: low. Novelty: new.

**Fact:** main.cpp's DISPLAY_FULLRESH branch passes width and height as end coordinates, which is only correct when x1=y1=0. display_push_colors() names its end-coordinate parameters 'width, hight'. T-Display-S3 is configured with BOARD_HAS_TOUCH 1, so touch init also runs on non-touch boards. power_driver_init sets GPIO15 HIGH. Return values of esp_lcd_new_i80_bus and esp_lcd_new_panel_st7789 are not checked.

**Evidence:** LilyGo-Display-IDF main/main.cpp:49-63,139-143; main/display_s3.c:49-52,93,119; main/product_pins.h:227; main/power_driver.cpp:174-195@b1a1cc5

**Action:** Don't copy this code verbatim.

### K44: Strapping pins on the LCD data bus

Relevance: low. Novelty: new.

**Fact:** LCD D4 and D5 are GPIO45 and GPIO46, both ESP32-S3 strapping pins; ESPHome configs for this board need ignore_strapping_warning on them.

**Evidence:** README.md:140-141@1cc41eb; issue #323

**Action:** Don't hold D4/D5 high through sleep or reset; leave the data bus floating or held low.

### K45: Diagnostic firmware

Relevance: low. Novelty: new.

**Fact:** firmware/ provides factory images (no-touch 2023-10-13, touch 2023-04-17), an LVGL demo (2025-01-09), a TFT_eSPI demo (2023-11-13) and the ScreenDetection tool (built on IDF v4.4.4). The README says that if serial output is normal but nothing shows, 'the screen may be damaged'.

**Evidence:** firmware/README.MD@1cc41eb; string 'esp-idf: v4.4.4' in t-display-s3-screen-detect-20230315_0x0.bin

**Action:** Flash the no-touch factory image to rule out hardware before debugging Gogadget display code.

### K46: Arduino reference versions

Relevance: low. Novelty: refines.

**Fact:** The README tests Arduino core 2.0.14. CI compiles all examples with esp32:esp32@2.0.17, and platformio pins espressif32@6.5.0. Bundled libraries: LVGL 8.3.0-dev, TFT_eSPI 2.5.43. Only the factory example was adapted to core 3.3.11.

**Evidence:** README.md:86; .github/workflows/arduino_ci.yml:53; platformio.ini:39; lib/lvgl/library.json:3; lib/TFT_eSPI/library.properties:2@1cc41eb

**Action:** Treat LilyGO's Arduino code as an IDF 4.4-era reference.

### K47: Bundled TFT_eSPI patch history

Relevance: low. Novelty: new.

**Fact:** 2022-07: LilyGO shipped a modified TFT_eSPI 2.4.71: Setup24_ST7789.h repurposed for 170×320 with TFT_BGR and INVERSION_ON, plus a custom TFT_DATA_PIN_OFFSET_EN macro ('added by Micky -20220701') for data pins ≥32. 2023-01: replaced by upstream 2.5.0 with Setup206; in #82 Micky said 'I have submitted a pull request to TFT_eSPI'. 2023-11: TFT_CS 6 added (it had been missing). 2026-03: upstream 2.5.43 with INIT_SEQUENCE_3, TFT_RGB and USE_HSPI_PORT ('I only changed two parts', #326).

**Evidence:** git show 758b2a3:lib/TFT_eSPI/User_Setups/Setup24_ST7789.h; commits c3dfc1a, 04f26b4, 0742ef9, f15652b; issues #66, #82, #326

**Action:** None for the ESP-IDF build.

### K48: Consolidated display sequence for Gogadget (synthesis)

Relevance: high. Novelty: new.

**Fact:** Wake: GPIO15 HIGH (battery) → RD HIGH → i80 bus (DC7, WR8, D0–D7 = 39,40,41,42,45,46,47,48, LCD_CLK_SRC_DEFAULT) → io (CS6, 10 MHz, dc_levels 0/0/0/1, 8/8 bits, swap_color_bytes=1) → st7789 (RST5, RGB, 16 bpp) → reset → init → invert(true) → swap_xy(true) → mirror(false,true) → gap(0,35) → CMD_1 (3Ah→55h) → render and flush the first frame → disp_on_off(true) → AW9364 EN high ≥30 µs, then pulses to the target level. Sleep: AW9364 off (EN low ≥3 ms) → disp_sleep(true) → GPIO15 LOW → hold CS6/RD9/RST5 → deep sleep with GPIO0/GPIO14 wake.

**Evidence:** Derived from K07–K38: examples/factory/factory.ino@1cc41eb, LilyGo-Display-IDF main/display_s3.c@b1a1cc5, ST7789V datasheet, AW9364 datasheet, issues #103, #232, #289, #300

**Action:** Use this as the baseline implementation and measure sleep current and visual behaviour on both a glossy and a matte unit.

## Reusable code

### 1. Vendor ST7789 init table (CMD_1) with delay flag and the send loop

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:32-47 and 139-143 @b1a1cc5 (identical table in Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:126-150 @1cc41eb)

**Licence:** MIT (LilyGo-Display-IDF LICENSE 'Copyright (c) 2024 TTGO'; file header '@license MIT, Copyright (c) 2024 Shenzhen Xinyuan Electronic Technology Co., Ltd')

**Port notes:** Keep it as a static const table of {cmd, len, delay_ms, data[14]} and send with esp_lcd_panel_io_tx_param() after esp_lcd_panel_init() and invert/swap/mirror. Change 3A 05 to 3A 55, or drop it. Keep D0 A7 and D6 A1 (undocumented but vendor-validated). SLPOUT delay: 120 ms (5 ms is the datasheet minimum). Parenthesise len masks or avoid flag bits (see the K42 bug). Keep the MIT notice.

### 2. i80 bus and ST7789 panel bring-up for T-Display-S3 (RD high, bus, io, panel, invert, swap_xy, mirror, gap)

**Source:** Xinyuan-LilyGO/LilyGo-Display-IDF main/display_s3.c:60-154 @b1a1cc5; Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:257-331 @1cc41eb

**Licence:** MIT

**Port notes:** For IDF v6.1: replace .color_space with .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB. Remove psram_trans_align/sram_trans_align (use dma_burst_size or leave 0). Use pclk 10 MHz with LCD_CLK_SRC_DEFAULT. Add io flags.swap_color_bytes = 1 instead of LVGL byte swapping. trans_queue_depth 10. Check every esp_err_t. Move esp_lcd_panel_disp_on_off(true) after the first LVGL flush. Replace the backlight GPIO write with the AW9364 driver. Size max_transfer_bytes to the esp_lvgl_port buffer.

### 3. AW9364 pulse-count brightness (16 levels, wrap-around step maths)

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:223-248 @1cc41eb

**Licence:** MIT (T-Display-S3 LICENSE, Copyright (c) 2022 Xinyuan-LilyGO)

**Port notes:** Replace digitalWrite with gpio_set_level or gpio_ll set/clear. Hold each LOW and HIGH phase for 1–2 µs with esp_rom_delay_us (datasheet minimum 0.5 µs). Wait ≥30 µs after the first enable (TON 20 µs). Wrap the pulse train (≤15 pulses, under 50 µs) in portENTER_CRITICAL so TLO stays below 500 µs. Off: drive LOW for ≥3 ms (TOFF up to 2.5 ms). Reset the cached level whenever EN was low ≥0.8 ms or V3V was removed (GPIO15 low on battery). Brightness level n gives n × 1.25 mA per LED (≈5 mA total per level).

### 4. Panel sleep before ESP32-S3 deep sleep

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory.ino:395-404 and examples/Sleep/Sleep.ino:77-117 @1cc41eb

**Licence:** MIT

**Port notes:** Use esp_lcd_panel_disp_sleep(panel, true) (SLPIN plus a 100 ms wait) instead of raw 0x10 with no delay. Order: AW9364 off → SLPIN → GPIO15 low (battery) → rtc_gpio_hold/gpio_hold on CS6, RD9 and RST5 → enable wake on GPIO0/GPIO14 (ext1 for two pins) → esp_deep_sleep_start(). On wake, release the holds before esp_lcd reconfigures the pins.

### 5. Flip-clock screen layout (two 140×140 boxes, 120 px digits, blinking colon, tileview pages)

**Source:** Xinyuan-LilyGO/T-Display-S3 examples/factory/factory_gui.cpp:23-80,137-149 and factory_gui.h:3-6 @1cc41eb

**Licence:** MIT for the code. font_Alibaba.c holds glyph data from an Alibaba typeface whose licence is not stated in the repo.

**Port notes:** LVGL 8 to 9: lv_obj_clear_flag becomes lv_obj_remove_flag, lv_msg becomes lv_subject/observer, lv_scr_act becomes lv_screen_active. Replace font_Alibaba with your own lv_font_conv font (digits plus colon ≈25 KB at 120 px, 4 bpp) and Vietnamese label fonts. Update only on minute change, not every 100 ms as the factory loop does.

### 6. Orientation reference values (MADCTL per rotation and 35 px CGRAM offsets for 170-px panels)

**Source:** Xinyuan-LilyGO/T-Display-S3 lib/TFT_eSPI/TFT_Drivers/ST7789_Rotation.h:23-27,57-61,68,91-95,102,124-128,135 @1cc41eb

**Licence:** TFT_eSPI licence (MIT/BSD mix, lib/TFT_eSPI/license.txt)

**Port notes:** Use only as reference numbers. Implement with esp_lcd_panel_swap_xy/mirror/set_gap(0,35), or esp_lvgl_port rotation flags with the gap set first.

## Open questions

- What do the undocumented CMD_1 entries D0h A7 and D6h A1 do, and which controller variant (ST7789V, V2, V3 or P3) is fitted to the glossy and the matte panel revisions?
- Does CMD_1 help or hurt the matte ('frosted') panel? mmMicky says it is 'not required' there. Is the #300 white border/ghost on matte panels caused by the missing SLPIN, by floating control pins, or by the panel itself?
- Does the panel honour RAMCTRL little-endian (esp_lcd data_endian = LITTLE) on every revision? LilyGO never uses it; i80 swap_color_bytes is the safer zero-CPU option until this is tested.
- Is FPC pin 19 (unconnected on the board) the panel's TE output? If so, could a bodge wire to a free GPIO give tear-free updates?
- What draws the remaining ~0.22–0.24 mA in deep sleep (LDO quiescent current, charger, floating LCD bus inputs, PSRAM)? Do held LCD pin levels reduce it, and which RST level (high or low) is best during sleep?
- What 'mosaic' prompted the 2023-03-03 comment 'Too low or too high pixel clock may cause screen mosaic' (adc038e)? No issue documents a failure at low clock rates.
- What pulse widths does the factory setBrightness (back-to-back digitalWrite) actually produce, and are pulses below the AW9364's 0.5 µs minimum dropped by its deglitch filter?
- Root cause of the intermittent colour flip in #118, and of LilyGo-Display-IDF issue #8 (slow LVGL under ESP-IDF).
- Which typeface and licence is behind examples/factory/font_Alibaba.c?
- Do the 'two versions of the screen' (#122) really differ in RGB/BGR order, or is the confusion fully explained by TFT_eSPI writing LCMCTRL=0x0C (my inference)?
- How does the AW9364 behave if EN is already high when V3V rises (presumably it starts at level 16)? The datasheet does not say.
