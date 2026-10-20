# Hand-write the UI and verify it with golden screenshots

Screens are written by hand in C (no SquareLine, EEZ or other designer tools, which are paid or generate code nobody can maintain) and verified by rendering fixed states in the headless host build with LVGL's test module and comparing them pixel-for-pixel with committed reference PNGs. This lets an AI agent change UI code and see the result as an image without hardware, and lets CI reject unintended visual changes. In CI, missing references fail the run (`LV_TEST_SCREENSHOT_CREATE_REFERENCE_IMAGE` off); updating references is an explicit local step whose PNG diff a human reviews.

## Consequences

- The simulator renders RGB565 exactly like the device because the byte swap happens in the i80 hardware (ADR-0006), not in LVGL.
- The simulator cannot show panel-specific effects: colour shift (pure red renders orange), gap or orientation off-by-one, tearing, stale frames. A short on-device check (colour bars, 1-px frame, cold boot vs wake) covers those, and the palette is approved once on the real panel.
