# Alert hardware: buzzer or haptics

Date: 2026-10-20

Method: multi-agent research workflow: one research agent, then an adversarial verifier that re-checked each critical claim and the recommendation against primary sources

Status: verified: 10 confirmed / 0 partially / 0 refuted; the verifier says the recommendation holds

Notes: the roadmap (docs/roadmap.md) gives the Pomodoro/Timer a visual alert and lists a passive piezo on GPIO21 only as optional later hardware; haptics are not planned.

## Research report

### 1. Header GPIOs (v1.2, non-touch)

LilyGO says v1.2 changed only the charger IC ([#236](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/236)). The 2024-04-29 [schematic](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf) matches the [pinmap](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/image/T-DISPLAY-S3.jpg).

- **Left header (P2), antenna end first:** 3V3, 1, 2, 3, 10, 11, 12, 13, NC, NC, GND, 5V
- **Right header (P1):** GND, GND, 43, 44, 18, 17, 21, 16, NC, GND, GND, 3V3
- **JST-SH 4-pin "STEMMA QT/Qwiic"** ([product](https://lilygo.cc/products/t-display-s3)): GND, 3V3, 43 (SDA position), 44 (SCL position)

The 3V3 pins are the always-on LDO output and GPIO15 does not switch them (maintainer, [#352](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/352)). The README limits their load to 100 mA. Not exposed: flash/PSRAM (26–37), USB (19/20) and strapping pins 0/45/46 (BOOT key, LCD D4/D5).

| GPIO | RTC | Caveats ([ESP32-S3 datasheet v2.2](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf), schematic) | Alert pin? |
|---|---|---|---|
| 1, 2, 10 | yes | ADC1/touch; 60 µs low glitch at power-up | OK (2 is the fallback) |
| 3 | yes | Strapping (JTAG select; ignored with default eFuses) | avoid |
| 11–13 | yes | SD-shield pins | OK if no shield |
| 16 | yes | 100 kΩ pull-up on the board | avoid |
| 17, 18 | yes | 10 kΩ pull-ups, fitted on non-touch boards too ([#313](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/313)); I2C bus | keep for I2C |
| 21 | yes | No pull resistor, no analog function, no power-up glitch. Also goes to LCD FPC pin 26 (TP_RESET); assumed unused on the non-touch panel, UNVERIFIED | **best** |
| 43, 44 | no | UART0: ROM boot log on 43 at every boot; Qwiic lines | avoid |

### 2. Options

Pads drive 20 mA by default, and 40 mA only at maximum drive strength (datasheet Table 5-4). Anything with a coil therefore needs a transistor.

| Option | Parts | Current | Notes |
|---|---|---|---|
| **Passive piezo, direct drive** | piezo + 470 Ω | capacitive, <1 mA | Murata: can be driven directly from an IC through a 470 Ω–1 kΩ series resistor; avoid long DC ([FAQ](https://www.murata.com/en-us/support/faqs/products/sound/sounder/char/sch0001)). PKLCS1212E4001-R1: 84 dB typ / 75 dB min at 10 cm, ±1.5 Vo-p 4 kHz ([Murata](https://www.murata.com/en-global/products/productdetail?partno=PKLCS1212E4001-R1)), about one GPIO swing |
| Bridge drive (2 pins in antiphase) | +470 Ω | same | about +6 dB ([Same Sky](https://www.sameskydevices.com/blog/buzzer-basics-technologies-tones-and-driving-circuits)) |
| Magnetic buzzer (passive or active) | NPN/N-FET + flyback diode | >20 mA, inductive (Same Sky) | Active types play one fixed tone |
| ERM coin motor | [AO3400A](https://www.aosmd.com/res/datasheets/AO3400A.pdf), 100 kΩ gate pull-down ([AB-001](https://www.precisionmicrodrives.com/ab-001)), diode | 90 mA running, 150 mA at start ([PMD 310-103](https://www.precisionmicrodrives.com/product/310-103-10mm-vibration-motor-3mm-type)) | Over the 100 mA 3V3 guidance |
| DRV2605L + motor | I2C on 17/18, address 0x5A | 4 µA in standby, 0.5 mA if left active ([TI](https://www.ti.com/lit/ds/symlink/drv2605l.pdf)) | Adafruit board's power LED stays on unless a trace is cut ([Adafruit](https://learn.adafruit.com/adafruit-drv2605-haptic-controller-breakout/pinouts)) |

**Wiring:** header pin **21 → 470 Ω → piezo → G**.

**Parts:** one external-drive piezo transducer around 4 kHz (for example PKLCS1212E4001-R1), one 470 Ω resistor, and a sound hole in the case. Don't buy a coil-type "passive buzzer". At about 19.5 nF (distributor figure, UNVERIFIED), the RC time constant is about 9 µs, far below the 125 µs half-period, so the resistor costs no loudness.

### 3. Deep sleep

- **Current:** a piezo has no DC path, so it adds about 0 µA. The SoC draws 7–8 µA (Table 5-10). One user measured the whole board at about 263 µA ([#289](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/289), UNVERIFIED).
- **Before sleep:** call `ledc_stop(…, 0)`, drive the pin LOW, then call `gpio_hold_en(21)`. On RTC pins this calls `rtc_gpio_hold_en()` ([gpio.c](https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_gpio/src/gpio.c)), and the hold lasts through deep sleep.
- **After wake:** IDF releases holds automatically only on wake-up pins ([sleep_gpio.c](https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_hw_support/sleep_gpio.c)). Initialise LEDC with duty 0, then call `gpio_hold_dis(21)`.
- **Never hold HIGH.** That leaves DC across the piezo, and a motor or active buzzer would keep running.
- **Alternative:** `rtc_gpio_isolate()` is fine if there is an external pull-down ([rtc_io.h](https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_gpio/include/driver/rtc_io.h)).
- **Pins 43/44:** these would need `gpio_deep_sleep_hold_en()`, after which IDF asserts that the sleeping task's stack is in internal RAM.
- **Pins 17/18:** holding them LOW leaks 330 µA through the pull-ups.
- **When to beep:** LEDC stops in deep sleep. Wake on the RTC timer, beep before initialising the LCD and LVGL, then sleep again.

### 4. Software

- **LEDC setup:** low-speed mode; the S3 has 4 timers and 8 channels ([soc_caps](https://github.com/espressif/esp-idf/blob/v5.5.6/components/soc/esp32s3/include/soc/soc_caps.h)). Give the buzzer its own timer and channel. In [#248](https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/248), a shared channel let `tone()` blank the backlight. LilyGO's [factory code](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/examples/factory/factory.ino) now dims the AW9364 with pulses instead.
- **Tones:** 10-bit resolution, APB clock, `ledc_set_freq()` for each tone. Duty 512 is on and 0 is a gap. Stay between 3 and 4.5 kHz: below about 3 kHz this part is 10–20 dB quieter (datasheet curve).
- **Patterns:** a `{hz, on_ms, off_ms}` table stepped by `esp_timer`, in a module with no LVGL dependency.
- **IDF versions:** the API is the same in v5.5.6 and v6.1 ([ledc.h](https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_ledc/include/driver/ledc.h)). Since v6.0, `esp_driver_ledc` no longer re-exports `esp_driver_gpio` ([migration](https://github.com/espressif/esp-idf/blob/v6.1/docs/en/migration-guides/release-6.x/6.0/peripherals.rst)), so declare `REQUIRES esp_driver_ledc esp_driver_gpio esp_timer`.
- **Components:** Espressif publishes no buzzer component. The community ones are thin wrappers with a single maintainer: [supcik/buzzer](https://components.espressif.com/components/supcik/buzzer) 3.0.1 (C++, MIT, IDF ≥5.3), hayschan/buzzer, cdsama/buzzer. Write about 100 lines in the repo instead, using IDF's `ledc_basic` and [`rmt/musical_buzzer`](https://github.com/espressif/esp-idf/tree/v6.1/examples/peripherals/rmt/musical_buzzer) examples as references (the latter supports the S3).
- **Haptics:** [espp/drv2605](https://components.espressif.com/components/espp/drv2605) 1.3.8 or [lewisxhe/sensorlib](https://components.espressif.com/components/lewisxhe/sensorlib) 0.4.1.

The schematic, pinmap and datasheets were downloaded to a temporary folder only so they could be read. No project files were created or changed.

## Recommendation

Use a passive (external-drive) piezo transducer such as Murata PKLCS1212E4001-R1, wired from header GPIO21 through a 470 Ω series resistor to GND, and drive it with its own LEDC timer and channel at about 3–4.5 kHz. It needs no transistor, diode or extra supply, and adds essentially no deep-sleep current. Before esp_deep_sleep_start(), stop LEDC, drive GPIO21 LOW and call gpio_hold_en() (an RTC hold on the S3); on wake, call gpio_hold_dis() only after LEDC is set up again with duty 0. Avoid GPIO16/17/18 (on-board pull-ups; keep 17/18 for I2C), 43/44 (UART0 boot log, not RTC, Qwiic lines) and 3 (strapping), and switch to GPIO2 if GPIO21 turns out to be loaded through the LCD FPC. Move to an ERM motor with an AO3400A, or a DRV2605L on I2C 17/18, only if silent alerts become a requirement, accepting 60–150 mA bursts and more parts.

## Open questions

- Is LCD FPC pin 26 (TP_RESET, wired to GPIO21) connected to anything inside the non-touch panel? UNVERIFIED. Check GPIO21's resistance to GND and to 3V3 with the board unpowered, and scope it with the LCD on and off. GPIO2 is the fallback.
- How loud is it in the real enclosure? The datasheet minimum of 75 dB at 10 cm is about 55 dB at 1 m by inverse-square. This needs a prototype.
- The piezo capacitance (19.5 nF) comes only from distributor listings, not Murata.
- The lifecycle and availability of PKLCS1212E4001-R1 are unconfirmed: Murata's product page is not machine-readable and the datasheet copy is dated 2020-12.
- How accurately will a Pomodoro end on time after deep sleep? GPIO15/16 (the XTAL_32K pins) are used for PWR_EN and TP_INT, so no 32 kHz crystal can be fitted. IDF docs say the internal RC drifts with temperature but give no figure. Consider CONFIG_RTC_CLK_SRC_INT_8MD256 (about +5 µA) and measure the drift.
- The board's deep-sleep floor is uncharacterised: the ~263 µA figure is a single user report (issue #289). The schematic labels the LDO 'AP2112K-3.3V(RT9013)', and which part is actually fitted, with its quiescent current, is unknown.
- Are the 16/17/18 pull-ups fitted on every v1.2 production batch? The only evidence is a 2025 maintainer statement; no BOM has been published.
- If a DRV2605L is chosen, which bus is better: header I2C on 17/18 (pull-ups already fitted) or the Qwiic port on 43/44? On the Qwiic port, the ROM boot log toggles SDA at every boot, which is untested with the DRV2605L. It is also unverified whether the ROM print can be disabled without burning eFuses.

## Critical claims and verification

### C1: confirmed

**Claim:** Exposed GPIOs are: left header 1, 2, 3, 10, 11, 12, 13; right header 43, 44, 18, 17, 21, 16; the 4-pin JST-SH 1.0 mm connector is GND, VDD3V3, GPIO43 (U0TXD), GPIO44 (U0RXD). The header 3V3 pins are the always-on LDO output.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf ; https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/image/T-DISPLAY-S3.jpg ; https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/352

**Why it matters:** Defines the candidate alert pins and the always-on supply that any external alert circuit would use.

**Evidence:** Schematic T_Display_S3.pdf (title block 4/29/2024). P2, pins 12 to 1: VDD3V3, GPIO1, GPIO2, GPIO3, GPIO10, GPIO11, GPIO12, GPIO13, NC, NC, GND, +5V. P1, pins 12 to 1: GND, GND, U0TXD, U0RXD, II2C_SDA, II2C_SCL, ESP_TP_RESET, ESP_TP_INT, NC, GND, GND, VDD3V3. U2 pins 22-27: XTAL_32K_N=ESP_TP_INT (GPIO16), GPIO17=SCL, GPIO18=SDA, GPIO21=ESP_TP_RESET. CNC (WAFER-100W-4A), pins 1-4: GND, VDD3V3, U0TXD, U0RXD. The pinmap JPG matches ('G 3V 43 44'). The LilyGO product page says 'STEMMA QT / Qwiic, JST-SH 1.0mm 4-PIN'. VDD3V3 is the AP2112K-3.3 output, with EN tied high through R11. V3V is VDD3V3 switched by Q3 and does not reach the headers. Maintainer lewisxhe in #352 (2026-10-13): 'The pin header is connected directly to the LDO output and cannot be controlled'. The README was fixed the same day (commit 'Fix docs #352') and says the 3V header load must not exceed 100 mA.

### C2: confirmed

**Claim:** T-Display-S3 v1.2 differs from earlier revisions only in the charger IC (TP4065). The schematic was updated on 2024-04-29 in response to the request for a v1.2 schematic.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/236

**Why it matters:** Allows the published schematic and the 2022 pinmap to be applied to the user's v1.2 board.

**Evidence:** #236: lewisxhe (2024-04-29) wrote 'There is no other difference except that the charging chip has changed.' #230: lewisxhe (2024-03-05) wrote 'TP4065 is used instead of TP4056'. Commit b0a5ecd796 (2024-04-29), 'Update schematic https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/236', is the latest change to the schematic file. The new sheet shows U6 = TP4065 and is dated 4/29/2024. Caveat: this rests on the maintainer's statement, and the sheet carries no 'V1.2' revision label.

### C3: confirmed

**Claim:** GPIO17 (SCL) and GPIO18 (SDA) have 10 kΩ pull-ups and GPIO16 (TP_INT) has a 100 kΩ pull-up to VDD3V3. The maintainer confirms they are fitted on the non-touch board.

**Source:** https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/main/schematic/T_Display_S3.pdf ; https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/313

**Why it matters:** Rules these pins out for the alert: holding one LOW in deep sleep leaks about 330 µA (33 µA on GPIO16), and a floating pin is pulled HIGH, which would switch on a driver or put DC on a piezo.

**Evidence:** The schematic shows RIIC 10K on II2C_SCL (GPIO17), RIID 10K on II2C_SDA (GPIO18) and RINT 100K on ESP_TP_INT (GPIO16), all to VDD3V3, at FPC CN1 pins 27-29. By contrast, the ESP_TP_RESET net (GPIO21) appears only at U2 pin 27, CN1 pin 26 and P1 pin 6, with no resistor. In #313 a user with a non-touch board reported 16/17 floating high. The maintainer replied 'Pull-up resistors are already added to the 16 and 17', said they can be removed if touch is not used, and later (2025-11-05) marked the third resistor, for GPIO18. 3.3 V/10 kΩ = 330 µA and 3.3 V/100 kΩ = 33 µA.

### C4: confirmed

**Claim:** On the ESP32-S3, GPIO0–21 are RTC IOs. Strapping pins are GPIO0, 3, 45 and 46; GPIO3 is ignored with default eFuses. GPIO21 has no default pull, no analog function and is absent from the power-up glitch table. GPIO43/44 are VDD3P3_CPU (non-RTC) pads, and the ROM prints its boot messages on UART0 by default.

**Source:** https://documentation.espressif.com/esp32-s3_datasheet_en.pdf (v2.2: Tables 2-1, 2-2, 2-6, 3-1, 3-5; Section 3.3)

**Why it matters:** This is the basis for choosing GPIO21: no boot chirp, no leakage path, RTC-holdable and not tying up an ADC or touch channel.

**Evidence:** ESP32-S3 datasheet v2.2 (PDF dated 2026-03). Table 2-1: GPIO21 (pin 27) is powered by VDD3P3_RTC, has nothing under At Reset or After Reset (no pull, no IE) and has no Analog entry. U0TXD/U0RXD are VDD3P3_CPU with WPU. Table 2-6 lists RTC_GPIO0 to RTC_GPIO21. Table 2-8's analog functions stop at RTC_GPIO20. Table 2-2 lists glitches on GPIO1-14, XTAL_32K_P/N, 17, 18, 19 and 20, but not GPIO21. Table 3-1 lists strapping pins GPIO0, 3, 45 and 46. Table 3-5 shows GPIO3 'Ignored' with all-zero eFuses. Section 3.3 says ROM messages print by default to 'UART0 and USB Serial/JTAG controller'.

### C5: confirmed

**Claim:** gpio_hold_en() on an RTC-capable GPIO calls rtc_gpio_hold_en(), and the hold persists through deep sleep. Digital pads (GPIO26–48) need gpio_deep_sleep_hold_en(), after which IDF asserts that the task calling esp_deep_sleep_start() has its stack in internal RAM. On wake, IDF releases holds automatically only on wake-up pins.

**Source:** https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_gpio/src/gpio.c ; https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_gpio/include/driver/gpio.h ; https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_hw_support/sleep_gpio.c ; https://github.com/espressif/esp-idf/blob/v5.5.6/components/soc/esp32s3/include/soc/soc_caps.h

**Why it matters:** Makes holding the alert pin LOW in deep sleep simple and glitch-free on GPIO21, and avoids the PSRAM-stack pitfall that GPIO43/44 would bring.

**Evidence:** v5.5.6: gpio.c gpio_hold_en() calls rtc_gpio_hold_en() when rtc_gpio_is_valid_gpio(). gpio.h note 3 says that for ESP32/S2/C3/S3/C2 a digital GPIO needs gpio_deep_sleep_hold_en(). soc_caps sets SOC_GPIO_VALID_DIGITAL_IO_PAD_MASK = 0x0001FFFFFC000000 (GPIO26-48). In sleep_gpio.c, esp_sleep_isolate_digital_gpio() fires the assert 'If hold digital IO, the stack of the task calling esp_deep_sleep_start must be in internal ram!' only when deep-sleep digital hold is enabled. esp_deep_sleep_wakeup_io_reset() runs from cpu_start.c after a deep-sleep reset and unholds only EXT1 wake pins; deep-sleep GPIO-wakeup pins exist only on other chips. v6.1 has the same logic: the assert now uses esp_cpu_get_sp() and the unhold runs from ESP_SYSTEM_INIT_FN. On this board, the #352 reporter held GPIO15 LOW through deep sleep and the screen and green LED stayed off. Extra not in the claim: v6.1, unlike v6.0.x and v5.5.6, also logs 'Deep sleep requests are not allowed from tasks with stacks in PSRAM' when CONFIG_SPIRAM and CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM are both set. In that case esp_deep_sleep_try_to_start() returns ESP_ERR_NOT_ALLOWED.

### C6: confirmed

**Claim:** Murata states that an external-drive piezo sounder can be driven directly from an IC through a series resistor of about 470 Ω–1 kΩ. Murata also advises designing so that no DC voltage stays on the element for long periods.

**Source:** https://www.murata.com/en-us/support/faqs/products/sound/sounder/char/sch0001

**Why it matters:** Justifies driving the piezo without a transistor, and holding the pin LOW (0 V across the piezo) during sleep.

**Correction:** Add Murata's caveat: a parallel Zener or TVS is optional but advised 'if necessary'. So 'no diode' is acceptable, but it is not Murata's unconditional advice.

**Evidence:** Murata FAQ sch0001, case 2: '...it can be driven directly from an IC, though... please add a resistor in series between IC output and sound component... (Note: Approx. 470 to 1kΩ would be preferable.)' It also says: 'Design the circuit in such a way that a DC voltage is not applied to the piezoelectric sound component for a long period.' The figure shows IC, then resistor, then sounder, then GND, which is exactly the proposed wiring. The researcher left out that Murata also says to 'consider connecting a Zener diode in parallel with the piezoelectric sound component and Rp if necessary' to protect the IC from back-EMF.

### C7: confirmed

**Claim:** Murata PKLCS1212E4001-R1 is an external-drive SMD piezo sounder (12×12×3 mm, 4.0 kHz). It gives 84 dB typical and 75 dB minimum at 10 cm with a ±1.5 Vo-p 4 kHz square wave, and accepts up to ±12.5 Vo-p.

**Source:** https://www.murata.com/en-global/products/productdetail?partno=PKLCS1212E4001-R1 (Murata Product Search Data Sheet, last updated 12/12/2020; copy read at https://static.chipdip.ru/lib/101/DOC032101147.pdf)

**Why it matters:** Sets the expected loudness from a single 3.3 V GPIO swing, which roughly matches the measurement condition.

**Correction:** The specs are right, but the example part reaches end of life in 2027 (Murata is exiting the piezo-sounder business). Name an active non-Murata 4 kHz external-drive piezo transducer instead, or buy spares before 2027-03-31.

**Evidence:** Murata's live PIM page (rendered 2026-10-20): 12.0×12.0×3.0 mm, 4.0 kHz, external drive, SMD, '84dB Typ. [±1.5Vo-p,4.0kHz,square wave,10cm]', maximum input ±12.5 Vo-p. The 12/12/2020 Product Search Data Sheet copy (chipdip) adds '75dB (min.)'. Its frequency-response curve peaks near 3.2 and 4.2 kHz and is roughly 63-70 dB below 3 kHz. A 0-3.3 V GPIO swing (3.3 Vp-p) is close to the ±1.5 Vo-p (3 Vp-p) test condition. Important: the same Murata page marks the part 'To be discontinued', with Last Time Order Due 3/31/2027, End of Production 9/30/2027 and no alternative listed. Murata notice PSE-24GA-0083 (Mar 1, 2025) announces withdrawal from the whole piezo-sounder business (all PKM pin-type, PKHPS, PKLCS and PKMCS series), with last shipment 3/31/2028. DigiKey lists the part as Last Time Buy.

### C8: confirmed

**Claim:** Magnetic buzzers run at more than 20 mA and are inductive, so a flyback diode is required with a transistor switch. A full-bridge drive doubles the peak-to-peak voltage across a piezo transducer, adding about 6 dB.

**Source:** https://www.sameskydevices.com/blog/buzzer-basics-technologies-tones-and-driving-circuits

**Why it matters:** Supports rejecting magnetic buzzers for a direct GPIO drive, and gives a no-extra-parts loudness upgrade (two antiphase GPIOs).

**Evidence:** Same Sky blog by Nick Grillone, undated. It says: 'Magnetic buzzers operate at lower voltages and higher currents (1.5~12 V, > 20 mA)'. It models a magnetic buzzer as 'a coil in series with a resistor' (it never uses the word 'inductive') and says 'The diode is required to clamp the fly-back voltage created when the switch (transistor) is shut off quickly.' On full-bridge drive: 'the peak-to-peak voltage applied across the transducer is twice as large as the available supply voltage', giving 'an increase in the sound volume of about 6 dB'. This is a vendor application blog, not a datasheet.

### C9: confirmed

**Claim:** ESP32-S3 default pad drive is 20 mA (10 mA on GPIO17/18). The 40 mA IOH figure applies only at PAD_DRIVER=3. A 3 V coin ERM (PMD 310-103) draws up to 90 mA running and 150 mA at start.

**Source:** https://documentation.espressif.com/esp32-s3_datasheet_en.pdf (Table 2-1 note 5, Table 5-4) ; https://www.precisionmicrodrives.com/product/310-103-10mm-vibration-motor-3mm-type

**Why it matters:** Shows why motors and magnetic buzzers need a MOSFET or BJT while a capacitive piezo does not.

**Evidence:** ESP32-S3 datasheet v2.2, Table 2-1 note 5: 'GPIO17 and GPIO18: 10 mA; GPIO19 and GPIO20: 40 mA; All other pins: 20 mA'. Table 5-4: IOH 40 mA typ at VOH ≥ 2.64 V with PAD_DRIVER = 3, and IOL 28 mA at PAD_DRIVER = 3. The Precision Microdrives 310-103 page gives rated 3 V, max 3.6 V, max rated current 90 mA and max start current 150 mA.

### C10: confirmed

**Claim:** The LEDC API used here (ledc_timer_config, ledc_channel_config, ledc_set_freq, ledc_set_duty/ledc_update_duty, ledc_stop with an idle level) is present in both IDF v5.5.6 and v6.1. Since v6.0, esp_driver_ledc no longer re-exports esp_driver_gpio.

**Source:** https://github.com/espressif/esp-idf/blob/v6.1/components/esp_driver_ledc/include/driver/ledc.h ; https://github.com/espressif/esp-idf/blob/v5.5.6/components/esp_driver_ledc/include/driver/ledc.h ; https://github.com/espressif/esp-idf/blob/v6.1/docs/en/migration-guides/release-6.x/6.0/peripherals.rst

**Why it matters:** The alert module works unchanged whichever IDF version is chosen, provided its CMake REQUIRES lists esp_driver_ledc and esp_driver_gpio.

**Evidence:** Both the v5.5.6 and v6.1 driver/ledc.h declare ledc_channel_config, ledc_timer_config, ledc_update_duty, ledc_stop(speed_mode, channel, idle_level), ledc_set_freq and ledc_set_duty. In v5.5.6, ledc.h does #include "driver/gpio.h" and CMake has 'REQUIRES esp_driver_gpio # IDF-11989: Remove this in IDF v6.0'. In v6.1, CMake has REQUIRES esp_hal_ledc and PRIV_REQUIRES esp_driver_gpio, and ledc.h no longer includes gpio.h. The v6.0 peripherals migration guide says: 'Removed esp_driver_gpio as a public required component from esp_driver_ledc.' The same guide also removes ledc_timer_set, LEDC_APB_CLK_HZ and LEDC_USE_RTC8M_CLK, and deprecates ledc_channel_config_t::intr_type. Code copied from older examples that sets .intr_type therefore builds with a deprecation warning on v6.x.

## Verifier's assessment of the recommendation

The core design holds. GPIO21 is the best exposed alert pin. On the board, its net reaches only the SoC, header P1 pin 6 and FPC pin 26, with no pull resistor. It is not a strapping pin, has no power-up glitch and no analog function, and it can be held through deep sleep. Murata explicitly endorses driving the piezo directly from an IC through 470 Ω to 1 kΩ. The piezo adds no DC load in sleep, and the LEDC and hold APIs behave as described in both IDF v5.5.6 and v6.1. Three changes are needed:

(1) Do not standardize on PKLCS1212E4001-R1. Murata is leaving the piezo-sounder business (last order 2027-03-31, end of production 2027-09-30). Either buy spares now or use an active equivalent. One example is Mallory/Sonalert AST1240MLTRQ; per DigiKey it is Active, external-drive, 4 kHz, 12×12×3.3 mm and 75 dB at 3 V and 10 cm (its rating condition differs, so check loudness on the bench).

(2) Change the tone settings from 'APB clock' to XTAL_CLK. IDF's LEDC docs list APB_CLK on the S3 as not compatible with dynamic frequency scaling (DFS). The LEDC driver takes no power-management (PM) lock, and its default sleep mode (NO_ALIVE_NO_PD) stops output in light sleep. If esp_pm (DFS or automatic light sleep) is ever enabled, an APB-clocked beep can shift in pitch or be cut off. Hold an ESP_PM_NO_LIGHT_SLEEP lock for the length of a beep pattern.

(3) 'No diode' is acceptable but goes slightly beyond Murata. Murata suggests a Zener across the piezo 'if necessary', which is cheap insurance on a handheld that may be dropped.

The sleep sequence is consistent with the IDF source: stop LEDC at idle 0, drive the pin LOW, then gpio_hold_en (an RTC hold); on wake, set up LEDC at duty 0, then gpio_hold_dis. The fallback pins and the decision to keep the ERM and DRV2605L only for silent alerts are reasonable.

Whether GPIO21 is loaded through the non-touch panel's FPC is still unverified. Weak evidence suggests it is free: in #318 the maintainer implies 16 and 21 are usable on non-touch boards, and the #352 reporter used GPIO21 as a key. Measure it on the real board.

## What the research missed (verifier)

1) Part end of life (most important). Murata's product page marks PKLCS1212E4001-R1 'To be discontinued' (last order 3/31/2027, end of production 9/30/2027, no alternative listed). Notice PSE-24GA-0083 covers every Murata PKM pin-type, PKHPS, PKLCS and PKMCS piezo sounder, with last shipment 3/31/2028. DigiKey shows Last Time Buy. Any Murata substitute has the same problem.

2) LEDC clock and power management. On the ESP32-S3, IDF documents APB_CLK as not DFS-compatible; XTAL_CLK is DFS-compatible, and RC_FAST is both DFS- and light-sleep-compatible. The LEDC driver takes no PM lock. With 10-bit resolution, XTAL at 40 MHz still covers 3-4.5 kHz.

3) Timer accuracy in deep sleep. The board cannot take a 32 kHz crystal because XTAL_32K_P/N are used as GPIO15 (PWR_EN) and GPIO16 (TP_INT). Pomodoro alarms that fire from deep sleep therefore run on the internal RC slow clock, which IDF says drifts with temperature. IDF's 8MD256 option is more stable at about +5 µA. Resyncing over NTP or calibrating may be needed.

4) IDF v6.1 deep-sleep check. v6.1 adds a PSRAM-stack check to deep-sleep entry (it logs an error; esp_deep_sleep_try_to_start returns ESP_ERR_NOT_ALLOWED). Enter deep sleep from a task whose stack is in internal RAM, not from an LVGL task with a PSRAM stack.

5) ERM supply. Per the schematic, the header '5V' pin is the +5V power-path node: VBUS through the IN5819 diode on USB, or VBAT through Q4 (SI2307) on battery. It is the only header rail that can feed an ERM's 90-150 mA without breaking the README's 100 mA limit on 3V3. At up to 4.2 V, though, it exceeds the 310-103's 3.6 V maximum, so limit the motor with PWM.

6) Small point: the researcher's 19.5 nF piezo capacitance is still unverified (I found no Murata figure). Even 28 nF, the figure for the sibling E2400 part, gives a time constant of about 13 µs, far below the 125 µs half-period, so the conclusion stands.

Process note: the Playwright browser tool wrote 4 log files to a new folder in the working directory while I checked the Murata page; I deleted that folder. All downloaded sources went only to a temporary folder.
