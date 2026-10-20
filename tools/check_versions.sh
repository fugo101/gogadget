#!/usr/bin/env bash
# The simulator must render with exactly the LVGL the firmware runs (ADR-0002,
# ADR-0005). Compare every place that names the LVGL version.
# Usage: tools/check_versions.sh      exit 0 = all equal, 1 = mismatch
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

manifest() { # file -> version pinned for lvgl/lvgl
    awk '/lvgl\/lvgl:/ {f=1} f && /==/ {sub(/.*==/, ""); sub(/".*/, ""); print; exit}' "$1"
}

want=$(sed -n 's/^set(GG_LVGL_VERSION "\(.*\)")/\1/p' sim/CMakeLists.txt)
status=0
check() { # where version
    if [ "$2" != "$want" ]; then
        echo "check_versions: $1 pins LVGL '$2', sim/CMakeLists.txt pins '$want'" >&2
        status=1
    fi
}
check ui/idf_component.yml "$(manifest ui/idf_component.yml)"
check firmware/components/board/idf_component.yml "$(manifest firmware/components/board/idf_component.yml)"
if [ -f firmware/dependencies.lock ]; then
    check firmware/dependencies.lock \
        "$(awk '/^  lvgl\/lvgl:/ {f=1} f && /^    version:/ {print $2; exit}' firmware/dependencies.lock)"
fi
[ "$status" -eq 0 ] && echo "check_versions: LVGL $want everywhere"
exit "$status"
