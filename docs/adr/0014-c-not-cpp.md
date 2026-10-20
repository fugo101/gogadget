# Write C, not C++

All Gogadget code is C, limited to the subset of C23 that both the ESP-IDF 6.1 toolchain (GCC 15) and Apple clang compile, because ESP-IDF, LVGL and esp_lvgl_port are C APIs, the host simulator must compile the same sources with a different compiler, and the code that is worth learning from (ESP-IDF examples, LVGL docs) is C. C++ would mainly add RAII and templates, at the cost of a second language, exceptions/RTTI configuration and harder-to-read compiler errors for AI-generated code. Host builds pass `-funsigned-char` because plain `char` is unsigned on Xtensa.
