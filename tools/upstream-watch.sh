#!/usr/bin/env bash
# upstream-watch.sh: report what changed upstream (LilyGO) in the paths Gogadget learns from.
#
# Usage: tools/upstream-watch.sh [lockfile]
#        (default lockfile: upstream.lock next to this script)
#
# Exit status:
#   0   no drift: no commit after a pin touches a watched path
#   10  drift: triage the report (docs/upstream.md), then bump the pin
#   2   error: the report is incomplete (every failure maps to 2, never to 0 or 10)
#
# Lock file lines: name|git-url|pinned full SHA|space-separated watched paths.
# Blank lines and lines starting with # are ignored.
#
# Environment:
#   UPSTREAM_CACHE  where the blob-less bare clones and the last-run date live
#                   (default: ${XDG_CACHE_HOME:-$HOME/.cache}/gogadget-upstream)
#   UPSTREAM_SINCE  YYYY-MM-DD start of the issue/PR scan
#                   (default: the date of the last complete run, else 31 days ago)
#
# Needs bash 3.2 or newer (macOS /bin/bash is fine) and git; runs on BSD (macOS)
# and GNU (Ubuntu) userlands. Optional: gh for the issue/PR scan, pdftotext
# (poppler) for word diffs of the schematic and datasheet PDFs.
#
# stdout carries the summary of every repository first and then, on drift, the
# per-file details, so a truncated copy of the report keeps the summary.

set -euo pipefail
export LC_ALL=C GIT_TERMINAL_PROMPT=0

# Hardware keywords flagged with ! in the issue scan (matched case-insensitively).
keywords='sleep|backlight|gpio ?15|battery|charg|schematic|v1[.][0-9]|revision|st7789|screen|power|current|adc|lcd_module|antenna'
skip_re='^[[:space:]]*(#|$)'
name_re='^[A-Za-z0-9._-]+$'
sha_re='^[0-9a-f]{40}$'
date_re='^[0-9]{4}-[0-9]{2}-[0-9]{2}$'

finished=0
tmp=
repo=

cleanup() {
    local rc=$?
    if [[ -n $tmp ]]; then rm -rf "$tmp"; fi
    if [[ $finished != 1 ]]; then
        if [[ $rc != 2 ]]; then
            echo "upstream-watch: unexpected failure (status $rc); the report is incomplete" >&2
        fi
        exit 2
    fi
}
trap cleanup EXIT

die() {
    echo "upstream-watch: $*" >&2
    exit 2
}

usage() {
    sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

# git in the current upstream clone, with output that does not depend on user config.
g() { git -C "$repo" -c core.quotePath=false -c color.ui=never "$@"; }

# A unified diff from its first hunk on, capped at $1 lines.
cap_diff() {
    awk -v max="$1" '
        /^Binary files / { print "      " $0; next }
        /^@@/ { body = 1 }
        body { n++; if (n <= max) print "      " $0 }
        END { if (n > max) printf "      ... %d more lines\n", n - max }'
}

# Unique words of a PDF text layer, one per line (nothing for a missing or broken PDF).
pdf_words() {
    { pdftotext -raw "$1" - 2>/dev/null || true; } | awk '{ for (i = 1; i <= NF; i++) print $i }' | sort -u
}

# Words removed (-) and added (+) between two PDFs, wrapped at about 100 columns and
# capped at $3 lines. Altium object names (PI<designator><pin>, CO<designator>,
# NL<net>) are dropped because they change with every export.
pdf_word_diff() {
    { diff <(pdf_words "$1") <(pdf_words "$2") || true; } | awk -v max="$3" '
        function flush() {
            if (line != "") { n++; if (n <= max) print "      " line; line = "" }
        }
        /^[<>] / {
            w = $2
            if (w ~ /^(PI|CO)[A-Z]+[0-9]/ || w ~ /^NL[A-Z0-9_]+$/) next
            w = ($1 == "<" ? "-" : "+") w
            if (line != "" && length(line) + 1 + length(w) > 100) flush()
            line = (line == "" ? w : line " " w)
        }
        END {
            flush()
            if (n > max) printf "      ... %d more lines of words\n", n - max
            if (n == 0) print "      (no word changes in the text layer)"
        }'
}

# Per-file details of the drift in the current repository: $1 = pin, $2 = tip, then the paths.
file_details() {
    local pin=$1 tip=$2 st file lower
    shift 2
    g diff -z --name-status --no-renames "$pin" "$tip" -- "$@" |
        while IFS= read -r -d '' st && IFS= read -r -d '' file; do
            lower=$(printf '%s' "$file" | tr '[:upper:]' '[:lower:]')
            case $lower in
                *.pdf)
                    echo "   --- $st $file: words removed (-) and added (+)"
                    if command -v pdftotext >/dev/null 2>&1; then
                        g show "$pin:$file" >"$tmp/old.pdf" 2>/dev/null || : >"$tmp/old.pdf"
                        g show "$tip:$file" >"$tmp/new.pdf" 2>/dev/null || : >"$tmp/new.pdf"
                        pdf_word_diff "$tmp/old.pdf" "$tmp/new.pdf" 60
                    else
                        echo "      (install pdftotext from poppler for a word diff)"
                    fi
                    ;;
                *.png | *.jpg | *.jpeg | *.gif | *.bmp | *.webp | *.stl | *.stp | *.step | *.dwg | *.dxf | *.zip | *.bin)
                    echo "   --- $st $file (binary)"
                    ;;
                *)
                    echo "   --- $st $file"
                    g diff --no-ext-diff "$pin" "$tip" -- "$file" | cap_diff 400
                    ;;
            esac
        done
}

# Issues and PRs of a GitHub repository updated since $2, hardware keywords flagged with !.
issue_scan() {
    local slug=$1 since=$2 found
    local jq_fmt='.items[] | "#\(.number) [\(.state)\(if .pull_request then " PR" else "" end)] \(.updated_at[:10]) \(.title)"'
    echo "   issues and PRs updated since $since (! = hardware keyword):"
    if found=$(gh api -X GET search/issues -f q="repo:$slug updated:>=$since" -f per_page=100 --jq "$jq_fmt" 2>/dev/null); then
        if [[ -z $found ]]; then
            echo "      none"
        else
            printf '%s\n' "$found" | awk -v re="$keywords" '{ print (tolower($0) ~ re ? "    ! " : "      ") $0 }'
        fi
    else
        echo "      (scan failed: is gh authenticated?)"
    fi
}

main() {
    case ${1:-} in
        -h | --help) usage; finished=1; return 0 ;;
        -*) usage >&2; die "unknown option: $1" ;;
    esac
    [[ $# -le 1 ]] || die "too many arguments; usage: $0 [lockfile]"

    local script_dir lock cache since details status=0 drifted=
    local name url pin paths branch tip count slug p
    local -a watched
    script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
    lock=${1:-$script_dir/upstream.lock}
    [[ -f $lock && -r $lock ]] || die "cannot read lock file: $lock"
    cache=${UPSTREAM_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/gogadget-upstream}
    mkdir -p "$cache"

    if [[ -n ${UPSTREAM_SINCE:-} ]]; then
        since=$UPSTREAM_SINCE
    elif [[ -s $cache/last-run ]]; then
        since=$(cat "$cache/last-run")
    else  # BSD date first, then GNU date
        since=$(date -u -v-31d +%F 2>/dev/null || date -u -d '31 days ago' +%F)
    fi
    [[ $since =~ $date_re ]] || die "bad start date for the issue scan: '$since'"

    tmp=$(mktemp -d)
    details=$tmp/details.txt
    : >"$details"
    echo "upstream-watch $(date -u +%F): lock file $(basename "$lock"), issue scan since $since"

    while IFS='|' read -r name url pin paths <&3 || [[ -n ${name:-} ]]; do
        [[ $name =~ $skip_re ]] && continue
        paths=${paths%$'\r'}
        [[ $name =~ $name_re ]] || die "$lock: bad repository name '$name'"
        [[ -n $url && -n ${paths//[[:space:]]/} ]] || die "$lock: malformed line for $name"
        [[ $pin =~ $sha_re ]] || die "$name: the pin must be a full 40-character SHA, not '$pin'"
        read -r -a watched <<<"$paths"

        repo=$cache/$name.git
        if [[ ! -d $repo ]]; then
            git clone --quiet --bare --filter=blob:none "$url" "$repo"
        fi
        g remote set-url origin "$url"
        g fetch --quiet --prune origin '+refs/heads/*:refs/heads/*'
        # Follow upstream's default branch, even if it was renamed after the clone.
        branch=$(g ls-remote --symref origin HEAD | awk '$1 == "ref:" { sub("^refs/heads/", "", $2); print $2 }')
        [[ -n $branch ]] || die "$name: cannot read the default branch of $url"
        g symbolic-ref HEAD "refs/heads/$branch"
        tip=$(g rev-parse --verify --quiet "refs/heads/$branch^{commit}") ||
            die "$name: branch $branch is missing after the fetch"
        g cat-file -e "$pin^{commit}" 2>/dev/null ||
            die "$name: pinned commit ${pin:0:12} is not upstream (history rewritten, or a typo in the lock file)"

        echo
        echo "== $name: pinned ${pin:0:7} ($(g log -1 --format=%cs "$pin")), upstream $branch ${tip:0:7} ($(g log -1 --format=%cs "$tip"))"
        if ! g merge-base --is-ancestor "$pin" "$tip"; then
            echo "   ! the pin is not an ancestor of $branch: was upstream history rewritten?"
        fi
        for p in "${watched[@]}"; do
            if [[ -z $(g ls-tree "$pin" -- "$p") && -z $(g ls-tree "$tip" -- "$p") ]]; then
                echo "   ! watched path exists neither at the pin nor upstream (typo in the lock file?): $p"
            fi
        done

        count=$(g rev-list --count "$pin..$tip" -- "${watched[@]}")
        if [[ $count == 0 ]]; then
            echo "   no changes in watched paths"
        else
            status=10
            drifted="$drifted $name"
            echo "   $count commit(s) touch watched paths:"
            g log --max-count=40 --date=short --format='   %h %ad %an: %s' "$pin..$tip" -- "${watched[@]}"
            if [[ $count -gt 40 ]]; then echo "   ... $((count - 40)) older commit(s) not listed"; fi
            g diff --stat=110 "$pin" "$tip" -- "${watched[@]}" | sed 's/^/   /'
            {
                echo "== $name: changes in watched paths, ${pin:0:7}..${tip:0:7}"
                file_details "$pin" "$tip" "${watched[@]}"
                echo
            } >>"$details"
        fi

        if [[ $url != https://github.com/* ]]; then
            echo "   issue scan skipped: not a GitHub URL"
        elif ! command -v gh >/dev/null 2>&1; then
            echo "   issue scan skipped: gh is not installed"
        else
            slug=${url#https://github.com/}
            issue_scan "${slug%.git}" "$since"
        fi
    done 3<"$lock"

    echo
    if [[ $status == 10 ]]; then
        echo "result: drift in$drifted (exit 10): triage it as docs/upstream.md describes, then bump the pin"
        echo
        cat "$details"
    else
        echo "result: no drift (exit 0)"
    fi
    date -u +%F >"$cache/last-run"
    finished=1
    return "$status"
}

main "$@"
