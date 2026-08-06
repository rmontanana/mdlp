#!/usr/bin/env python3
# ***************************************************************
# SPDX-FileCopyrightText: Copyright 2026 Ricardo Montañana Gómez
# SPDX-FileType: SOURCE
# SPDX-License-Identifier: MIT
# ***************************************************************
"""Run the mdlp benchmark on this machine and compare results across platforms.

Two subcommands:

  run     execute the benchmark binary, fingerprint this machine, and store the
          result under docs/benchmarks/results/
  report  merge every stored result into docs/benchmarks-platforms.md

Methodology (see docs/benchmarks.md):
  * Repetition counts are FIXED and identical on every platform. The minimum of a
    sample shrinks as the sample grows, so comparing minima taken with different
    rep counts would favour whichever machine ran more of them.
  * Cross-platform comparisons therefore use the MEDIAN. The minimum is reserved
    for before/after checks on a single machine.
  * The compiler is deliberately NOT unified across platforms. Differences below
    are hardware AND toolchain combined; the report never attributes them to the
    CPU alone.
"""

import argparse
import hashlib
import json
import math
import os
import platform
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
RESULTS_DIR = REPO_ROOT / "docs" / "benchmarks" / "results"
SORTBENCH_DIR = REPO_ROOT / "docs" / "benchmarks" / "sortbench"
SORTBENCH_REPORT = REPO_ROOT / "docs" / "benchmarks-sortbench.md"
REPORT_PATH = REPO_ROOT / "docs" / "benchmarks-platforms.md"
DEFAULT_BINARY = REPO_ROOT / "build_bench" / "bench" / "benchmark"

# Benchmarks whose scaling behaviour is worth reporting. Rows measuring copies or
# harness controls are excluded: their timings are dominated by fixed overhead.
SCALING_BENCHMARKS = [
    "CPPFImdlp::fit",
    "BinDisc::fit (uniform)",
    "BinDisc::fit (quantile)",
    "PKIDisc::fit (sqrt)",
    "CPPFImdlp::transform",
    "BinDisc::transform",
]


# --------------------------------------------------------------------------- #
# platform fingerprint
# --------------------------------------------------------------------------- #

def _run(cmd):
    """Return stripped stdout of cmd, or None if it fails."""
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return out.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def _read(path):
    try:
        return Path(path).read_text()
    except OSError:
        return None


def _cpu_brand_linux():
    info = _read("/proc/cpuinfo") or ""
    for line in info.splitlines():
        if line.lower().startswith("model name"):
            return line.split(":", 1)[1].strip()
    # arm64 Linux often has no "model name"; fall back to lscpu
    lscpu = _run(["lscpu"]) or ""
    for line in lscpu.splitlines():
        if line.lower().startswith("model name"):
            return line.split(":", 1)[1].strip()
    return platform.processor() or "unknown"


def _total_ram_bytes():
    if platform.system() == "Darwin":
        v = _run(["sysctl", "-n", "hw.memsize"])
        return int(v) if v and v.isdigit() else None
    meminfo = _read("/proc/meminfo") or ""
    for line in meminfo.splitlines():
        if line.startswith("MemTotal:"):
            parts = line.split()
            if len(parts) >= 2 and parts[1].isdigit():
                return int(parts[1]) * 1024
    return None


def _os_description():
    system = platform.system()
    if system == "Darwin":
        ver = _run(["sw_vers", "-productVersion"]) or platform.mac_ver()[0]
        return f"macOS {ver}"
    osr = _read("/etc/os-release") or ""
    for line in osr.splitlines():
        if line.startswith("PRETTY_NAME="):
            return line.split("=", 1)[1].strip().strip('"')
    return f"{system} {platform.release()}"


def _core_topology():
    """Performance/efficiency core split where the OS exposes it."""
    if platform.system() == "Darwin":
        p = _run(["sysctl", "-n", "hw.perflevel0.logicalcpu"])
        e = _run(["sysctl", "-n", "hw.perflevel1.logicalcpu"])
        if p and p.isdigit():
            return {"performance": int(p), "efficiency": int(e) if e and e.isdigit() else 0}
    return None


def slugify(text):
    text = text.lower()
    text = re.sub(r"\(r\)|\(tm\)|@.*$|cpu|processor", " ", text)
    text = re.sub(r"[^a-z0-9]+", "-", text)
    return re.sub(r"-+", "-", text).strip("-")


def source_hash():
    """Hash of the code that is actually measured: src/ and bench/.

    Comparing full commit SHAs flags results as incomparable whenever anything in
    the repository changed — including docs and previous benchmark results, which
    cannot affect a timing. Hashing only the measured sources says whether the
    runs really executed the same code.
    """
    h = hashlib.sha256()
    for base in ("src", "bench"):
        root = REPO_ROOT / base
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if path.is_file():
                h.update(path.relative_to(REPO_ROOT).as_posix().encode())
                h.update(path.read_bytes())
    return h.hexdigest()[:12]


def git_state():
    sha = _run(["git", "-C", str(REPO_ROOT), "rev-parse", "--short", "HEAD"]) or "unknown"
    status = _run(["git", "-C", str(REPO_ROOT), "status", "--porcelain"])
    return {"commit": sha, "dirty": bool(status)}


def fingerprint(label=None):
    system = platform.system()
    brand = (_run(["sysctl", "-n", "machdep.cpu.brand_string"])
             if system == "Darwin" else _cpu_brand_linux()) or "unknown"
    ram = _total_ram_bytes()
    fp = {
        "os": system,
        "os_description": _os_description(),
        "kernel": platform.release(),
        "arch": platform.machine(),
        "cpu": brand,
        "cpu_count": os.cpu_count(),
        "cores": _core_topology(),
        "ram_bytes": ram,
        "ram_gib": round(ram / (1024 ** 3)) if ram else None,
        "label": label,
    }
    parts = [system.lower(), platform.machine().lower(), slugify(brand)]
    if label:
        parts.append(slugify(label))
    fp["slug"] = "-".join(p for p in parts if p)
    return fp


# --------------------------------------------------------------------------- #
# run
# --------------------------------------------------------------------------- #

def cmd_run(args):
    binary = Path(args.binary)
    if not binary.exists():
        sys.exit(f"benchmark binary not found: {binary}\nRun `make bench` first.")

    fp = fingerprint(args.label)
    git = git_state()
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    tmp_json = RESULTS_DIR / ".raw.json"

    print(f">>> platform: {fp['cpu']} ({fp['arch']}, {fp['os_description']})")
    if git["dirty"]:
        print(">>> WARNING: working tree is dirty; this result is not reproducible "
              "from the recorded commit.")

    proc = subprocess.run([str(binary), "--json", str(tmp_json), "--level", args.level])
    if proc.returncode != 0:
        sys.exit(proc.returncode)

    payload = json.loads(tmp_json.read_text())
    tmp_json.unlink()
    payload["platform"] = fp
    payload["git"] = git
    payload["source_hash"] = source_hash()
    payload["recorded_at"] = datetime.now(timezone.utc).isoformat(timespec="seconds")

    out = RESULTS_DIR / f"{fp['slug']}__{git['commit']}.json"
    out.write_text(json.dumps(payload, indent=2) + "\n")
    print(f">>> stored {out.relative_to(REPO_ROOT)}")
    print(">>> regenerate the comparison with: make bench-report")


# --------------------------------------------------------------------------- #
# report
# --------------------------------------------------------------------------- #

def load_results():
    if not RESULTS_DIR.exists():
        return []
    runs = []
    for path in sorted(RESULTS_DIR.glob("*.json")):
        try:
            runs.append(json.loads(path.read_text()))
        except json.JSONDecodeError as exc:
            print(f"skipping malformed {path.name}: {exc}", file=sys.stderr)
    return runs


def median_of(run, benchmark, n):
    for r in run["results"]:
        if r["benchmark"] == benchmark and r["n"] == n:
            return r["median_ms"]
    return None


def dataset_version_of(run):
    # Files written before checksums existed used the stdlib distributions, whose
    # sequences are not portable across standard libraries.
    return run.get("dataset_version", 1)


def checksums_of(run):
    return {d["n"]: d["checksum"] for d in run.get("datasets", [])}


def code_of(run):
    """Short identifier of the code that was measured."""
    return run.get("source_hash") or "legacy"


def group_key(run):
    """Runs are comparable only within one dataset version AND one code revision.

    Grouping on the dataset version alone was not enough: once the library itself
    changed, the same machine appeared twice in a table under the same name with
    wildly different numbers and no way to tell which row was which.
    """
    return (dataset_version_of(run), code_of(run))


def sizes_of(run):
    return sorted({r["n"] for r in run["results"]})


def benchmarks_of(run):
    seen = []
    for r in run["results"]:
        if r["benchmark"] not in seen:
            seen.append(r["benchmark"])
    return seen


def fmt(value, digits=4):
    return "—" if value is None else f"{value:.{digits}f}"


# Below this median (ms) a measurement is dominated by fixed per-iteration
# overhead — object construction, allocation, timer granularity — rather than by
# the algorithm, so it distorts a power-law fit.
OVERHEAD_FLOOR_MS = 0.01


def loglog_slope(points):
    """Least-squares slope of log10(t) against log10(n), with R^2.

    Fitted over the LARGEST THREE sizes only. Small-n points sit on the flat part
    of the curve where fixed overhead dominates and would bias the exponent
    downwards: CPPFImdlp::fit measures 1.86 over all four sizes but 2.04 over the
    largest three, and the latter is the real complexity.

    Returns (slope, r2, reliable) where reliable is False when even the fitted
    points are overhead-dominated.
    """
    usable = [(n, t) for n, t in points if t and t > 0]
    usable = usable[-3:]
    reliable = bool(usable) and min(t for _, t in usable) >= OVERHEAD_FLOOR_MS
    pts = [(math.log10(n), math.log10(t)) for n, t in usable]
    if len(pts) < 2:
        return None, None, False
    k = len(pts)
    mx = sum(x for x, _ in pts) / k
    my = sum(y for _, y in pts) / k
    sxx = sum((x - mx) ** 2 for x, _ in pts)
    if sxx == 0:
        return None, None, False
    sxy = sum((x - mx) * (y - my) for x, y in pts)
    slope = sxy / sxx
    syy = sum((y - my) ** 2 for _, y in pts)
    r2 = (sxy ** 2) / (sxx * syy) if syy > 0 else 1.0
    return slope, r2, reliable


def homogeneity_notes(runs):
    notes = []

    # Compare the measured sources, not the whole repository: a docs-only commit
    # cannot change a timing. Legacy results carry no source_hash, so fall back to
    # the commit SHA for those.
    hashes = {r.get("source_hash") for r in runs}
    if None in hashes:
        commits = {r["git"]["commit"] for r in runs}
        if len(commits) > 1:
            notes.append(
                f"**Some results predate source hashing, and they span "
                f"{len(commits)} commits ({', '.join(sorted(commits))}).** Whether "
                "the measured code was identical cannot be verified for those; "
                "re-run them to find out.")
        hashes.discard(None)
    if len(hashes) > 1:
        notes.append(
            f"**The measured sources differ between runs ({', '.join(sorted(hashes))}).** "
            "`src/` and `bench/` were not identical, so these timings measure "
            "different code and cannot be compared.")

    # The datasets themselves must match, otherwise a platform may simply have
    # been given less work to do.
    by_n = {}
    for r in runs:
        for n, c in checksums_of(r).items():
            by_n.setdefault(n, set()).add(c)
    mismatched = sorted(n for n, cs in by_n.items() if len(cs) > 1)
    if mismatched:
        notes.append(
            f"**The generated datasets differ between platforms at n = "
            f"{', '.join(f'{n:,}' for n in mismatched)}.** The platforms did not "
            "measure the same work, so absolute comparisons between them are "
            "meaningless.")

    legacy = [r["platform"]["slug"] for r in runs if dataset_version_of(r) < 2]
    if legacy:
        notes.append(
            f"**{len(legacy)} result(s) use dataset version 1**, generated with "
            "`std::normal_distribution` and `std::uniform_int_distribution`. Those "
            "are not specified to produce the same sequence across standard "
            "libraries, so a shared seed did not give shared data and each "
            "platform very likely discretized a *different* dataset. Their "
            "cross-platform absolute timings are not interpretable. Scaling "
            "exponents remain valid, being computed within one platform.")

    levels = {r.get("level", "full") for r in runs}
    if len(levels) > 1:
        notes.append(
            f"**Results mix `--level` settings ({', '.join(sorted(levels))}).** "
            "Only sizes present in every run are compared.")
    versions = {r.get("library_version", "?") for r in runs}
    if len(versions) > 1:
        notes.append(f"**Library versions differ ({', '.join(sorted(versions))}).**")
    dirty = [r["platform"]["slug"] for r in runs if r["git"].get("dirty")]
    if dirty:
        notes.append(
            f"**Measured with a dirty working tree: {', '.join(dirty)}.** "
            "Those results cannot be reproduced from the recorded commit.")
    return notes


def render_comparison(runs, add):
    """Render the comparison sections for one set of mutually comparable runs."""
    common_sizes = sorted(set.intersection(*[set(sizes_of(r)) for r in runs]))
    if not common_sizes:
        add("_No size is present in every run of this group._")
        add("")
        return

    # ---- scaling ---------------------------------------------------------- #
    add("### Scaling behaviour")
    add("")
    add("Ratio of median time when n grows 10×. A ratio near **10** is linear, "
        "near **100** is quadratic.")
    add("")
    add("The exponent is the log-log slope fitted over the **largest three sizes**; "
        "R² near 1.0 means the power law fits well. Small-n points are excluded "
        "because fixed per-iteration overhead flattens the curve there and biases "
        "the exponent downwards.")
    add("")
    add(f"A **⚠** marks a fit whose own points are still under "
        f"{OVERHEAD_FLOOR_MS} ms and therefore overhead-dominated: read those "
        "exponents as indicative only.")
    add("")
    for bench in SCALING_BENCHMARKS:
        present = [r for r in runs if any(x["benchmark"] == bench for x in r["results"])]
        if not present:
            continue
        add(f"#### {bench}")
        add("")
        add("| Platform | " + " | ".join(
            f"{a:,}→{b:,}" for a, b in zip(common_sizes, common_sizes[1:]))
            + " | exponent | R² |")
        add("|---|" + "---:|" * (len(common_sizes) - 1) + "---:|---:|")
        for r in present:
            ratios = []
            for a, b in zip(common_sizes, common_sizes[1:]):
                ta, tb = median_of(r, bench, a), median_of(r, bench, b)
                ratios.append(f"{tb / ta:.1f}×" if ta and tb and ta > 0 else "—")
            slope, r2, reliable = loglog_slope(
                [(n, median_of(r, bench, n)) for n in common_sizes])
            mark = "" if reliable else " ⚠"
            add(f"| {r['platform']['cpu']} | " + " | ".join(ratios)
                + f" | {fmt(slope, 2)}{mark} | {fmt(r2, 3)} |")
        add("")

    # ---- absolute medians ------------------------------------------------- #
    add("### Median times")
    add("")
    add("All values in milliseconds. Median, not minimum — see the methodology "
        "note at the top of `scripts/benchmarks.py`.")
    add("")
    all_benches = []
    for r in runs:
        for b in benchmarks_of(r):
            if b not in all_benches:
                all_benches.append(b)
    for n in common_sizes:
        add(f"#### n = {n:,}")
        add("")
        add("| Benchmark | " + " | ".join(r["platform"]["cpu"] for r in runs) + " |")
        add("|---|" + "---:|" * len(runs))
        for bench in all_benches:
            add(f"| {bench} | "
                + " | ".join(fmt(median_of(r, bench, n)) for r in runs) + " |")
        add("")

    largest = common_sizes[-1]

    # ---- relative speed --------------------------------------------------- #
    if len(runs) > 1:
        ref = runs[0]
        add("### Relative speed")
        add("")
        add(f"Median time divided by the same measurement on "
            f"**{ref['platform']['cpu']}** (the reference). Below 1.00 is faster "
            "than the reference.")
        add("")
        add(f"At n = {largest:,}:")
        add("")
        add("| Benchmark | " + " | ".join(r["platform"]["cpu"] for r in runs) + " |")
        add("|---|" + "---:|" * len(runs))
        for bench in SCALING_BENCHMARKS:
            base = median_of(ref, bench, largest)
            if not base:
                continue
            cells = []
            for r in runs:
                t = median_of(r, bench, largest)
                cells.append(f"{t / base:.2f}×" if t else "—")
            add(f"| {bench} | " + " | ".join(cells) + " |")
        add("")

    # ---- noise ------------------------------------------------------------ #
    add("### Measurement noise")
    add("")
    add("`(mean − min) / min` at the largest common size, averaged over all "
        "benchmarks. **Any cross-platform difference smaller than a platform's "
        "noise figure is not a real difference.**")
    add("")
    add("| Platform | mean noise | worst benchmark | thermal drift |")
    add("|---|---:|---|---:|")
    for r in runs:
        spreads = []
        worst, worst_val = "—", -1.0
        for x in r["results"]:
            if x["n"] != largest or x["min_ms"] <= 0:
                continue
            spread = (x["mean_ms"] - x["min_ms"]) / x["min_ms"]
            spreads.append(spread)
            if spread > worst_val:
                worst_val, worst = spread, x["benchmark"]
        avg = sum(spreads) / len(spreads) if spreads else None
        drift = r.get("thermal_drift") or {}
        before, after = drift.get("before_median_ms"), drift.get("after_median_ms")
        drift_txt = (f"{(after - before) / before * 100:+.1f}%"
                     if before and after and before > 0 else "—")
        add(f"| {r['platform']['cpu']} | "
            f"{f'{avg * 100:.1f}%' if avg is not None else '—'} | "
            f"{worst} ({worst_val * 100:.1f}%) | {drift_txt} |")
    add("")
    add("Positive thermal drift means the machine ran slower at the end than at "
        "the start, i.e. it throttled. Large *negative* drift means it started "
        "cold, so the cells measured first — the small-n ones — were taken at "
        "reduced clocks.")
    add("")


def render_report(runs):
    runs = sorted(runs, key=lambda r: r["platform"]["slug"])
    lines = []
    add = lines.append

    add("# Cross-platform benchmark comparison")
    add("")
    add("Generated by `make bench-report` from every result in "
        "`docs/benchmarks/results/`. Do not edit by hand.")
    add("")
    machines = {r["platform"]["slug"] for r in runs}
    add(f"**{len(runs)}** run(s) across **{len(machines)}** machine(s). "
        f"Generated {datetime.now(timezone.utc).isoformat(timespec='seconds')}.")
    add("")

    notes = homogeneity_notes(runs)
    if notes:
        add("## ⚠ Homogeneity warnings")
        add("")
        for n in notes:
            add(f"- {n}")
        add("")

    # ---- platforms -------------------------------------------------------- #
    add("## Platforms")
    add("")
    add("| # | CPU | Arch | Cores | RAM | OS | Compiler | Dataset | Code | Commit |")
    add("|---|---|---|---:|---:|---|---|---:|---|---|")
    for i, r in enumerate(runs, 1):
        p = r["platform"]
        cores = str(p.get("cpu_count") or "?")
        topo = p.get("cores")
        if topo:
            cores += f" ({topo['performance']}P+{topo['efficiency']}E)"
        ram = f"{p['ram_gib']} GiB" if p.get("ram_gib") else "?"
        add(f"| {i} | {p['cpu']} | {p['arch']} | {cores} | {ram} | "
            f"{p['os_description']} | {r['build']['compiler']} | "
            f"v{dataset_version_of(r)} | `{code_of(r)}` | `{r['git']['commit']}` |")
    add("")
    add("> The compiler differs between platforms by design (see "
        "`scripts/benchmarks.py`). Every difference below is hardware **and** "
        "toolchain combined and must not be read as a CPU comparison alone.")
    add("")

    # ---- one comparison per (dataset version, code revision) -------------- #
    groups = {}
    for r in runs:
        groups.setdefault(group_key(r), []).append(r)

    # Newest first, by the most recent run in each group.
    def recency(key):
        return max(r.get("recorded_at", "") for r in groups[key])

    multi = len(groups) > 1
    for key in sorted(groups, key=recency, reverse=True):
        version, code = key
        group = groups[key]
        if multi:
            commits = sorted({r["git"]["commit"] for r in group})
            add(f"## Dataset v{version} · code `{code}`")
            add("")
            add(f"{len(group)} run(s), commit(s) {', '.join(f'`{c}`' for c in commits)}.")
            add("")
            add("Runs are grouped by dataset version **and** by the hash of the "
                "measured sources, and are never compared across groups: a "
                "different dataset version generates different data, and a "
                "different code hash measures a different library.")
            add("")
        else:
            add("## Comparison")
            add("")
        if version < 2:
            add("> ⚠ **Dataset version 1 used the standard library's "
                "distributions**, which are not specified to produce the same "
                "sequence across implementations. Each platform very likely "
                "discretized a *different* dataset, so the absolute times below "
                "cannot be compared between platforms. The scaling exponents are "
                "unaffected — each is computed within a single platform.")
            add("")
        render_comparison(group, add)

    return "\n".join(lines) + "\n"


def cmd_report(args):
    runs = load_results()
    if not runs:
        sys.exit(f"no results found in {RESULTS_DIR}\nRun `make bench` first.")
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(render_report(runs))
    print(f">>> wrote {REPORT_PATH.relative_to(REPO_ROOT)} from {len(runs)} platform(s)")
    for r in sorted(runs, key=lambda x: x["platform"]["slug"]):
        print(f"    - {r['platform']['cpu']} ({r['platform']['arch']}, "
              f"{r['build']['compiler']}, {r['git']['commit']})")


# --------------------------------------------------------------------------- #
# sortbench: toolchain diagnostic
# --------------------------------------------------------------------------- #

# A build has to differ from the others by more than this to be called faster.
# Comfortably above the 0.9-3.5% noise the library benchmark reports.
TOOLCHAIN_MARGIN = 0.15

# The shape that matters most: since Phase 7, sortIndices is ~39% of
# CPPFImdlp::fit, so this is where a standard-library difference would hurt.
KEY_SHAPE = "stable_sort<index>"


def cmd_sortbench_store(args):
    csv_path = Path(args.csv)
    if not csv_path.exists():
        sys.exit(f"csv not found: {csv_path}")

    rows = []
    with csv_path.open() as f:
        header = f.readline().strip().split(",")
        expected = ["label", "toolchain", "shape", "n", "min_ms", "median_ms"]
        if header != expected:
            sys.exit(f"unexpected csv header: {header}")
        for line in f:
            line = line.strip()
            if not line:
                continue
            label, toolchain, shape, n, min_ms, median_ms = line.split(",")
            rows.append({
                "label": label, "toolchain": toolchain, "shape": shape,
                "n": int(n), "min_ms": float(min_ms), "median_ms": float(median_ms),
            })
    if not rows:
        sys.exit("csv contained no measurements")

    fp = fingerprint(args.label)
    git = git_state()
    builds = []
    for r in rows:
        if not any(b["label"] == r["label"] for b in builds):
            builds.append({"label": r["label"], "toolchain": r["toolchain"]})

    payload = {
        "schema": 1,
        "kind": "sortbench",
        "platform": fp,
        "git": git,
        "source_hash": source_hash(),
        "recorded_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "builds": builds,
        "results": rows,
    }
    SORTBENCH_DIR.mkdir(parents=True, exist_ok=True)
    out = SORTBENCH_DIR / f"{fp['slug']}__{git['commit']}.json"
    out.write_text(json.dumps(payload, indent=2) + "\n")
    print(f">>> stored {out.relative_to(REPO_ROOT)}")
    print(">>> regenerate the comparison with: make sortbench-report")


def sortbench_median(run, label, shape, n):
    for r in run["results"]:
        if r["label"] == label and r["shape"] == shape and r["n"] == n:
            return r["median_ms"]
    return None


def diagnose_shape(run, shape, n):
    """Compiler and standard-library effect for one shape, at one size.

    clang+libstdc++ against gcc+libstdc++ isolates the compiler; clang+libc++
    against clang+libstdc++ isolates the library.
    """
    base = sortbench_median(run, "gcc_libstdcxx", shape, n)
    same_lib = sortbench_median(run, "clang_libstdcxx", shape, n)
    other_lib = sortbench_median(run, "clang_libcxx", shape, n)
    if not base or not same_lib or not other_lib:
        return None
    compiler = 1.0 - same_lib / base
    stdlib = 1.0 - other_lib / same_lib

    # If GCC tuned for the actual CPU catches up with clang, the gap was never
    # the compiler *vendor* — it was GCC's default generic x86-64 scheduling.
    # Measured on Zen 5: -march=native moves GCC to within 1% of clang, while a
    # GCC 15 -> 16 upgrade moved it 0.4%.
    native = sortbench_median(run, "gcc_native", shape, n)
    tuning = (1.0 - native / base) if (native and base) else None
    tuned_matches_clang = (
        native is not None and same_lib
        and abs(native - same_lib) / same_lib < 0.10
    )

    # Tuning explains the *compiler* half only. The library effect is independent
    # and must not be swallowed by it: on sort<float>, -march=native brings GCC to
    # clang's level while libc++ remains 2.9x faster than libstdc++ on top of that.
    compiler_is_tuning = bool(
        compiler > TOOLCHAIN_MARGIN and tuning and tuning > TOOLCHAIN_MARGIN
        and tuned_matches_clang)
    stdlib_matters = stdlib > TOOLCHAIN_MARGIN

    if compiler_is_tuning and stdlib_matters:
        verdict = "tuning+stdlib"
    elif compiler_is_tuning:
        verdict = "tuning"
    elif compiler > TOOLCHAIN_MARGIN and stdlib_matters:
        verdict = "both"
    elif compiler > TOOLCHAIN_MARGIN:
        verdict = "compiler"
    elif stdlib_matters:
        verdict = "stdlib"
    else:
        verdict = "neither"
    return {"shape": shape, "n": n, "compiler": compiler, "stdlib": stdlib,
            "tuning": tuning, "verdict": verdict}


def diagnose(run):
    """Per-shape readings, plus why the run may not support any.

    Deliberately not a single verdict. The first version of this judged only
    stable_sort<index> and reported "compiler", which was true for that shape and
    hid that sort<float> is dominated by the standard library instead. Different
    hot loops have different answers and collapsing them loses the finding.
    """
    labels = [b["label"] for b in run["builds"]]
    resolved = {b["label"]: b["toolchain"] for b in run["builds"]}
    if len({resolved[x] for x in labels}) < 2:
        return [], ("every build resolved to the same toolchain, so this machine "
                    "cannot separate compiler from standard library")
    needed = ["gcc_libstdcxx", "clang_libstdcxx", "clang_libcxx"]
    missing = [x for x in needed if x not in labels]
    if missing:
        return [], (f"missing build(s): {', '.join(missing)}; all three are "
                    "required to separate the two variables")

    shapes = []
    for r in run["results"]:
        if r["shape"] not in shapes:
            shapes.append(r["shape"])
    largest = max(r["n"] for r in run["results"])
    out = []
    for shape in shapes:
        d = diagnose_shape(run, shape, largest)
        if d:
            out.append(d)
    return out, None


def sortbench_display_names(runs):
    """Machine names, disambiguated when one machine appears more than once.

    Re-running a machine after a toolchain upgrade is exactly the experiment this
    diagnostic invites, so two runs of one machine is the normal case, not an edge
    case. Labelling both with the bare CPU name would repeat the mistake the
    library report had to fix.
    """
    counts = {}
    for r in runs:
        counts[r["platform"]["slug"]] = counts.get(r["platform"]["slug"], 0) + 1
    names = {}
    for r in runs:
        key = id(r)
        cpu = r["platform"]["cpu"]
        if counts[r["platform"]["slug"]] == 1:
            names[key] = cpu
            continue
        # Disambiguate by whatever the default build resolved to; a compiler
        # upgrade is the usual reason for a repeat run.
        tag = None
        for b in r["builds"]:
            if b["label"] == "gcc_libstdcxx":
                tag = b["toolchain"].split(" / ")[0]
                break
        names[key] = f"{cpu} · {tag}" if tag else f"{cpu} · {r['git']['commit']}"

    # A toolchain tag is not always enough: repeating a run with the same
    # compiler is a legitimate way to check reproducibility, and two identical
    # labels would be as useless as none.
    used = {}
    for r in runs:
        used[names[id(r)]] = used.get(names[id(r)], 0) + 1
    for r in runs:
        if used[names[id(r)]] > 1:
            names[id(r)] = f"{names[id(r)]} ({r['git']['commit']})"
    return names


def render_sortbench(runs):
    runs = sorted(runs, key=lambda r: (r["platform"]["slug"], r.get("recorded_at", "")))
    names = sortbench_display_names(runs)
    lines = []
    add = lines.append

    add("# Toolchain diagnostic (sortbench)")
    add("")
    add("Generated by `make sortbench-report` from every result in "
        "`docs/benchmarks/sortbench/`. Do not edit by hand.")
    add("")
    add("`bench/sortbench/` replicates the library's hot loops with no "
        "dependencies, so one machine can build it with several compiler and "
        "standard-library combinations. These are **replicas**, not the library: a "
        "result here is a strong hypothesis about a cause, not a measurement of "
        "mdlp.")
    add("")
    add(f"**{len(runs)}** run(s). "
        f"Generated {datetime.now(timezone.utc).isoformat(timespec='seconds')}.")
    add("")

    add("## Verdict, per shape")
    add("")
    add("`tuning` = GCC catches clang once built with `-march=native`, so the gap "
        "was its default generic x86-64 scheduling, not the compiler. "
        "`tuning+stdlib` = that, **and** libc++ is faster still on top of it — the "
        "two are independent. `compiler` = "
        "clang beats GCC and tuning does not explain it. `stdlib` = libc++ beats "
        "libstdc++ under the same compiler. `both` = compiler and library each help "
        "independently. `neither` = the obvious suspects are ruled out.")
    add("")
    add(f"A swap has to beat {TOOLCHAIN_MARGIN * 100:.0f}% to count. Percentages "
        "are the time saved by making that one swap.")
    add("")
    for r in runs:
        diags, why = diagnose(r)
        add(f"**{names[id(r)]}**")
        add("")
        if why:
            add(f"- Inconclusive: {why}.")
            add("")
            continue
        add("| Shape | n | compiler | stdlib | -march=native | reading |")
        add("|---|---:|---:|---:|---:|---|")
        for d in diags:
            tune = f"{d['tuning'] * 100:+.0f}%" if d.get("tuning") is not None else "—"
            add(f"| {d['shape']} | {d['n']:,} | {d['compiler'] * 100:+.0f}% | "
                f"{d['stdlib'] * 100:+.0f}% | {tune} | **{d['verdict']}** |")
        add("")

    # ---- same build, different machines ----------------------------------- #
    # The per-machine tables cannot show this, and it is where a compiler *version*
    # difference reveals itself: a verdict of "compiler" may really mean "this
    # particular release of that compiler".
    if len(runs) > 1:
        add("## Same build, across machines")
        add("")
        add("A verdict of `compiler` above says clang beat GCC on that machine. It "
            "cannot say whether the cause is the vendor or the *version*, because "
            "each machine has whatever versions it has. Comparing one build label "
            "across machines separates them: if the clang columns agree while the "
            "GCC columns do not, the GCC release is the variable, not the vendor.")
        add("")
        largest = max(x["n"] for r in runs for x in r["results"])
        labels = []
        for r in runs:
            for b in r["builds"]:
                if b["label"] not in labels:
                    labels.append(b["label"])
        shapes = []
        for r in runs:
            for x in r["results"]:
                if x["shape"] not in shapes:
                    shapes.append(x["shape"])
        for label in labels:
            present = [r for r in runs if any(b["label"] == label for b in r["builds"])]
            if len(present) < 2:
                continue
            add(f"### `{label}` at n = {largest:,}")
            add("")
            add("| Machine | Toolchain | " + " | ".join(shapes) + " |")
            add("|---|---|" + "---:|" * len(shapes))
            for r in present:
                tc = next(b["toolchain"] for b in r["builds"] if b["label"] == label)
                cells = []
                for shape in shapes:
                    v = sortbench_median(r, label, shape, largest)
                    cells.append(f"{v:.3f}" if v is not None else "—")
                add(f"| {names[id(r)]} | {tc} | " + " | ".join(cells) + " |")
            add("")

    for r in runs:
        p = r["platform"]
        add(f"## {names[id(r)]}")
        add("")
        add(f"{p['os_description']}, {p['arch']}, commit `{r['git']['commit']}`, "
            f"code `{r.get('source_hash', '?')}`.")
        add("")
        add("| Build | Resolved to |")
        add("|---|---|")
        for b in r["builds"]:
            add(f"| `{b['label']}` | {b['toolchain']} |")
        add("")

        labels = [b["label"] for b in r["builds"]]
        shapes = []
        for x in r["results"]:
            if (x["shape"], x["n"]) not in shapes:
                shapes.append((x["shape"], x["n"]))
        add("| Shape | n | " + " | ".join(f"`{x}`" for x in labels) + " |")
        add("|---|---:|" + "---:|" * len(labels))
        for shape, n in shapes:
            base = sortbench_median(r, labels[0], shape, n)
            cells = []
            for lab in labels:
                v = sortbench_median(r, lab, shape, n)
                if v is None:
                    cells.append("—")
                elif base:
                    cells.append(f"{v:.4f} ({v / base:.2f}×)")
                else:
                    cells.append(f"{v:.4f}")
            add(f"| {shape} | {n:,} | " + " | ".join(cells) + " |")
        add("")
        add("Median ms; ratios against the first build.")
        add("")

    return "\n".join(lines) + "\n"


def cmd_sortbench_report(args):
    if not SORTBENCH_DIR.exists():
        sys.exit(f"no sortbench results in {SORTBENCH_DIR}\nRun `make sortbench` first.")
    runs = []
    for path in sorted(SORTBENCH_DIR.glob("*.json")):
        try:
            runs.append(json.loads(path.read_text()))
        except json.JSONDecodeError as exc:
            print(f"skipping malformed {path.name}: {exc}", file=sys.stderr)
    if not runs:
        sys.exit(f"no sortbench results in {SORTBENCH_DIR}\nRun `make sortbench` first.")
    SORTBENCH_REPORT.write_text(render_sortbench(runs))
    print(f">>> wrote {SORTBENCH_REPORT.relative_to(REPO_ROOT)} from {len(runs)} run(s)")
    names = sortbench_display_names(runs)
    for r in runs:
        diags, why = diagnose(r)
        if why:
            print(f"    - {names[id(r)]}: inconclusive ({why})")
        else:
            summary = ", ".join(f"{d['shape']}={d['verdict']}" for d in diags)
            print(f"    - {names[id(r)]}: {summary}")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    run = sub.add_parser("run", help="run the benchmark and store the result")
    run.add_argument("--binary", default=str(DEFAULT_BINARY))
    run.add_argument("--level", choices=["quick", "full"], default="full")
    run.add_argument("--label", default=None,
                     help="disambiguate machines with the same CPU, e.g. 'studio'")
    run.set_defaults(func=cmd_run)

    rep = sub.add_parser("report", help="merge stored results into a comparison")
    rep.set_defaults(func=cmd_report)

    sbs = sub.add_parser("sortbench-store", help="store a sortbench csv with a machine fingerprint")
    sbs.add_argument("--csv", required=True)
    sbs.add_argument("--label", default=None)
    sbs.set_defaults(func=cmd_sortbench_store)

    sbr = sub.add_parser("sortbench-report", help="merge stored sortbench results")
    sbr.set_defaults(func=cmd_sortbench_report)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
