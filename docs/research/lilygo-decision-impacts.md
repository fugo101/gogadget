# LilyGO deep dive: impact on the stack decisions

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive: seven parallel source sweeps of Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5, adversarial verification of the 24 top claims, then a synthesis agent that wrote this file)

Status: synthesis: 12 keep / 13 adjust / 1 reconsider

Notes: the owner has since settled several of these (docs/adr/): the 250 mAh cell stays on the stock ~525 mA charge current (about 2.1C, risk accepted); there is no multimeter, so power is verified with firmware logs; USB on the left is the default with a 180-degree setting; a USB plug-in during sleep is noticed at the next wake with no extra polling; accuracy is +-1 min with NTP about 4x/day on 1.ntp.vnix.vn and pool.ntp.org; the default backlight is 6/16; fonts are Be Vietnam Pro and DSEG7; the press that wakes the device is consumed (it does not count); local time is computed in core/ from the fixed UTC+7 offset instead of a TZ string; the coredump partition is 256 KB.

Superseded by decision: the button state machine is Gogadget's own (ADR-0008), so neither 'Code in C' nor 'Third-party code policy' carries OneButton code or its BSD notice (THIRD_PARTY_NOTICES.md has only the two LilyGO MIT notices); both buttons must read HIGH for at least 50 ms before sleep, not 30-50 ms; Wi-Fi is skipped below about 3.6 V VSYS, not 3.6-3.7 V.

The synthesis agent checked each decision taken before the deep dive against the LilyGO findings: **keep**, **adjust** (keep it, with the listed changes) or **reconsider**. Items marked NEW are decisions it proposed. Tags: [V] verified, [I] inference.

## Summary

| Decision | Verdict |
|---|---|
| Native ESP-IDF v6.1 + idf.py (no Arduino, no PlatformIO) | keep |
| LVGL 9.6.0 + esp_lvgl_port 2.9 | keep |
| esp_lcd i80 with the built-in ST7789 driver | adjust |
| Code in C | keep |
| Battery-first glance clock on a 250 mAh protected cell | reconsider |
| Deep sleep, wake on GPIO0/GPIO14 | keep |
| Show the time for 10 s | keep |
| Always-on while on USB | adjust |
| NTP time | adjust |
| Wi-Fi credentials via a USB console command | keep |
| BLE provisioning later | keep |
| Vietnamese UI, 320x170 landscape | keep |
| Own AW9364 pulse-count backlight driver | keep |
| Own two-button driver | keep |
| 10 MHz pixel clock with internal DMA draw buffers | keep |
| Headless LVGL screenshot tests in a desktop simulator | keep |
| NEW: RGB565 byte order in hardware (i80 flags.swap_color_bytes=1, esp_lvgl_port swap_bytes=0) | adjust |
| NEW: Display wake and sleep sequence | adjust |
| NEW: Deep-sleep pin parking and holds | adjust |
| NEW: sdkconfig baseline for fast, low-power wakes | adjust |
| NEW: GPIO4 battery and USB sensing procedure | adjust |
| NEW: Power-event resilience | adjust |
| NEW: Per-unit hardware log and bring-up checks | adjust |
| NEW: Third-party code policy | adjust |
| NEW: Pin LilyGO upstream by SHA and run a monthly drift check | adjust |
| NEW: Reset-free fast wake path (later optimisation) | adjust |

## Decisions

### Native ESP-IDF v6.1 + idf.py (no Arduino, no PlatformIO) (keep)

Nothing upstream targets IDF 6.x. The T-Display-S3 examples are Arduino 2.0.14-era; only the factory example builds on core 3.3.11, and it still uses fields that 6.1 removed. LilyGo-Display-IDF is IDF 5.3/LVGL 8, does not compile on 6.x, and its S3 path is unchanged since 2024-01-31. Gogadget has to own the port either way, and 6.1 brings what the clock needs: esp_sleep_enable_ext1_wakeup_io, the esp_sleep_get_wakeup_causes bitmap, i80 swap_color_bytes, esp_lcd_i80_alloc_draw_buffer, and a USB-Serial-JTAG console that returns EIO instead of blocking when no host is attached [V]. Budget for the 6.x removals: the legacy ADC and I2S drivers, wifi_provisioning, and warnings now treated as errors.

### LVGL 9.6.0 + esp_lvgl_port 2.9 (keep)

No LilyGO LVGL code is reusable anyway: the bundled LVGL is an 8.3.0-dev snapshot, and LDI's LVGL 8 glue has no 9.6 mapping. Verified esp_lvgl_port 2.9 behaviour to design around:
- lvgl_port_add_disp installs its own on_color_trans_done.
- It re-applies swap_xy/mirror from .rotation. Use {swap_xy 1, mirror_x 0, mirror_y 1} for USB on the left; LV_DISPLAY_ROTATION_180 then gives USB on the right.
- flags.swap_bytes is a CPU swap.
- buff_dma allocates MALLOC_CAP_DMA.
- lvgl_port_lock(0) waits forever.

Set CONFIG_LV_MEM_SIZE in bytes, and do not define LV_DISABLE_API_MAPPING, because esp_lvgl_port still calls the lv_draw_sw_* names.

### esp_lcd i80 with the built-in ST7789 driver (adjust)

Keep it, but it is not enough on its own. Required additions:
- Drive RD (GPIO9) HIGH before creating the bus (#103).
- Send LilyGO's 14-entry CMD_1 table byte-for-byte after init; the maintainer says pixels blur without it (#229).
- Call esp_lcd_panel_disp_on_off(true) yourself, after the first frame: since IDF 5.0 the init no longer sends DISPON [V].
- Keep CMD_1's SLPOUT + 120 ms. IDF sends SLPOUT 10 ms after reset, while datasheet Table 9 note 7 requires 120 ms [V].
- Invert on, gap (0,35), RGB order.

Never use esp_lcd_panel_set_brightness: its WRDISBV write does nothing on this board.

### Code in C (keep)

Everything worth reusing is small and ports to C trivially: the CMD_1 table, the bring-up order, the AW9364 step maths and the SNTP pattern. The C++ sources (LDI main.cpp, OneButton) are better rewritten; OneButton's state machine becomes a C struct that keeps its BSD notice.

### Battery-first glance clock on a 250 mAh protected cell (reconsider)

Do not pair a 250 mAh cell with the stock charger: its current is about 525 mA (spec 425-575 mA; TP4065 RPROG = 1050/I with R13 = 2 kΩ) [V], which is about 2C for 250 mAh. Either rework R13 (about 4.3 kΩ gives about 245 mA), or pick a protected cell rated for that current: about 500 mAh still fits the 47x25x7 mm shell cavity and nearly doubles runtime.

Expect about 95 µA that firmware cannot remove (R12 plus the LDO) and 243-263 µA measured in sleep. With 20 glances and one sync per day that is about 10-15 mAh/day, so 2.5-3.5 weeks on 250 mAh [I].

The R10 fuse can degrade; bridging it is fine with a protected cell. The connector is 1.25 mm MX1.25/PicoBlade style, so check the polarity.

### Deep sleep, wake on GPIO0/GPIO14 (keep)

Use ext1 ANY_LOW on BIT64(0)|BIT64(14) via esp_sleep_enable_ext1_wakeup_io. Both buttons have external pull-ups to the always-on rail, so internal pulls are not needed and RTC_PERIPH can be powered down, since ext1 works without it [V]. Before sleeping, require both lines HIGH for 30-50 ms, because GPIO14 has a 10 ms RC. Use esp_sleep_get_ext1_wakeup_status to tell which button woke the board. LilyGO only ever woke on GPIO14 via ext0, so test GPIO0 (a strapping pin) on hardware. Sending SLPIN before every sleep is mandatory: it cuts the panel from 6 mA to about 20 µA and prevents the #300 ghosting.

### Show the time for 10 s (keep)

The backlight level dominates each glance: about 0.17 mAh at level 4/16 versus about 0.37 mAh at full, since the AW9364 draws about 5 mA per level and 80 mA at full [I]. Choose a dim default on battery. GRAM keeps the stale time through sleep [V], so draw the new frame before DISPON and the backlight. Expect about 0.4-0.5 s from wake to first frame with a full panel init; a reset-free wake could later save about 0.23 s.

### Always-on while on USB (adjust)

USB shows up only on GPIO4: with GPIO15 LOW it reads about 0 V on battery and about 2.3 V on USB [V]. There is no VBUS or charge-status GPIO, and plugging in USB cannot wake the chip (a level wake on GPIO4 is marginal).

Define how always-on mode is entered: check at every wake, plus either an optional timer wake (about 2-3 µAh per check) or a ULP ADC poll (which keeps RTC_PERIPH on).

On USB:
- GPIO15 cannot switch V3V off, so the green LED stays lit; blank the screen with AW9364 EN only.
- Unplugging may reset the board (#174), so persist the time state.
- The schematic shows no CC resistors, so C-to-C chargers may not supply VBUS.
- Skip light sleep, because the USB-Serial-JTAG console fails in it.
- Shift the layout or dim it periodically to avoid image retention (#128) [I].

### NTP time (adjust)

Keep NTP and add a timekeeping policy. Deep-sleep time runs on the internal RC, because no 32 kHz crystal is possible. Evaluate RTC_CLK_SRC_INT_8MD256: the IDF docs say it is more stable for about +5 µA [V]; measure 24 h drift against INT_RC. Resync on every USB session, and about daily on battery when VSYS is healthy. Optionally learn a ppm correction from successive syncs [I].

Implementation:
- esp_netif_sntp with server_from_dhcp=false (#297) and two pool servers;
- a bounded wait, then esp_netif_sntp_deinit and esp_wifi_stop;
- TZ '<+07>-7' set on every boot, tested under PicolibC;
- never block the glance.

The v1.2 antenna is weak, so add retries and timeouts and log RSSI.

### Wi-Fi credentials via a USB console command (keep)

USB-Serial-JTAG is the right console: USB-C goes to native USB, and IDF's default UART0 console must be switched (#266). In IDF 6.1 its writes fail fast without a host [V], so logging cannot stall battery wakes. The console exists only while the board is awake, so provisioning happens in USB always-on mode. Terminals may reset the board through DTR/RTS (#274), so save credentials to NVS immediately. Erase the flash once, because the factory image leaves Wi-Fi config in NVS at 0x9000, or use WIFI_STORAGE_RAM. Set the auth threshold to WPA2 (#247) and mask SSIDs in the output.

### BLE provisioning later (keep)

The S3 is BLE-only (#200). IDF 6.0 removed wifi_provisioning, so plan on the espressif/network_provisioning managed component with NimBLE [V]. LilyGO has no provisioning example; its BLE MIDI code only shows the 31-byte advertising limit. Measure free internal RAM with Wi-Fi, NimBLE and LVGL running before deciding whether PSRAM is needed.

### Vietnamese UI, 320x170 landscape (keep)

Landscape costs nothing through MADCTL (swap_xy + mirror, gap 0/35) [V]. LilyGO's fonts (the Montserrat sizes) are ASCII-only, and font_Alibaba has no licence. Generate fonts with lv_font_conv covering the Vietnamese precomposed set: the Latin-1 and Extended-A/B subset plus U+1EA0-1EF9. Panel colours differ from the simulator (pure red shows as orange, #113), so choose the palette on hardware. Button hints must follow the chosen orientation.

### Own AW9364 pulse-count backlight driver (keep)

Confirmed as the only valid method (#278, 02f0e9d): the chip counts PWM low phases as dimming steps. Datasheet timing [V]:
- EN HIGH at least 20 µs to start; use 30 µs.
- Pulse high time at least 0.5 µs.
- Pulse low time 0.5-500 µs; use 1-2 µs inside a critical section.
- LOW for more than 2.5 ms turns the chip off; use 3 ms.
- 16 levels of 1.25 mA per LED.

EN has an internal 150 kΩ pull-down, so GPIO38 needs no deep-sleep hold. Invalidate the cached level after any shutdown or V3V loss. Port the factory step maths (MIT) with explicit pulse widths.

### Own two-button driver (keep)

GPIO0 has a 10 kΩ pull-up, no capacitor, and is a strapping pin, so it needs 20-30 ms of software debounce. GPIO14 has 100 kΩ + 100 nF, a 10 ms RC with a slow release. OneButton 2.0.4 (BSD; timings 50/400/800 ms) is a good state-machine reference. Start the button that woke the board in the DOWN state, so the waking press counts. Never leave a peripheral pin at a default that can claim GPIO0 (#101).

### 10 MHz pixel clock with internal DMA draw buffers (keep)

10 MHz is an exact divider (80 MHz/8) and stays within the ST7789V write-cycle limit of about 15.15 MHz [V]; LilyGO's 16, 20 and 40 MHz all exceed it. Espressif pairs PSRAM buffers with 2 MHz. Use two partial internal DMA buffers (e.g. 320x40 px, 25.6 KB each) through esp_lvgl_port buff_dma or esp_lcd_i80_alloc_draw_buffer, with max_transfer_bytes at least the buffer size. If artefacts appear, step down to 8 MHz (/10) or 6.67 MHz (/12), not up. This corrects the sweep suggestion to try 16 MHz.

### Headless LVGL screenshot tests in a desktop simulator (keep)

With the byte swap done in i80 hardware, LVGL renders plain RGB565 on both device and simulator, so screenshots compare 1:1. LVGL 9.6 has LV_USE_TEST_SCREENSHOT_COMPARE; turn LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE off in CI.

The simulator cannot catch:
- the panel colour shift (#113);
- gap or orientation off-by-one errors;
- tearing;
- stale-frame and backlight ordering.

Add a short on-device smoke test: RGB bars, a 1-px frame, cold boot versus wake.

### NEW: RGB565 byte order in hardware (i80 flags.swap_color_bytes=1, esp_lvgl_port swap_bytes=0) (adjust)

The LCD_CAM swizzle applies to colour transfers only. Command parameters are untouched because param_bits equals the bus width [V IDF 6.1 esp_lcd_panel_io_i80.c]. Community IDF ports used it (#103, zef). Use exactly one swap. Fallbacks in order: data_endian LITTLE (RAMCTRL; untested on this panel), then esp_lvgl_port swap_bytes (a CPU copy). This supersedes the sweep advice to enable swap_bytes.

### NEW: Display wake and sleep sequence (adjust)

Wake: GPIO15 HIGH (on battery), RD HIGH, bus/IO/panel, reset and init, invert and gap, CMD_1, LVGL first frame (lv_refr_now), DISPON, AW9364 ramp.

Sleep: AW9364 off (at least 3 ms), SLPIN (wait at least 5 ms, and never within 120 ms of SLPOUT), GPIO15 LOW, park the pins, deep sleep.

This removes boot noise and stale frames (#232, #300) and the 6 mA panel draw (#289). LDI and the factory both turn the backlight on too early.

### NEW: Deep-sleep pin parking and holds (adjust)

Pin states in deep sleep:
- GPIO15: LOW, pulls off, held. Never call gpio_reset_pin on it: that enables the pull-up, which turns V3V on [V].
- CS, RD, WR and RST: HIGH; DC: LOW. These are RTC pads, so gpio_hold_en is enough.
- D0-D7: LOW, with gpio_hold_en plus gpio_deep_sleep_hold_en. Never hold GPIO45/46 HIGH.
- GPIO38: LOW, no hold needed.
- GPIO16-18: leave untouched (they have board pull-ups).

Goal: recover part of the unexplained 120-190 µA [I]. Compare against the #300 all-LOW variant on a matte panel.

### NEW: sdkconfig baseline for fast, low-power wakes (adjust)

Baseline settings:
- PSRAM: CONFIG_SPIRAM off, or OCT mode with SPIRAM_MEMTEST=n. The memtest costs about 735 ms per boot (#4), and a quad PSRAM config boot-loops (#103).
- Flash: QIO, 80 MHz, 16 MB.
- Partitions: a custom partitions.csv with nvs, otadata, phy_init, 2 x 4 MB OTA, LittleFS storage and a 64 KB coredump.
- CPU and build: 160 MHz CPU, -O2 or -Os, FREERTOS_HZ 1000.
- Brownout detector on; boot and app log level WARN; esp_deep_sleep_disable_rom_logging().
- An explicit RTC clock source; optionally BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP.

LDI's sdkconfig is a template, not a reference.

### NEW: GPIO4 battery and USB sensing procedure (adjust)

Use adc_oneshot on ADC1_CH3 at 12 dB with curve-fitting calibration. Sample 3-5 ms after GPIO15 goes HIGH (10 nF behind 50 kΩ) and before the backlight or Wi-Fi load, take 16-32 samples, multiply by 2, and apply an optional per-unit trim. For the USB test, read with GPIO15 LOW; VSYS above about 4.35 V means USB. Label the value VSYS under load, not cell voltage, and show no percentage on USB. This replaces LilyGO's single-shot legacy read and its misleading message.

### NEW: Power-event resilience (adjust)

Unplugging USB may reset the board (#174). Wi-Fi peaks can brown out the 500 mA LDO, and R10 can degrade (#275, #291).
- Keep the brownout detector on and log esp_reset_reason on every boot.
- Store a time-valid flag and the last NTP epoch in RTC memory, and periodically in NVS.
- Show an 'unsynced' marker after an unexplained reset.
- On battery, start Wi-Fi only above about 3.6-3.7 V VSYS, with a dim backlight and capped TX power.
- Add a battery-path health check: the VSYS drop when the backlight steps up reveals the R10 series resistance.

### NEW: Per-unit hardware log and bring-up checks (adjust)

Batches differ:
- antenna: v1.2 vs v1.3;
- panel: glossy vs matte;
- LDO: AP2112K, RT9013 or RT9080;
- charger: TP4056 or TP4065;
- possibly the CC resistors.

For each unit, record:
- the silkscreen revision and date;
- the panel finish;
- the LDO and charger markings;
- the R13 value and R10 health;
- how it behaves on a C-to-C charger;
- the chip revision. Boards from 2022-23 report v0.1, so confirm IDF 6.1's minimum supported revision.

### NEW: Third-party code policy (adjust)

Allowed:
- MIT LilyGO files, with THIRD_PARTY_NOTICES and per-file headers. The notices are 'Copyright (c) 2022 Xinyuan-LilyGO' for TDS3 and 'Copyright (c) 2024 TTGO' / Shenzhen Xinyuan for LDI.
- The OneButton logic, with its BSD notice.
- CC0 Espressif examples, freely.

Never copy:
- the GPL ULP and audio examples;
- the unlicensed VolosR examples (PCBClock, PokerS3);
- font_Alibaba.c;
- krupis' unlicensed IDF port;
- llgok/lilygo_device_driver (GPL-3.0).

### NEW: Pin LilyGO upstream by SHA and run a monthly drift check (adjust)

There are no tags or releases, and hardware facts change mostly in README and schematic edits, often within days of an issue (#343, #352). Pin TDS3 at 1cc41eb and LDI at b1a1cc5, archive the schematic blob, and run tools/upstream-watch.sh monthly. Tested: exit 0 at the current pins; exit 10 with diffs when pinned at 02f0e9d.

### NEW: Reset-free fast wake path (later optimisation) (adjust)

After ESP_RST_DEEPSLEEP the panel has kept VDD3V3, its registers and GRAM, because SLPIN keeps memory [V datasheet]. Skipping reset, init and CMD_1 saves about 230 ms of wake time. Recipe:
- create the panel with reset_gpio_num = -1, so the driver does not drive RST;
- keep RST and CS held HIGH through sleep;
- draw while the panel is still in sleep-in;
- send SLPOUT and wait at least 5 ms;
- turn on DISPON, then the backlight.

Nobody has tested this. Prototype it only after the baseline works, and keep a full init for any other reset reason and periodically.

## Questions for the owner (with recommended answers)

- Battery and charging: the stock charger pushes about 525 mA, roughly 2C for a 250 mAh cell. Options:
  (a) Swap R13 from 2 kΩ to about 4.3 kΩ (about 245 mA).
  (b) Use a larger protected cell rated for at least 0.6 A charge, e.g. about 500 mAh within 47x25x7 mm.
  Recommended: (b) if the enclosure allows it (no soldering, about twice the runtime), otherwise (a). Do not charge a 250 mAh cell at the stock current unless its datasheet allows 2C.
- Measurement tools: the sleep floor, backlight current per level, R10 health and RTC drift all need measuring, and you have no multimeter. Will you buy one? Recommended: yes. Get a multimeter with a µA range and a JST 1.25 mm pass-through lead for sleep current, plus an inline USB-C power meter for awake and charging current. Add a Nordic PPK2 if the budget allows.
- Default orientation:
  - USB on the left (LilyGO default): GPIO0 upper-left, GPIO14 lower-left.
  - USB on the right: GPIO14 upper-right, GPIO0 lower-right.

  Recommended: support both as a setting. Default to USB-left unless the enclosure or cable routing favours the right. First confirm the button positions on your board; they are derived from a drawing.
- If USB is plugged in while the clock is in deep sleep, how quickly must always-on mode start? Options:
  - At the next button press: no extra cost.
  - A timer wake about every 15 min: about 0.25 mAh/day.
  - A ULP ADC poll: reacts within seconds, but needs more code and keeps RTC_PERIPH powered.

  Recommended for v1: next button press plus the 15-minute timer check. Use the ULP only if instant switching matters.
- Accuracy: what clock error between syncs is acceptable on battery? This decides the RTC clock source and the NTP schedule. Recommended: within ±30 s.
  - Use RTC_CLK_SRC_INT_8MD256 (about +5 µA) if a 24 h drift test shows it beats the default RC.
  - Sync on every USB session, plus about once a day on battery when VSYS is healthy.
- Default backlight on battery: each AW9364 level costs about 5 mA, and full brightness about 80 mA. Recommended: level 4 of 16 (about 20 mA) on battery, adjustable by the user, and brighter on USB.
- Which USB power source will run always-on mode? The schematic shows no CC resistors, so USB-C-to-C chargers may not power the board. Recommended: a USB-A charger with the A-to-C cable. Test your C-to-C charger once (green LED lit means VBUS is present) before considering a CC-resistor mod.
- Are small SMD mods acceptable? Candidates:
  - Remove R12: saves about 40 µA, about 15% of the sleep floor; its purpose is unknown.
  - Remove the green LED: keeps a bedside unit dark, and saves 0.25-0.6 mA while V3V is on.
  - Bridge R10, only if it has degraded.
  - Add 5.1 kΩ CC resistors.

  Recommended: none for now. Decide after measuring, and remove R12 only after asking LilyGO what it does.
- Future units: buy v1.3 (re-matched antenna) directly from lilygo.cc, SKU H569 without pins or H577 soldered? Recommended: yes, ask for v1.3 explicitly. Keep the current v1.2 as the development board and log its revision.
- Typeface for the Vietnamese UI and the big clock digits? Recommended: an OFL font with full Vietnamese coverage, such as Be Vietnam Pro, for the labels, plus a bold digits-only subset for the clock. Avoid font_Alibaba: its licence is unknown.
