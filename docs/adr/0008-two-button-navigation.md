# Navigate with two buttons: KEY press is Next, KEY hold is Back, BOOT press is Enter

The board has two user buttons, GPIO14 (KEY) and GPIO0 (BOOT). KEY press moves to the next item and KEY hold goes back; BOOT press selects (Enter). There is no Previous: lists wrap. Back on the watchface puts the device to sleep. The press that wakes the device only wakes it and is not delivered as an event. BOOT gets only a short press because GPIO0 is the boot strapping pin: holding it during a power-on or brownout reset enters download mode, so no feature may teach the user to hold it. Gogadget uses its own ISR + state machine instead of `espressif/button`, whose quirks cost zen-clock time, and because the wake press and the hold timing need exact control.

## Consequences

- The input state machine lives in `core/` and is unit-tested with simulated time; the firmware only feeds it pin levels and timestamps.
- A KEY long press at boot is reserved for entering BLE provisioning later (ADR-0012).
- With USB on the right (rotated 180°), the physical positions of the buttons swap; on-screen hints must follow the orientation.
