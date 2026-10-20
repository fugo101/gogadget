# Watchface v1 proposals

Rendered on 2026-10-20 from the same data (14:35:42, Thứ Ba 20/10/2026, lunar 11/9 Bính Ngọ, battery, Wi-Fi) by a throwaway prototype; `variants-sheet.png` shows all three.

- A, classic: HH:MM at 70 px with small seconds digits, full date line.
- B, calendar card: stacked hours and minutes, a calendar card with weekday, large day, month, lunar date and Can Chi.
- **C, minimal (chosen):** 88 px digits over faint seven-segment ghosts, seconds as a progress line, one compact date line.

The implemented face lives in `ui/src/ui_watchface.c`; its golden screenshots are in `ui/test/golden/`.
