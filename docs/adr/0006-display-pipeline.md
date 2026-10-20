# Drive the panel at 10 MHz from internal DMA buffers, with the vendor init table and an explicit DISPON

The ST7789V runs over esp_lcd i80 with the built-in ST7789 driver at a 10 MHz pixel clock, from two 320x40 partial draw buffers in internal DMA-capable RAM, with the RGB565 byte swap done by the LCD_CAM hardware (`swap_color_bytes`). The panel's 8080 write cycle must be at least 66 ns (about 15 MHz), so LilyGO's 16, 20 and 40 MHz settings are out of spec, and Espressif pairs PSRAM draw buffers with a 2 MHz clock because of PSRAM bandwidth; zen-clock combined 17 MHz with PSRAM buffers and chased display artefacts with folklore rules. A full frame still takes only about 11 ms at 10 MHz.

The bring-up order is fixed: drive RD (GPIO9) high (esp_lcd never touches it; floating RD gives a black or garbage screen), reset and init, invert on, gap (0,35), send LilyGO's 14-entry vendor init table byte-for-byte (without it "the pixels will be blurred"), render the first frame, then send DISPON ourselves (the IDF ST7789 init stopped sending it in 5.0), and only then turn the backlight on (GRAM keeps the previous frame through reset and sleep, so an early backlight shows a stale time or noise).

## Consequences

- If artefacts ever appear, step the clock down (8 MHz, 6.67 MHz), never up.
- `esp_lcd_panel_set_brightness()` is never used: it writes a panel register this board ignores (ADR-0007).
- The decoded init table and every timing rule live in `docs/hardware/t-display-s3.md`; code comments cite it so nobody "cleans up" the vendor values.
