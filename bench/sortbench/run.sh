#!/usr/bin/env bash
# ***************************************************************
# SPDX-FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT
# ***************************************************************
#
# Builds bench/sortbench/sortbench.cpp with every toolchain available on this
# machine and prints one comparison table.
#
# On Linux the three interesting builds are:
#   g++                        GCC   + libstdc++   (what the project uses today)
#   clang++                    clang + libstdc++   (isolates the compiler)
#   clang++ -stdlib=libc++     clang + libc++      (isolates the standard library)
#
# If clang+libstdc++ matches GCC and clang+libc++ is the fast one, the standard
# library is responsible. If both clang builds are fast, the compiler is. If all
# three match, neither is, and the difference lies elsewhere.
#
# Fedora needs `dnf install clang libcxx-devel` for the third build; Debian and
# Ubuntu need `apt install clang libc++-dev`. Builds that are unavailable are
# skipped with a note rather than failing the run.
#
# macOS realistically offers only AppleClang + libc++, which serves as the
# reference point.

set -uo pipefail

# awk must parse "1.5615" as a decimal; in a comma-decimal locale it would stop
# at the dot and read 1.
export LC_ALL=C

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$HERE/sortbench.cpp"
OUT="$(mktemp -d)"
CSV="$OUT/results.csv"
trap 'rm -rf "$OUT"' EXIT

FLAGS="-O3 -std=c++17"
BUILT=0
FIRST=1

build_and_run() {
    local name="$1"; shift
    local cxx="$1"; shift
    local extra="${1:-}"

    command -v "$cxx" >/dev/null 2>&1 || { echo ">>> skipping $name: $cxx not found"; return; }

    local bin="$OUT/sortbench_$name"
    # shellcheck disable=SC2086
    if ! "$cxx" $FLAGS $extra "$SRC" -o "$bin" 2>"$OUT/$name.err"; then
        echo ">>> skipping $name: build failed"
        sed 's/^/      /' "$OUT/$name.err" | head -5
        return
    fi
    echo ">>> running $name"
    if [ "$FIRST" -eq 1 ]; then
        "$bin" --header --label "$name" >>"$CSV" || return
        FIRST=0
    else
        "$bin" --label "$name" >>"$CSV" || return
    fi
    BUILT=$((BUILT + 1))
}

echo ">>> sortbench: standalone toolchain diagnostic (no libtorch, no conan)"
build_and_run "gcc_libstdcxx"    g++
build_and_run "clang_libstdcxx"  clang++
build_and_run "clang_libcxx"     clang++ "-stdlib=libc++"

if [ "$BUILT" -eq 0 ]; then
    echo ">>> no toolchain could be built; nothing to report"
    exit 1
fi

echo
awk -F, '
# LC_ALL=C is exported above so this parses dots as decimal separators.
NR == 1 { next }
{
    key = $3 "\t" $4
    if (!(key in seen)) { order[++n] = key; seen[key] = 1 }
    if (!($1 in lseen)) { lorder[++t] = $1; lseen[$1] = 1 }
    resolved[$1] = $2
    med[key SUBSEP $1] = $6
}
END {
    printf "%-22s %9s", "shape", "n"
    for (i = 1; i <= t; i++) printf "  %24s", lorder[i]
    printf "\n"
    printf "%-22s %9s", "----------------------", "---------"
    for (i = 1; i <= t; i++) printf "  %24s", "------------------------"
    printf "\n"
    for (j = 1; j <= n; j++) {
        split(order[j], p, "\t")
        printf "%-22s %9s", p[1], p[2]
        base = med[order[j] SUBSEP lorder[1]]
        for (i = 1; i <= t; i++) {
            v = med[order[j] SUBSEP lorder[i]]
            if (v == "") { printf "  %24s", "-" }
            else if (base + 0 > 0) { printf "  %16.4f (%5.2fx)", v, v / base }
            else { printf "  %24.4f", v }
        }
        printf "\n"
    }
    printf "\nMedian ms. Ratios are against the first column.\n"
    printf "\nWhat each build actually resolved to:\n"
    same = 1
    for (i = 1; i <= t; i++) {
        printf "  %-20s -> %s\n", lorder[i], resolved[lorder[i]]
        if (resolved[lorder[i]] != resolved[lorder[1]]) same = 0
    }
    if (same && t > 1) {
        printf "\nWARNING: every build resolved to the SAME toolchain, so this machine\n"
        printf "cannot separate compiler from standard library. On macOS g++ is a\n"
        printf "symlink to clang and libstdc++ is unavailable; run this on Linux.\n"
    }
}' "$CSV"

# Persist with a machine fingerprint, so results gathered on other machines can
# be compared here through the same git workflow as `make bench`.
if [ "${SORTBENCH_STORE:-1}" = "1" ]; then
    echo
    python3 "$HERE/../../scripts/benchmarks.py" sortbench-store --csv "$CSV" \
        || echo ">>> could not store the result (is python3 available?)"
else
    echo
    echo ">>> raw CSV (SORTBENCH_STORE=0, nothing written):"
    cat "$CSV"
fi
