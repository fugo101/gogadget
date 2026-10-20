# Gogadget

A pocket gadget built on a LilyGO T-Display S3: a battery-powered glance clock first, with more personal tools (timers, calendar, provisioning, weather) added over time.

## Power and modes

**Standby**:
The state between uses, with the screen dark and the device in deep sleep.
_Avoid_: idle, off

**Glance**:
A short period (10 s by default, extended while buttons are used) in which the screen is lit after a button wakes the device on battery, before it returns to Standby.
_Avoid_: peek, wake screen

**Always-on**:
The mode while USB power is present: the screen stays lit and the device never enters Standby on its own (Back on the Watchface still does).
_Avoid_: desk mode, plugged mode

**Background wake**:
A scheduled wake with the screen dark, used for a Sync, after which the device returns to Standby.
_Avoid_: silent wake

**Wake press**:
The button press that ends Standby; it only wakes the device and triggers no action.

**USB session**:
The period during which USB power is present.

**Brightness level**:
One of 16 backlight steps, 1 dimmest and 16 brightest; 0 means the backlight is off.

**Battery level**:
The estimated remaining charge of the cell, shown only on battery; on USB the device shows that it is powered, never "charging" or "full".

## Time and calendars

**Sync**:
Setting the device clock from NTP, either in a Background wake, during a USB session, or on request.
_Avoid_: update, refresh

**Unsynced**:
The condition after 24 hours without a successful Sync, or after an unexpected reset, shown as a badge on the Watchface.

**Drift**:
How much the clock gains or loses between two Syncs.

**Solar date**:
The Gregorian date (dương lịch).
_Avoid_: western date

**Lunar date**:
The date in the Vietnamese lunisolar calendar (âm lịch), computed in Vietnam's time zone, including leap months.
_Avoid_: Chinese date

**Leap month**:
An extra month inserted in some lunar years (tháng nhuận); it repeats the number of the month before it.

**Can Chi**:
The sexagenary name of a lunar year, month or day, such as Bính Ngọ.

## Screens and navigation

**Screen**:
One full-display view: the Watchface, the Menu, or a feature.
_Avoid_: page, view, activity

**Watchface**:
The home Screen: time, Solar date, Lunar date and the Status bar.
_Avoid_: home screen, clock screen

**Status bar**:
The indicators along the top edge of the Watchface: Battery level or USB power, Wi-Fi and Sync state, and a running timer.

**Menu**:
The list of features, opened from the Watchface with Enter.

**Next / Back / Enter**:
The three navigation actions: Next moves to the following item (lists wrap), Back returns to the previous Screen (on the Watchface it starts Standby), Enter selects.
_Avoid_: up/down, prev

**KEY / BOOT**:
The two physical buttons. KEY gives Next (press) and Back (hold); BOOT gives Enter (press only).

## Development

**Golden screenshot**:
A committed reference image of a Screen rendered from a fixed state; any pixel difference fails the check.

**Dev console**:
The command interface over the USB cable that developers and AI agents use to drive and inspect the device.

**Provisioning boot**:
A dedicated boot, entered by holding KEY at power-up, in which the device accepts Wi-Fi credentials over Bluetooth.
