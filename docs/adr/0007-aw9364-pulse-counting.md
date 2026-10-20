# Drive the AW9364 backlight by pulse counting, never PWM

The backlight driver is an AW9364, a 16-level current sink controlled by counting pulses on its EN pin (GPIO38): EN high for at least 20 µs starts it at full brightness, each rising edge steps one level down (wrapping from 1 back to 16), and EN low for more than 2.5 ms shuts it off. Driving EN with LEDC PWM, as most T-Display S3 code still does, makes the chip count every PWM low phase as a step, so brightness becomes erratic; LilyGO itself called that method wrong in 2024 (#278) and removed it in 2026 (02f0e9d). Gogadget's own driver emits explicit 1-2 µs pulses inside a critical section (a low phase stretched past 500 µs would be read as a shutdown) and caches the level, invalidating the cache whenever the chip may have reset.

## Consequences

- Each level costs about 5 mA (about 80 mA at 16/16), so the backlight level dominates glance energy; the battery default is 6/16.
- EN has an internal pull-down, so GPIO38 needs no hold in deep sleep.
- On USB the backlight supply (V3V) is forced on by VBUS; only EN can turn the backlight off.
