# Hardware bring-up and acceptance tests

How to put a new T-Display-S3 unit into service and prove that each board driver works on it.

- The hardware facts behind each test are in [t-display-s3.md](t-display-s3.md).
- The power sequences and the meter-free checks are in [power-budget.md](power-budget.md).

**When to run:**

- New unit: the first flash, the per-unit log and every test below.
- After changing a board driver: that driver's tests.
- Before a release: the smoke set DSP-1, DSP-4, BL-2, BTN-3, PWR-1 and USB-3 (5 trials).

Record every result in the unit's log ([Per-unit log](#per-unit-log)).

## Summary

| ID | What | Pass in one line |
|---|---|---|
| DSP-1 | Colour bars and 1-px frame, USB left | Frame complete, colours in order, no mirroring, black is black |
| DSP-2 | Same, rotated 180° (USB right) | Same as DSP-1 |
| DSP-3 | Cold boot after >= 60 s unpowered | Nothing visible before the first complete frame |
| DSP-4 | Deep-sleep wake | Old frame never visible; wake-to-time <= 0.5 s |
| DSP-5 | `esp_restart()` | Old frame never visible after the screen goes dark; no noise |
| DSP-6 | Long sleep soak (matte unit preferred) | No white border or ghost after 1 h and 8 h |
| DSP-7 | Pixel-clock margin (optional) | No artefacts at 10 MHz |
| BL-1 | 16 levels | Monotonic, no wrap, no flicker; random transitions land on the right level |
| BL-2 | No flash at boot with USB | No light before the first frame |
| BL-3 | Off after >= 3 ms EN LOW; level cache | Dark when off; correct level after re-enable and after a V3V loss |
| BTN-1 | Idle levels and positions | Both read 1 at idle, 0 pressed; positions recorded |
| BTN-2 | Debounce and events | One event per press; hold = Back on KEY only |
| BTN-3 | Wake from deep sleep | Every press wakes; right pin; normal boot; press consumed |
| BTN-4 | Sleep with a button held | No sleep while held; no re-wake after release |
| BTN-5 | Strapping (documentation) | BOOT + RST enters download mode |
| PWR-1 | Green LED in deep sleep on battery | Off |
| PWR-2 | Overnight drain | Decline matches < 1 mA average; no reset |
| PWR-3 | Endurance | >= 7 days to 3.6 V VSYS at default settings |
| USB-1 | Plug in, awake on battery | No reset; always-on entered |
| USB-2 | Plug in, asleep | Green LED on at once; always-on at the next wake |
| USB-3 | Unplug, awake | Reset reason logged; time never wrong without the badge |
| USB-4 | USB-C-to-C charger | Recorded (no pass/fail) |
| USB-5 | Console without a reader | Wake-to-time unchanged |
| RTC-1 | 24 h drift, RC vs 17.5 MHz/256 | ppm measured; source chosen |

## Equipment

- A USB-A charger or computer port and a USB-A-to-C cable. These work whatever the CC resistors on the board.
- Optional, for USB-4 only: a USB-C PD charger and a C-to-C cable.
- The protected 250 mAh cell with a 2-pin 1.25 mm plug. Check its polarity against the +/- marks on the board before the first connection.
- A phone camera on a stand: photos with exposure and focus locked, and slow-motion video (240 fps, about 4 ms per frame).
- A host with ESP-IDF v6.1 (esptool 5.x, esp-idf-monitor >= 1.10).
- No multimeter. The power checks use the methods in [power-budget.md](power-budget.md#verifying-power-without-a-multimeter).

The tests need two firmware hooks:

- a diagnostic pattern (from a test build or a diagnostic screen);
- a way to call `esp_restart()`.

They also use the console commands `bl`, `power.stay`, `sys.info` and `log.dump` (ADR-0016). Keep the board awake with `power.stay` while a test needs the console.

## First flash

The unit arrives running zen-clock (the previous project), or LilyGO's factory image. Before Gogadget's first flash, back up the whole 16 MB flash, then erase it.

- The backup is the only way back to the old firmware.
- The backup holds the old NVS, which may contain Wi-Fi credentials. It stays in the gitignored `backups/` directory: never commit it, attach it to an issue or upload it.
- Erasing is required: an old NVS (LilyGO's factory image keeps Wi-Fi settings at 0x9000) would otherwise survive `idf.py flash`.

Run from the repository root, in a shell where the ESP-IDF v6.1 environment is active (EIM's activation script, as the Makefile uses). esptool 5.x uses hyphenated commands; the older `read_flash` spelling still works with a deprecation warning.

```sh
# 1. Enter download mode by hand: hold BOOT, press and release RST, release BOOT.
#    (Asleep, the board has no USB device; in download mode it always has one.)

# 2. Find the port. macOS is shown; on Linux it is usually /dev/ttyACM0.
ls /dev/cu.usbmodem*
PORT=/dev/cu.usbmodemXXXX            # replace with the device listed above
UNIT=gg-01                           # your name for this board
BACKUP="backups/${UNIT}-zen-clock-$(date +%Y%m%d).bin"

# 3. Identify the chip; copy the output (chip revision, features, flash ID and size) into the unit log.
esptool --chip esp32s3 -p "$PORT" flash-id

# 4. Back up the full 16 MB flash (0x1000000 bytes) into the gitignored backups/ directory.
mkdir -p backups
git check-ignore -v backups/         # must print the .gitignore rule; stop if it prints nothing
esptool --chip esp32s3 -p "$PORT" read-flash 0 0x1000000 "$BACKUP"

# 5. Check the backup against the chip, then record its SHA-256 (not the file) in the unit log.
esptool --chip esp32s3 -p "$PORT" verify-flash 0 "$BACKUP"
shasum -a 256 "$BACKUP"

# 6. Erase the whole chip: removes the old app, its NVS and any stored credentials.
esptool --chip esp32s3 -p "$PORT" erase-flash
#    (equivalent: idf.py -C firmware -p "$PORT" erase-flash)

# 7. Build and flash Gogadget, then watch the first boot
#    (or: make fw; make flash PORT="$PORT"; make monitor PORT="$PORT").
idf.py -C firmware build
idf.py -C firmware -p "$PORT" flash monitor
```

- **If esptool cannot connect**, repeat the BOOT + RST sequence. After flashing in manual download mode, press RST to start the app (#16, #162).
- **Restore the old firmware** if ever needed: `esptool --chip esp32s3 -p "$PORT" write-flash 0 "$BACKUP"`, with `BACKUP` set to the saved file.
- **Sanity check.** If the display shows nothing, or the board misbehaves in ways the tests below cannot explain, flash LilyGO's no-touch factory image from T-Display-S3:firmware/ as a hardware sanity check. Erase again before returning to Gogadget.

## Per-unit log

One file per unit, for example `docs/hardware/units/<unit>.md`.

- Omit full MAC addresses and anything taken from the backup.
- Photos of the markings help later; put them next to the log.

```markdown
# Unit <name>

| Field | Value |
|---|---|
| Received (date, seller: lilygo.cc or a distributor) | |
| SKU (H569 without pins, H577 soldered) | |
| Silkscreen revision and date code ("22-8-2 V1.2", "25-9-12 V1.3", ...) | |
| Panel finish (glossy / matte) | |
| LDO U7 marking (AP2112K / RT9013 / RT9080 / other) | |
| Charger U6 marking ("65b..." = TP4065; SOP-8 = TP4056) | |
| R13 marking (expected 2 kΩ; "unmarked" if none) | |
| R10 health: R_path from the VSYS sag (Ω), first reading after a full charge (mV) | |
| Chip revision and features (esptool flash-id banner) | |
| Flash ID and size (esptool flash-id) | |
| MAC, last 3 bytes only | |
| Backup file name and SHA-256 (the file stays in backups/) | |
| USB-A charger + A-to-C cable: powers (green LED) / charges (red LED) | |
| USB-C-to-C charger: powers / charges (USB-4) | |
| Cell: capacity, protected, plug polarity checked | |
| First charges: temperature and swelling notes (ADR-0010) | |
| Antenna: onboard / external (0201 resistor moved) | |
| Button positions with USB on the left (expected: GPIO0 upper-left, GPIO14 lower-left) | |
| RTC clock source chosen and drift (RTC-1) | |

## Results

| Test | Date | Firmware commit | Result | Notes |
|---|---|---|---|---|
| DSP-1 | | | | |
```

## Display

Background: [t-display-s3.md](t-display-s3.md#display-panel-st7789v). Film with the phone in slow motion wherever "film" is written, then step through the video frame by frame.

### DSP-1: colour bars and 1-px frame, USB left

Pattern, in logical 320x170 landscape:

- black background;
- a 1-px white frame on x = 0, x = 319, y = 0 and y = 169;
- left to right, bars of pure red (0xF800), pure green (0x07E0), pure blue (0x001F), white and 50% grey;
- an asymmetric marker, for example a white "F", 10 px inside the top-left corner.

Pass:

- **Gap.** All four frame lines are visible end to end, with no missing or doubled edge row or column. A missing top or bottom line means the gap is off by one; one user needed 36 instead of 35 (#160).
- **Colour order and byte swap.** The bars appear red, green, blue in that order. Red may look orange (#113) but must not look blue; noise or wrong colours point to the byte swap.
- **Inversion.** Black is black and white is white. Inversion off shows a negative image.
- **Orientation.** The marker reads correctly (not mirrored) in the top-left corner, with the USB port on the viewer's left.
- Keep the photo in the unit log. It also serves the one-time palette approval on the real panel.

### DSP-2: the same, rotated 180°

Select the 180° setting (USB on the right) and reboot; the setting applies at boot.

Pass: the same as DSP-1. The marker is at the top left as the viewer sees it, and the frame is complete with the same gap.

### DSP-3: cold boot

Remove USB and the battery for at least 60 s; GRAM decays to noise after about a minute (#232). Reconnect power and film. Run it once on USB and once on battery.

Pass:

- From power-on to the first frame, nothing is visible: no noise, no white flash, no partial frame.
- The first visible image is the complete first frame, and the backlight comes on only after it.

### DSP-4: deep-sleep wake

Show a distinctive frame A (for example the time with a large "A"). Let the board sleep for at least 1 min, press KEY and film. Repeat 20 times.

Pass:

- Frame A never reappears; the first visible image is the new complete frame.
- Wake-to-time is at most 0.5 s, measured from the video frame where the button goes down to the first frame showing the time. Record the median and the worst case. Above 0.5 s, the planned fallback is the reset-free wake ([t-display-s3.md](t-display-s3.md#reset-free-wake)).

### DSP-5: esp_restart()

Show frame A with the backlight on, call `esp_restart()` and film. Repeat 20 times, on USB and on battery.

- This resets the panel while it is in Sleep Out, the case where the datasheet's 120 ms rules are tightest ([t-display-s3.md](t-display-s3.md#panel-timing-rules)).
- The backlight goes dark within about 2.5 ms of EN falling LOW.

Pass:

- After the screen goes dark, the next visible image is the new complete frame.
- No stale frame A, no noise, no blank panel with the backlight on, no wrong gamma.

### DSP-6: long sleep soak

Run on battery, preferably on a matte unit, since only matte panels ghosted in #300. Sleep for 1 h, wake and inspect. Then sleep overnight (at least 8 h), wake and inspect.

Pass: no white border, smear or ghost of the previous image.

Optional comparison: repeat with the #300 parking (every LCD pin LOW) and record which variant ghosts. See [power-budget.md](power-budget.md#pin-parking).

### DSP-7: pixel-clock margin (optional)

Run a full-screen moving gradient for 10 min at 10 MHz.

Pass: no "mosaic", corruption or broken lines. An unsynchronised full frame may show one tear boundary, because the board has no TE line. If artefacts appear, step the clock down (8 MHz, 6.67 MHz), never up (ADR-0006).

## Backlight

Background: [t-display-s3.md](t-display-s3.md#backlight-aw9364).

### BL-1: 16 monotonic levels

Fill the screen white. With the phone on a stand and exposure and focus locked, run `bl 1` to `bl 16`, taking a photo at each level, then `bl 16` back to `bl 1`.

Pass:

- Brightness never decreases as the level rises, and never increases as it falls.
- Levels are visibly distinct at least up to about 12. The top levels may compress at V3V ≈ 3.3 V [I]; record where they do.
- `bl 0` is dark.
- No visible flicker at any steady level.
- No wrap-around: going from 15 to 16 must never jump to the dimmest level.

Then script 200 random level changes and finish on 6. Pass: the final photo matches the level-6 reference, so no pulse was lost or added.

### BL-2: no flash at boot with USB

Work in a dark room with USB connected (V3V is on from power-up). Film each case:

- cold plug-in;
- an RST press;
- `esp_restart()`;
- a deep-sleep wake.

Pass: no backlight light before the first frame. The AW9364's EN pull-down holds it off while GPIO38 floats during reset.

### BL-3: off after >= 3 ms EN LOW; level cache

Run `bl 6`, `bl 0`, `bl 6`.

Pass:

- The screen is dark while off (check in a dark room).
- After re-enable, brightness matches the level-6 reference. The driver must restart from 16 and step down 10 pulses.

On battery, also force a V3V loss while lit: drive GPIO15 LOW for at least 5 ms, as the USB-test dip does. Pass: the level afterwards matches the reference.

## Buttons

Background: [t-display-s3.md](t-display-s3.md#buttons).

### BTN-1: idle levels and positions

With internal pulls disabled, log GPIO0 and GPIO14 at idle and while pressed.

Pass:

- Both read 1 at idle (the board pull-ups are present) and 0 while pressed.
- Record which physical button is which with USB on the left. Expected: GPIO0 upper-left, GPIO14 lower-left.

### BTN-2: debounce and events

Make 50 short presses on each button, then 10 holds on each.

Pass:

- Exactly 50 press events per button: no doubles from bounce on GPIO0 and no missed releases on GPIO14.
- A KEY hold gives Back, not Next.
- BOOT gives Enter on a press and nothing extra on a hold (ADR-0008).

### BTN-3: wake from deep sleep

Wake the board from deep sleep 20 times with KEY and 20 times with BOOT.

Pass:

- Every press wakes the board.
- `esp_sleep_get_ext1_wakeup_status()` names the pin that was pressed.
- The board boots normally. BOOT never enters download mode on a deep-sleep wake, as expected: a wake is a Core Reset and strapping is latched only at a Chip Reset.
- The waking press is consumed: no Next or Enter action follows.

LilyGO never woke on GPIO0 or through ext1, so this test is the first evidence for this unit.

### BTN-4: sleep with a button held

Hold KEY past the glance timeout, release it, and repeat with BOOT.

Pass:

- The board does not sleep while the button is held.
- It sleeps at least 50 ms after release and stays asleep, with no immediate re-wake (ext1 is level-triggered).

### BTN-5: strapping (documents expected behaviour)

Hold BOOT, press and release RST, release BOOT.

Pass: the board is in ROM download mode (`esptool flash-id` connects). A later RST press runs the app. This confirms the caveat: holding BOOT at a chip reset enters download mode, so no feature may ask the user to hold BOOT.

## Sleep current

No meter: these tests bound the current from observable effects ([power-budget.md](power-budget.md#idle-drain-test)). Run them on battery with USB unplugged; on USB, VBUS powers everything.

### PWR-1: green LED in deep sleep

Put the board to sleep on battery and look at the green LED.

Pass: it is off. If it glows, V3V is on (+0.5-0.9 mA): the GPIO15 parking or its pull-up is wrong.

### PWR-2: overnight drain

Follow step 1 of the idle-drain test: full charge, 1 h rest, 10-12 h of deep sleep with hourly VSYS logging.

Pass:

- The VSYS decline is consistent with well under 1 mA average.
- The log shows no reset during the night.

### PWR-3: endurance

From a full charge, run the default settings: backlight 6/16, normal glances and scheduled syncs. Log every wake.

Pass: at least 7 days until VSYS (no load) first reads below 3.6 V. Record the days and compare with the model, about 12 days.

### PWR-4: with a meter (optional, if one is ever available)

Measure on the battery lead with USB unplugged:

- the sleep floor, against the 243-263 µA references;
- both parking variants;
- the current at each backlight level.

Then update the model inputs in [power-budget.md](power-budget.md#inputs).

## USB plug and unplug

Background: [power-budget.md](power-budget.md#usb-plug-and-unplug).

### USB-1: plug in while awake on battery

Pass:

- No reset: uptime continues and the reset reason is unchanged.
- The green LED turns on, and the red LED too if the cell is not full.
- Firmware reports USB and enters always-on within its check interval.

### USB-2: plug in while asleep

Pass:

- The green LED lights at once and the device stays asleep.
- At the next button press, or the next scheduled Background wake, it enters always-on.

### USB-3: unplug while awake

Run in always-on mode, 20 trials for each case:

- Wi-Fi active with the backlight at 16;
- Wi-Fi off with the backlight at 6.

Run each case at full charge and at about half charge. For each trial, record:

- whether the board reset;
- `esp_reset_reason()` on the next boot;
- whether the shown time was right after the next sync (compare with NTP);
- what the face showed meanwhile.

Pass:

- The clock never shows a wrong time without the "unsynced" badge.
- Every boot logs its reset reason.
- Whether resets happen at all, and whether the RTC time survives them, is recorded: this answers an open question (#174).

### USB-4: USB-C-to-C charger

Power the board through a C-to-C cable from a PD charger, and from a laptop if one is available. Record in the unit log:

- whether the green LED lights (VBUS present);
- whether the red LED lights (charging).

There is no pass or fail. The schematic shows no CC resistors, so failure is possible.

### USB-5: console without a reader

Measure wake-to-time three ways:

- with the USB port open in a terminal that does not read;
- with no host at all (battery);
- with a normal monitor attached.

Pass: wake-to-time stays the same within 50 ms. USB-Serial-JTAG drops output after one stall of at most 50 ms, so logging must not slow a glance.

## RTC drift

### RTC-1: 24 h drift, RC vs 17.5 MHz/256

The choice between the two deep-sleep clock sources rests on this test (ADR-0011).

Builds, on the same unit and in the same place:

- **A:** `CONFIG_RTC_CLK_SRC_INT_RC` (the default).
- **B:** `CONFIG_RTC_CLK_SRC_INT_8MD256`, about +5 µA.

Procedure:

1. Sync with NTP, then disable scheduled syncs in the test build.
2. Run 24 h on battery with a realistic duty: about 50 timer-wake glances a day. The warm-die calibration at each sleep entry depends on the duty.
3. Sync again. Before applying the new time, log the offset (t_ntp - t_rtc) and the elapsed time. Drift in ppm is offset / elapsed x 10^6.
4. Run each source twice on different days, once on a desk and once warm (in a pocket or near a window).

Decision rule:

- Choose the source with the smaller worst-case |ppm|. If they are equal, prefer the RC, which costs 5 µA less.
- Pass for ±1 min with 4 syncs a day: worst-case raw drift at most 2,800 ppm, with the drift-learning residual within ±60 s over 6 h.
- Also report the 24 h error, after which the "unsynced" badge appears. At most 690 ppm keeps ±1 min for a whole day without a sync.

Record the choice and both ppm figures in the unit log, and carry the decision into ADR-0011.
