#!/usr/bin/env bash
# Fail if anything secret is tracked by git (ADR-0012). The repository is
# public and CI builds it, so credentials only ever live in gitignored files
# (secrets/, *.local) and reach the device through the USB console.
# Usage: tools/check_secrets.sh        exit 0 = clean, 1 = finding
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

status=0
fail() {
    echo "check_secrets: $*" >&2
    status=1
}

# 1. Files that must never be tracked.
forbidden='^(firmware/sdkconfig(\.old)?|secrets/.*|backups/.*|.*\.local(\..*)?|.*\.(pem|key|p12)|.*flash.*\.bin)$'
while IFS= read -r path; do
    fail "tracked file must stay local: $path"
done < <(git ls-files | grep -E "$forbidden" || true)

# 2. Credential-looking content in tracked text files (placeholders in <> are fine).
patterns=(
    '-----BEGIN [A-Z ]*PRIVATE KEY-----'
    'gh[pousr]_[A-Za-z0-9]{36}'
    'github_pat_[A-Za-z0-9_]{40,}'
    'AKIA[0-9A-Z]{16}'
    'xox[baprs]-[A-Za-z0-9-]{10,}'
    '(PASSWORD|PASSPHRASE|PSK)="[^"<$]{4,}"'
    'wifi[ .]add +[^<[:space:]]+ +[^<[:space:]]+'
)
for p in "${patterns[@]}"; do
    if hits=$(git grep -nIE -e "$p" -- ':!tools/check_secrets.sh' 2>/dev/null); then
        while IFS= read -r hit; do
            fail "possible credential: ${hit%%:*}:$(echo "$hit" | cut -d: -f2) matches /$p/"
        done <<<"$hits"
    fi
done

# 3. gitleaks, when installed, over exactly what git tracks (the index), not
#    build output or downloaded components.
if command -v gitleaks >/dev/null 2>&1; then
    tree=$(mktemp -d)
    trap 'rm -rf "$tree"' EXIT
    git checkout-index -a --prefix="$tree/"
    if ! gitleaks dir --no-banner --redact --exit-code 1 "$tree" >/dev/null 2>&1; then
        fail "gitleaks reported findings: run 'gitleaks dir --redact <tracked files>' for details"
    fi
fi

[ "$status" -eq 0 ] && echo "check_secrets: OK"
exit "$status"
