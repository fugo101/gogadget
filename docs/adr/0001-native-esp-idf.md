# Build on native ESP-IDF v6.1 with idf.py

Gogadget builds with ESP-IDF v6.1 directly (`idf.py` + the IDF Component Manager, toolchain installed by EIM), not with Arduino and not through PlatformIO. The previous project (zen-clock) used PlatformIO's packaging of IDF and paid for it: the CMake build ran inside SCons glue, IDF patch releases arrived late or never (PlatformIO skipped IDF 6.0.2 and 6.0.3), and problems had to be debugged through two build systems. Native IDF gets every Espressif bugfix release, the official docs and examples apply verbatim, and the LilyGO code we learn from is ported by hand anyway (nothing upstream builds on IDF 6.x).

## Considered options

- **Arduino core 3.x**: lags IDF by a major version (Arduino 3.3.12 is IDF 5.5.5) and hides the esp_lcd/sleep APIs Gogadget needs.
- **PlatformIO `framework = espidf`** (platform-espressif32 8.0.0 ships IDF 6.1.0): convenient `pio project init`, but the packaging and patch-release gaps above are exactly what made zen-clock hard to maintain.

## Consequences

- Only `firmware/sdkconfig.defaults` (and its fragments) is committed; the generated `sdkconfig` is gitignored, so configuration intent stays reviewable.
- Component versions are pinned in `idf_component.yml` and locked by a committed `dependencies.lock`.
- v6.1 has no bugfix release yet (2026-10-20). Move to v6.1.x patch releases deliberately, after reading their notes for esp_lcd, sleep and USB-Serial-JTAG changes.
