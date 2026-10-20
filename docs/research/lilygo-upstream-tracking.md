# Tracking LilyGO upstream

Date: 2026-10-20

Method: multi-agent research workflow (LilyGO deep dive: seven parallel source sweeps of Xinyuan-LilyGO/T-Display-S3 @1cc41eb and Xinyuan-LilyGO/LilyGo-Display-IDF @b1a1cc5, adversarial verification of the 24 top claims, then a synthesis agent that wrote this file)

Status: synthesis; the watch script was tested on 2026-10-20 (exit 0 at the pins, exit 10 when pinned at 02f0e9d)

Notes: the settled decisions archive only LilyGO's MIT schematic in the repo; vendor datasheets (Sitronix, Awinic, TOPPOWER) are cited by link and SHA-256, not copied (docs/research/upstream/ is maintained separately). The IDF v6.1 value in section 1 (`4dc1c655...`) is the annotated tag object, not the commit: `git ls-remote` on 2026-10-20 gives `refs/tags/v6.1^{}` = `fff9895c82d744c7237be8847347bdd1b07c6643`, the commit stack-esp-idf.md quotes.

Superseded by decision: the committed `tools/upstream.lock`, `tools/upstream-watch.sh` and `.github/workflows/upstream-watch.yml` replace the drafts in sections 4 and 5 ([docs/upstream.md](../upstream.md) describes them). Among other fixes, the committed script's PDF filter no longer drops GPIO words, every error exits 2, it follows each repository's default branch, and the workflow comments on an open drift issue instead of opening a duplicate. Gogadget's toolchain pins sit in the component manifests under `ui/` and `firmware/` plus `firmware/dependencies.lock` (ADR-0002), not in `main/idf_component.yml`.

## 1. Pins

- **Xinyuan-LilyGO/T-Display-S3** at `1cc41eb1c76b2f3215130a3dd64e7138ce00b482` (2026-10-13). The repo has no tags, releases or wiki, so pin by the full SHA.
- **Xinyuan-LilyGO/LilyGo-Display-IDF** at `b1a1cc54994bf1b417e3bb30c437bbe1036bff7f` (2025-11-03). Its T-Display-S3 files are unchanged since `c465092` (2024-01-31).
- **Archive copies** go in `docs/research/upstream/`, because upstream overwrites files under the same names:
  - `schematic/T_Display_S3.pdf`: blob `318cc4f0ae28657991ca34f7980811c64c450756`, last changed in `b0a5ecd` on 2024-04-29.
  - `datasheet/ST7789V_SPEC_V1.4.pdf`.
  - The AW9364 V2.4 datasheet (doc.awinic.com).
  - The TP4065 REV2.0 datasheet (the toppwr.com URL in #230).
- **Gogadget's own toolchain pins** are recorded separately:
  - ESP-IDF v6.1: tag commit `4dc1c65503e98e9a6b4c3c5646237280ba79829b`.
  - `lvgl/lvgl` 9.6.0 and `espressif/esp_lvgl_port` 2.9.0, in `main/idf_component.yml` with a committed `dependencies.lock`.
  - Before bumping, review the IDF 6.1.x patch notes for esp_lcd, sleep and USB-Serial-JTAG.

## 2. What to watch

- **T-Display-S3, hardware truth:**
  - `schematic/`, `datasheet/`, `image/`, `dimensions/README.MD`.
  - `README.md` and `README_CN.md`: the Pinout, GPIO15 IMPORTANT, FAQ, LED, Wi-Fi antenna and Electrical sections. Corrections land here, often within days of an issue (#343, #352).
- **T-Display-S3, reference code:**
  - `examples/factory/factory.ino` and `pin_config.h`: CMD_1, setBrightness, sleep and SNTP.
  - `examples/Sleep/Sleep.ino` and `examples/GetBatteryVoltage/GetBatteryVoltage.ino`.
  - `examples/lv_demos/lv_demos.ino` and its `pin_config.h`; `examples/tft/tft.ino`.
  - `lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h`.
- **T-Display-S3, toolchain and recovery signals:** `.github/workflows/`, `platformio.ini`, `boards/`, `firmware/README.MD`.
- **LilyGo-Display-IDF:**
  - `main/display_s3.c`, `main/main.cpp`, `main/power_driver.cpp`, `main/product_pins.h` (the T-Display-S3 block), `main/Kconfig.projbuild`.
  - `sdkconfig.defaults.t-display-s3`, `setup.py` (the IDF and LVGL targets), `README.md`.
  - `main/initSequence.*` is not needed: it serves the AMOLED boards, and `display_s3.c` does not include it.
- **Ignore:**
  - `lib/` (vendored code);
  - the community MIDI/LovyanGFX examples (they still use PWM backlight and wrong pins);
  - the 3D shells;
  - nes, PCBClock and PokerS3.
- **Issues and PRs in both repos,** flagged by keyword: sleep, backlight, GPIO15, battery, charg*, schematic, v1.x, revision, ST7789, screen, power, current, ADC, antenna.
  - Read the threads rather than trusting the state. A stale bot closes issues after 30 + 14 days, so "closed" does not mean fixed.
  - Follow LDI #16 (BSP request) and #8 (slow LVGL): either could signal an IDF 6 / LVGL 9 port.
- **Quarterly, by hand:**
  - The lilygo.cc T-Display-S3 page: version selector and v1.3 notes (SKUs H569 and H577 are non-touch).
  - The wiki.lilygo.cc T-Display-S3 "Version History".
  - The fccid.io/2ASYE-T-DISPLAYS3 exhibits: check the v1.3 internal photos for R12, the LDO marking and the CC resistors.
  - The Espressif Component Registry, for any T-Display-S3 BSP.

## 3. Cadence and triage

- Run the script automatically once a month, on the 1st. Exit 0 means no drift. Exit 10 means drift: open an issue with the report.
- Classify each drift:
  - **A. Hardware truth** (schematic; README power, pinout, LED or electrical sections; a new revision): update the KB and the decision log, and schedule a hardware re-check.
  - **B. Init, backlight, sleep or pin code** (CMD_1 bytes, setBrightness, the sleep order, `pin_config.h`): diff it against Gogadget's port, then rerun the display, backlight and sleep acceptance tests.
  - **C. Toolchain or docs only:** note it and bump the pin.
  - **D. Community examples or `lib/`:** ignore.
- Bump the pins in `tools/upstream.lock` in the same commit as the KB update.
- Check out of cycle when any of these appear:
  - a new schematic PDF, or a v1.3 schematic;
  - any change to the CMD_1 table, `pin_config.h`, setBrightness or the sleep sequence;
  - an LDI commit that touches `display_s3.c` or adds IDF 6 / LVGL 9 support.

## 4. Script (tested 2026-10-20)

Requirements:
- git, which keeps blob-less bare clones in a cache;
- optionally `gh`, for the issue scan;
- optionally `pdftotext` (poppler), for word diffs of the schematic and datasheet PDFs.

Test runs:
- With the pins below: "no changes in watched paths", exit 0.
- With T-Display-S3 pinned at `02f0e9d`: exit 10. The report listed every 2026 README and factory change, the README_CN addition, the board-file rename and the new `image/WiFi.png`.

=== tools/upstream.lock ===
```
# tools/upstream.lock  (name|git-url|pinned-sha|space-separated watched paths)
T-Display-S3|https://github.com/Xinyuan-LilyGO/T-Display-S3.git|1cc41eb1c76b2f3215130a3dd64e7138ce00b482|README.md README_CN.md schematic datasheet dimensions/README.MD image examples/factory/factory.ino examples/factory/pin_config.h examples/Sleep/Sleep.ino examples/GetBatteryVoltage/GetBatteryVoltage.ino examples/lv_demos/lv_demos.ino examples/lv_demos/pin_config.h examples/tft/tft.ino lib/TFT_eSPI/User_Setups/Setup206_LilyGo_T_Display_S3.h firmware/README.MD boards platformio.ini .github/workflows
LilyGo-Display-IDF|https://github.com/Xinyuan-LilyGO/LilyGo-Display-IDF.git|b1a1cc54994bf1b417e3bb30c437bbe1036bff7f|main/display_s3.c main/main.cpp main/power_driver.cpp main/product_pins.h main/Kconfig.projbuild sdkconfig.defaults.t-display-s3 setup.py README.md
```

=== tools/upstream-watch.sh ===
```bash
#!/usr/bin/env bash
# upstream-watch.sh - report what changed upstream (LilyGO) in the paths Gogadget depends on.
# Usage: tools/upstream-watch.sh [lockfile]   exit 0 = no drift, exit 10 = drift (review, then bump the pin)
# Lock file lines: name|git-url|pinned-sha|space-separated paths   (lines starting with # are comments)
set -euo pipefail
LOCK=${1:-tools/upstream.lock}
CACHE=${UPSTREAM_CACHE:-$HOME/.cache/gogadget-upstream}
mkdir -p "$CACHE"
since=$(cat "$CACHE/last-run" 2>/dev/null || date -u -v-31d +%F 2>/dev/null || date -u -d '31 days ago' +%F)
status=0
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
words() { pdftotext -raw "$1" - 2>/dev/null | awk '{for(i=1;i<=NF;i++) print $i}' | sort -u; }

while IFS='|' read -r name url pin paths; do
  [[ -z "${name// }" || "${name:0:1}" == "#" ]] && continue
  repo="$CACHE/$name.git"
  [[ -d "$repo" ]] || git clone --quiet --bare --filter=blob:none "$url" "$repo"
  git -C "$repo" fetch --quiet --prune origin '+refs/heads/*:refs/heads/*'
  head=$(git -C "$repo" rev-parse HEAD)
  read -r -a P <<<"$paths"
  echo "== $name  pinned ${pin:0:7}  upstream HEAD ${head:0:7} ($(git -C "$repo" log -1 --format=%cs HEAD))"
  if [[ -z "$(git -C "$repo" log --format=%h "$pin..$head" -- "${P[@]}")" ]]; then
    echo "   no changes in watched paths"
  else
    status=10
    git -C "$repo" log --date=short --format='   %h %ad %an: %s' "$pin..$head" -- "${P[@]}"
    git -C "$repo" diff --stat=110 "$pin" "$head" -- "${P[@]}" | sed 's/^/   /'
    git -C "$repo" diff --name-status --no-renames "$pin" "$head" -- "${P[@]}" | while read -r st file; do
      case "$file" in
        *.pdf|*.PDF)   # schematic/datasheet: compare extracted words, not bytes
          git -C "$repo" show "$pin:$file" > "$tmp/old.pdf" 2>/dev/null || : > "$tmp/old.pdf"
          git -C "$repo" show "$head:$file" > "$tmp/new.pdf" 2>/dev/null || : > "$tmp/new.pdf"
          echo "   --- $file: words removed(-) / added(+)"
          { diff <(words "$tmp/old.pdf") <(words "$tmp/new.pdf") | grep '^[<>]' | grep -v -E 'PI[A-Z0-9]+$|^. (CO|NL)' |
            sed 's/^</-/; s/^>/+/' | paste -sd' ' - | fold -w 110 | sed 's/^/      /'; } || true ;;
        *.h|*.c|*.cpp|*.ino|*.md|*.MD|*.ini|*.json|*.yml|*.yaml|*.txt|*.projbuild|sdkconfig*)
          echo "   --- diff $file"
          git -C "$repo" diff "$pin" "$head" -- "$file" | sed -n '5,400p' | sed 's/^/      /' ;;
        *) echo "   --- $st (binary): $file" ;;
      esac
    done
  fi
  if command -v gh >/dev/null; then   # issues/PRs touched since the last run; hardware keywords flagged with !
    slug=${url#https://github.com/}; slug=${slug%.git}
    echo "   --- issues/PRs updated since $since"
    jq_fmt='.items[] | "      #" + (.number|tostring) + " [" + .state + "] " + .updated_at[:10] + " " + .title'
    { gh api -X GET search/issues -f q="repo:$slug updated:>=$since" -f per_page=100 --jq "$jq_fmt" 2>/dev/null |
      awk 'tolower($0) ~ /sleep|backlight|gpio ?15|battery|charg|schematic|v1[.][0-9]|revision|st7789|screen|power|current|adc|lcd_module|antenna/ {print "   !" $0; next} {print}'; } || true
  fi
done < "$LOCK"

date -u +%F > "$CACHE/last-run"
exit $status
```

## 5. Scheduling

GitHub Actions (`.github/workflows/upstream-watch.yml`):
```yaml
name: upstream-watch
on:
  schedule:
    - cron: "0 3 1 * *"
  workflow_dispatch:
permissions:
  contents: read
  issues: write
jobs:
  watch:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: sudo apt-get update && sudo apt-get install -y poppler-utils
      - id: watch
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          set +e
          bash tools/upstream-watch.sh tools/upstream.lock > report.txt 2>&1
          rc=$?
          echo "rc=$rc" >> "$GITHUB_OUTPUT"
          if [ "$rc" != 0 ] && [ "$rc" != 10 ]; then cat report.txt; exit "$rc"; fi
      - if: steps.watch.outputs.rc == '10'
        env:
          GH_TOKEN: ${{ github.token }}
        run: |
          head -c 60000 report.txt > body.txt
          gh issue create --title "Upstream drift $(date -u +%F)" --body-file body.txt
```
- On CI the cache starts empty, so the issue scan falls back to the last 31 days. That matches the monthly cadence.
- To run it locally instead, schedule this monthly with cron or launchd: `UPSTREAM_CACHE=$HOME/.cache/gogadget-upstream tools/upstream-watch.sh tools/upstream.lock`. The script stores the date of its last run, so each issue scan starts where the previous one ended.
