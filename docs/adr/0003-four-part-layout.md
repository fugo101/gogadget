# Split the code into core, ui, firmware and sim, and let the host build enforce it

The repository has four parts with one-way dependencies: `core/` (pure C: state, policies, calendars; no `esp_*`, no LVGL), `ui/` (LVGL screens that render core state; depends on LVGL and core only), `firmware/` (the ESP-IDF project: board drivers, services, composition root) and `sim/` (a plain host CMake project that builds core and ui with LVGL for tests, golden screenshots and an optional SDL window). Inside an IDF build every component implicitly sees FreeRTOS, logging and system headers, so nothing would stop UI code from reaching into the platform again, which is how zen-clock's UI ended up impossible to build off-device. Compiling core and ui on the host, where no IDF headers exist, turns that rule into a compile error instead of a code-review convention.

## Considered options

- **ESP-IDF `linux` target**: experimental, `esp_lcd` and `esp_lvgl_port` do not build there, and SDL needs the main thread on macOS.
- **QEMU for ESP32-S3**: no LCD_CAM (i80) or GPIO emulation, a different panel driver, no Espressif support.

## Consequences

- `core/CMakeLists.txt` and `ui/CMakeLists.txt` build both ways (`idf_component_register` when `ESP_PLATFORM` is set, a plain library otherwise).
- Hardware-facing services reach core through small interfaces with one firmware adapter and one host fake each.
