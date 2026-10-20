#!/usr/bin/env bash
# Regenerate ui/fonts/*.c from the vendored sources in tools/fonts/src.
# The generated files are committed and never edited by hand (docs/ui/design.md).
# Usage: tools/fonts/generate.sh   (needs Node.js; installs the pinned lv_font_conv)
set -euo pipefail

# Run from tools/fonts with relative paths: lv_font_conv records its options in
# each generated file, and absolute paths would differ between machines.
cd "$(dirname "$0")"
out="../../ui/fonts"
src="src"

npm ci --silent --no-audit --no-fund
conv="node_modules/.bin/lv_font_conv"

# Printable ASCII + every precomposed Vietnamese letter (Latin-1, Latin Extended-A/B
# subsets and Latin Extended Additional U+1EA0-1EF9) + degree sign and middle dot.
vi="0x20-0x7E,0xB0,0xB7,0xC0-0xC3,0xC8-0xCA,0xCC-0xCD,0xD2-0xD5,0xD9-0xDA,0xDD,0xE0-0xE3,0xE8-0xEA,0xEC-0xED,0xF2-0xF5,0xF9-0xFA,0xFD,0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9"

# Font Awesome glyphs behind LVGL's LV_SYMBOL_* macros that the UI uses:
# ok, close, settings, home, refresh, left, right, warning, charge, bell, wifi,
# battery full/3/2/1/empty, usb.
symbols="0xF00C,0xF00D,0xF013,0xF015,0xF021,0xF053,0xF054,0xF071,0xF0E7,0xF0F3,0xF1EB,0xF240-0xF244,0xF287"

# Seven-segment digits for the clock: space, minus, dot, 0-9 and colon.
digits="0x20,0x2D-0x2E,0x30-0x3A"

text_font() { # size
    "$conv" --bpp 4 --size "$1" --no-compress --format lvgl --lv-include lvgl.h \
        --font "$src/BeVietnamPro-Medium.ttf" -r "$vi" \
        --font "$src/FontAwesome5-Solid+Brands+Regular.woff" -r "$symbols" \
        --lv-font-name "gg_font_text_$1" -o "$out/gg_font_text_$1.c"
}

digit_font() { # size
    "$conv" --bpp 4 --size "$1" --no-compress --format lvgl --lv-include lvgl.h \
        --font "$src/DSEG7Classic-Bold.ttf" -r "$digits" \
        --lv-font-name "gg_font_digits_$1" -o "$out/gg_font_digits_$1.c"
}

mkdir -p "$out"
text_font 16
text_font 20
digit_font 28
digit_font 70
echo "fonts written to ui/fonts"
