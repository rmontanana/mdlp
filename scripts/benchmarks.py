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
    commits = {r["git"]["commit"] for r in runs}
    if len(commits) > 1:
        notes.append(
            f"**Results span {len(commits)} different commits "
            f"({', '.join(sorted(commits))}).** Timings are only comparable when the "
            "measured code is identical — re-run the outdated platforms before "
            "drawing conclusions.")
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


def render_report(runs):
    runs = sorted(runs, key=lambda r: r["platform"]["slug"])
    lines = []
    add = lines.append

    add("# Cross-platform benchmark comparison")
    add("")
    add("Generated by `make bench-report` from every result in "
        "`docs/benchmarks/results/`. Do not edit by hand.")
    add("")
    add(f"Platforms: **{len(runs)}**. "
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
    add("| # | CPU | Arch | Cores | RAM | OS | Compiler | Commit |")
    add("|---|---|---|---:|---:|---|---|---|")
    for i, r in enumerate(runs, 1):
        p = r["platform"]
        cores = str(p.get("cpu_count") or "?")
        topo = p.get("cores")
        if topo:
            cores += f" ({topo['performance']}P+{topo['efficiency']}E)"
        ram = f"{p['ram_gib']} GiB" if p.get("ram_gib") else "?"
        add(f"| {i} | {p['cpu']} | {p['arch']} | {cores} | {ram} | "
            f"{p['os_description']} | {r['build']['compiler']} | `{r['git']['commit']}` |")
    add("")
    add("> The compiler differs between platforms by design (see "
        "`scripts/benchmarks.py`). Every difference below is hardware **and** "
        "toolchain combined and must not be read as a CPU comparison alone.")
    add("")

    # ---- scaling ---------------------------------------------------------- #
    add("## Scaling behaviour")
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
        present = [r for r in runs if any(
            x["benchmark"] == bench for x in r["results"])]
        if not present:
            continue
        add(f"### {bench}")
        add("")
        common = sorted(set.intersection(*[set(sizes_of(r)) for r in present]))
        header = "| Platform | " + " | ".join(
            f"{a:,}→{b:,}" for a, b in zip(common, common[1:])) + " | exponent | R² |"
        add(header)
        add("|---|" + "---:|" * (len(common) - 1) + "---:|---:|")
        for r in present:
            p = r["platform"]
            ratios = []
            for a, b in zip(common, common[1:]):
                ta, tb = median_of(r, bench, a), median_of(r, bench, b)
                ratios.append(f"{tb / ta:.1f}×" if ta and tb and ta > 0 else "—")
            pts = [(n, median_of(r, bench, n)) for n in common]
            slope, r2, reliable = loglog_slope(pts)
            mark = "" if reliable else " ⚠"
            add(f"| {p['cpu']} | " + " | ".join(ratios) + " | "
                f"{fmt(slope, 2)}{mark} | {fmt(r2, 3)} |")
        add("")

    # ---- absolute medians ------------------------------------------------- #
    add("## Median times")
    add("")
    add("All values in milliseconds. Median, not minimum — see the methodology "
        "note at the top of `scripts/benchmarks.py`.")
    add("")
    all_benches = []
    for r in runs:
        for b in benchmarks_of(r):
            if b not in all_benches:
                all_benches.append(b)
    common_sizes = sorted(set.intersection(*[set(sizes_of(r)) for r in runs]))
    for n in common_sizes:
        add(f"### n = {n:,}")
        add("")
        add("| Benchmark | " + " | ".join(r["platform"]["cpu"] for r in runs) + " |")
        add("|---|" + "---:|" * len(runs))
        for bench in all_benches:
            cells = [fmt(median_of(r, bench, n)) for r in runs]
            add(f"| {bench} | " + " | ".join(cells) + " |")
        add("")

    # ---- relative speed --------------------------------------------------- #
    if len(runs) > 1:
        ref = runs[0]
        add("## Relative speed")
        add("")
        add(f"Median time divided by the same measurement on **{ref['platform']['cpu']}** "
            "(the reference). Below 1.00 is faster than the reference.")
        add("")
        largest = common_sizes[-1]
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
    add("## Measurement noise")
    add("")
    add("`(mean − min) / min` at the largest common size, averaged over all "
        "benchmarks. **Any cross-platform difference smaller than a platform's "
        "noise figure is not a real difference.**")
    add("")
    add("| Platform | mean noise | worst benchmark | thermal drift |")
    add("|---|---:|---|---:|")
    largest = common_sizes[-1]
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
    add("A positive thermal drift means the machine was slower at the end of the "
        "run than at the start, i.e. it throttled.")
    add("")

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

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
