# Performance baseline — 3.0.0

Baseline measurements for the 3.0.0 release, taken before any performance work.
This is the reference every later phase compares against, so that improvements can
be reported as measured deltas rather than asserted.

Corresponds to Phase 0 of [RELEASE_PLAN_V3.md](../RELEASE_PLAN_V3.md).

## Running it

```bash
export PATH="$HOME/miniconda3/bin:$PATH"   # conan
make bench                                 # full run, ~4 minutes
make bench LEVEL=quick                     # stops at n=10,000, ~seconds
```

`make bench` forces a Release build (`-O3`) into `build_bench/` with
`-DENABLE_BENCHMARK=ON`. The benchmark target is off by default and is never part
of `make test`. Almost all of a full run is the two largest `CPPFImdlp::fit`
cells — see the scaling note below.

The harness has no external dependencies. Data is generated from class-conditional
normal distributions with a fixed seed (42), so every run measures identical work.

`make bench` also stores a machine-readable result under
`docs/benchmarks/results/`, fingerprinted with the CPU, core topology, RAM, OS,
compiler and git commit. `LABEL=name` disambiguates two machines with the same CPU.

### Which statistic to use

Repetition counts are **fixed** and identical on every platform, deliberately. The
minimum of a sample shrinks as the sample grows, so comparing minima taken with
different rep counts would systematically favour whichever machine ran more of
them.

- **Minimum** — for before/after comparisons on *one* machine. Most stable figure
  under load.
- **Median** — for comparisons *across* machines. This is what
  `docs/benchmarks-platforms.md` reports.

## Comparing across platforms

```bash
# on each machine
make bench
git add docs/benchmarks/results && git commit -m "bench: <machine>" && git push

# anywhere, once the results are gathered
git pull
make bench-report        # regenerates docs/benchmarks-platforms.md
```

Results are versioned in git, so they accumulate through the normal workflow. Run
on a **clean working tree** — the driver records whether the tree was dirty and the
report flags such results as not reproducible from the recorded commit.

The generated comparison reports scaling exponents per platform, median times,
relative speed against a reference platform, and a per-platform noise fingerprint.
It warns loudly when results span different commits, `--level` settings or library
versions instead of averaging incomparable numbers.

Two limits are stated there and repeated here because they are easy to forget:

- **The compiler is not unified.** AppleClang on macOS, GCC on Linux. Every
  cross-platform difference is hardware *and* toolchain; none of it is a clean CPU
  comparison.
- **Anything smaller than a platform's noise figure is not a real difference.**

No benchmarking runs in CI: GitHub's shared runners vary by more than the effects
being measured.

### Dataset versions

A shared seed is not enough to make two platforms measure the same work.
`std::mt19937` is specified down to the bit, but `std::normal_distribution` and
`std::uniform_int_distribution` are **not** specified to produce the same sequence
across standard library implementations. libc++ and libstdc++ disagree, so the
first round of results had each platform discretizing a *different* dataset.

Dataset version 2 generates its data from integer operations and IEEE-754 addition
only — a 24-bit uniform built by shifting the engine output, and an Irwin-Hall
normal (twelve uniforms minus six, so no `libm` whose last-ulp behaviour is
likewise unguaranteed). Each run also records an FNV-1a checksum of the exact
bytes handed to the library, so `make bench-report` can *state* whether two
platforms measured the same work rather than assume it.

The report groups results by dataset version and never compares across groups.
**Version 1 results are retained but their cross-platform absolute times are not
interpretable**; their scaling exponents remain valid, being computed within a
single platform.

### Homogeneity is judged on the measured sources

Runs are compared by a hash of `src/` and `bench/`, not by commit SHA. A
docs-only commit cannot change a timing, and flagging results as incomparable
because the README moved is noise that trains people to ignore the warning.

### Clock ramp

Each run now spins the CPU for 1.5 s before anything is timed. Without it the
cells measured first — the small-n ones — are taken at idle clocks: the version 1
Strix Halo run finished **21.7% faster than it started**, which silently depressed
its small-n numbers and distorted its scaling ratios. The drift probe now warns in
both directions.

## Environment

| | |
|---|---|
| Machine | Apple M4 Max, 14 cores |
| OS | macOS 26.5.2 (arm64) |
| Compiler | Apple clang 21.0.0 (`clang-2100.1.1.101`) |
| Build | `CMAKE_BUILD_TYPE=Release`, `-O3`, C++17 |
| libtorch | 2.7.1 (via conan) |
| Library version reported | 2.1.3 (bump to 3.0.0 is T8.6) |
| Commit | Phase 3 complete (`3e0fb52`) |
| Dataset | 3 classes, class-conditional normals, seed 42 |

## Results

All times in milliseconds.

| benchmark | n | reps | min | median | mean |
|---|---:|---:|---:|---:|---:|
| CPPFImdlp::fit | 100 | 500 | 0.0149 | 0.0166 | 0.0174 |
| BinDisc::fit (uniform) | 100 | 500 | 0.0001 | 0.0002 | 0.0002 |
| BinDisc::fit (quantile) | 100 | 500 | 0.0004 | 0.0005 | 0.0005 |
| PKIDisc::fit (sqrt) | 100 | 500 | 0.0005 | 0.0005 | 0.0005 |
| CPPFImdlp::transform | 100 | 500 | 0.0001 | 0.0002 | 0.0002 |
| BinDisc::transform | 100 | 500 | 0.0001 | 0.0002 | 0.0002 |
| *(reference)* copy X + y | 100 | 500 | 0.0000 | 0.0000 | 0.0000 |
| CPPFImdlp::fit | 1 000 | 200 | 0.7795 | 0.8213 | 0.8277 |
| BinDisc::fit (uniform) | 1 000 | 200 | 0.0013 | 0.0016 | 0.0016 |
| BinDisc::fit (quantile) | 1 000 | 200 | 0.0048 | 0.0056 | 0.0057 |
| PKIDisc::fit (sqrt) | 1 000 | 200 | 0.0050 | 0.0057 | 0.0057 |
| CPPFImdlp::transform | 1 000 | 200 | 0.0020 | 0.0022 | 0.0022 |
| BinDisc::transform | 1 000 | 200 | 0.0015 | 0.0018 | 0.0017 |
| *(reference)* copy X + y | 1 000 | 200 | 0.0001 | 0.0001 | 0.0001 |
| CPPFImdlp::fit | 10 000 | 50 | 78.6715 | 80.7728 | 81.2756 |
| BinDisc::fit (uniform) | 10 000 | 50 | 0.0067 | 0.0072 | 0.0073 |
| BinDisc::fit (quantile) | 10 000 | 50 | 0.0593 | 0.0601 | 0.0643 |
| PKIDisc::fit (sqrt) | 10 000 | 50 | 0.0597 | 0.0656 | 0.0646 |
| CPPFImdlp::transform | 10 000 | 50 | 0.0349 | 0.0390 | 0.0389 |
| BinDisc::transform | 10 000 | 50 | 0.0178 | 0.0196 | 0.0227 |
| *(reference)* copy X + y | 10 000 | 50 | 0.0008 | 0.0011 | 0.0011 |
| CPPFImdlp::fit | 100 000 | 10 | 8248.06 | 8354.63 | 8354.81 |
| BinDisc::fit (uniform) | 100 000 | 10 | 0.1056 | 0.1183 | 0.1231 |
| BinDisc::fit (quantile) | 100 000 | 10 | 1.4817 | 1.4912 | 1.4935 |
| PKIDisc::fit (sqrt) | 100 000 | 10 | 1.4900 | 1.5122 | 1.5112 |
| CPPFImdlp::transform | 100 000 | 10 | 0.5707 | 0.5715 | 0.5750 |
| BinDisc::transform | 100 000 | 10 | 0.3713 | 0.3881 | 0.3909 |
| *(reference)* copy X + y | 100 000 | 10 | 0.0095 | 0.0095 | 0.0095 |

## Findings

### 1. `CPPFImdlp::fit` is quadratic, not O(n log n)

This is the headline result, and it was not anticipated by the release plan.

| n | min (ms) | factor vs. previous |
|---:|---:|---:|
| 100 | 0.0149 | — |
| 1 000 | 0.7795 | **52×** |
| 10 000 | 78.67 | **101×** |
| 100 000 | 8 248.06 | **105×** |

Ten times the data costs roughly **one hundred times** the work. That is
unambiguously O(n²). The `ARCHITECTURE.md` draft in the superseded 2.2.0 plan
claimed "MDLP: O(n log n) for sorting + O(n) per recursion level"; the measurement
contradicts it.

The cost is in `CPPFImdlp::getCandidate()` (`src/CPPFImdlp.cpp:140-172`). For every
class boundary in the interval it calls `metrics.entropy(start, idx)` and
`metrics.entropy(idx, end)`. Each *uncached* entropy call walks its whole interval
twice (`src/Metrics.cpp`), so summing over the O(n) boundary positions gives O(n²)
work per `getCandidate` call.

This was confirmed by instrumenting `Metrics::entropy` to count calls, cache hits
and elements actually scanned, rather than inferred from the timings:

| n | entropy calls | cache hits | elements scanned | scanned / n² |
|---:|---:|---:|---:|---:|
| 1 000 | 1 903 | 752 | 642 268 | 0.642 |
| 2 000 | 4 545 | 1 762 | 2 996 138 | 0.749 |
| 4 000 | 9 453 | 3 627 | 12 433 718 | 0.777 |
| 8 000 | 20 691 | 8 091 | 50 312 412 | 0.786 |
| 16 000 | 46 472 | 18 570 | 206 624 902 | 0.807 |

Elements scanned **quadruples on every doubling of n**, and `scanned / n²`
converges to a constant ≈0.8 — the definition of Θ(n²). The timing side agrees
independently: the log-log fit over the largest three sizes gives an exponent of
**2.03 with R² = 1.000** (see `docs/benchmarks-platforms.md`). Note the shape of it: the
*number* of entropy calls only doubles per doubling (so it is O(n)), but each call
rescans a large interval. Memoization is working — about 40% of calls hit the cache
— and is still powerless against this, because the misses are precisely the
expensive full-interval scans.

The fix is to compute the entropies incrementally — carry running per-class counts
as `idx` advances instead of rescanning from `start` each time — which turns
`getCandidate` into O(n) plus O(k) per step for k classes. **This was implemented
in Phase 7; see the section below.** It was a Phase 7 item
and is worth far more than the "20-30%" the original plan hoped for: at n = 100 000
the current implementation spends over eight seconds on a single feature.

**Practical consequence today:** discretizing a 100 000-row dataset with MDLP costs
~8 s *per feature*. Users with large datasets should prefer `BinDisc` or `PKIDisc`,
which are four orders of magnitude faster at that size, until Phase 7 lands.

### 1b. Confirmed on three platforms

Reproduced on two more machines at dataset version 2. Full tables in
[benchmarks-platforms.md](benchmarks-platforms.md).

The comparison is verified rather than assumed: all three runs report the **same
source hash** (`89bc5a73a4ae`), the **same dataset checksums** at every size, a
clean working tree and `--level full`. The platforms executed identical code over
identical bytes.

| Platform | ISA | Compiler | exponent | R² |
|---|---|---|---:|---:|
| Apple M4 Max | ARM | AppleClang 21.0.0 | **1.98** | 1.000 |
| AMD Ryzen 9 7950X3D | x86_64 | GCC 16.1.1 | **1.96** | 0.999 |
| AMD Ryzen AI Max+ 395 | x86_64 | GCC 15.3.1 | **1.93** | 0.999 |

Three microarchitectures, two instruction sets, three compilers. **The quadratic is
a property of the algorithm, not of any machine.** Version 1 measured 2.03 / 1.97 /
1.94 on different data per platform; changing the generator moved the exponent by
less than the fit's own resolution, which is itself a useful check.

The clock ramp did its job: the Strix Halo drifted −21.7% under version 1 and
+1.9% under version 2.

### 1c. Two toolchain anomalies, now isolated from the data

With the datasets proven identical, these cannot be explained by one platform
having been given less work. Measurement noise is 0.9–3.5%; the effects below are
240–520%, so they are real.

**`BinDisc::fit (quantile)` — Linux 3.0–3.2× slower at n = 100 000**, and 4.8–5.2×
slower at n = 10 000, yet *equal or faster* at n = 1 000 (0.56× on the Strix Halo).
The crossover sits between n = 1 000 and n = 10 000.

| n | M4 Max | 7950X3D | Ryzen AI Max+ 395 |
|---:|---:|---:|---:|
| 1 000 | 0.0053 | 0.0051 (0.96×) | 0.0030 (**0.56×**) |
| 10 000 | 0.0670 | 0.3233 (**4.83×**) | 0.3508 (**5.24×**) |
| 100 000 | 1.5063 | 4.5810 (**3.04×**) | 4.8791 (**3.24×**) |

`PKIDisc::fit` mirrors it exactly, as expected since it delegates. The work here is
essentially one `std::sort` over a by-value copy, so a 5× swing between adjacent
sizes points at either the sort implementation (libstdc++ vs libc++) or the
allocator behind the copy — glibc switches to `mmap` for large blocks, macOS does
not.

**`CPPFImdlp::transform` — Linux 2.4–2.8× slower, but only at n = 100 000.** At
n = 10 000 the 7950X3D is *faster* (0.73×) and at n = 1 000 the three are level.
The routine is an `upper_bound` over a handful of cut points plus `push_back` into
a buffer that already has capacity, so nothing in the algorithm explains a
size-dependent cliff.

Neither is diagnosable from the result files alone. The next step is a targeted
micro-benchmark on one Linux machine timing `std::sort` on its own, separated from
the by-value copy, at n = 1 000 / 10 000 / 100 000. That distinguishes the sort
implementation from the allocator in one run.

Both anomalies are in unsupervised code paths that are already four orders of
magnitude cheaper than `CPPFImdlp::fit`, so neither blocks the release.

### 1d. MDLP itself is genuinely faster on the AMD machines

Now that the inputs are verified identical, this comparison can be trusted:
`CPPFImdlp::fit` runs at **0.67× and 0.68×** of the M4 Max time at n = 100 000, and
the advantage holds across sizes. Both AMD parts land in the same place, which is
what one would expect if the effect is real rather than a scheduling artefact.

## Phase 7 result: the quadratic is gone

`getCandidate()` now carries per-class counts as `idx` advances instead of asking
`Metrics::entropy` to rescan the interval at every class boundary. Measured on the
M4 Max against the dataset version 2 baseline:

| n | before | after | speedup |
|---:|---:|---:|---:|
| 1 000 | 0.9514 ms | 0.0723 ms | **13×** |
| 10 000 | 82.03 ms | 1.048 ms | **78×** |
| 100 000 | 8 517 ms | 16.53 ms | **515×** |

Discretizing a 100 000-sample feature dropped from **8.5 seconds to 16.5
milliseconds**. The speedup grows with n, which is what replacing an O(n²) term
with an O(n·k) one looks like.

The scaling ratios confirm it. Ten times the data now costs 11.2× / 14.5× / 15.8×
instead of the previous ~100×; what remains is the `std::stable_sort` in
`sortIndices` plus the linear scans, i.e. roughly n log n.

Nothing else moved: `transform` and the `BinDisc` paths are within 10% of their
previous figures, as expected from a change confined to `getCandidate`.

### Equivalence

Cut points are **identical**, not merely close. `Metrics::entropyFromCounts` is now
the single implementation shared by the batch and incremental paths, so the two
cannot drift: same iteration order over labels, same types, same skipping of zero
counts, therefore bit-identical floating point.

Verified four ways. The full test suite passes unchanged, including the tests
that assert exact cut points on iris. A differential harness compared the old and
new implementations across 180 synthetic configurations — n from 50 to 4 000, 2 to
6 classes, seven duplicate densities, three parameter sets. The same harness then
ran both implementations over two real datasets and compared every feature:

| dataset | samples | features | classes | result |
|---|---:|---:|---:|---|
| mfeat-factors | 2 000 | 216 | 10 | identical on all 216 |
| miniboone | 130 064 | 50 | 2 | identical on all 50 |

Byte-identical cut points, recursion depths and transform outputs throughout.

### On real data the speedup is larger still

miniboone has 130 064 rows, so the quadratic term bit harder there than anywhere
in the synthetic benchmark:

| | per feature | all 50 features |
|---|---:|---:|
| before | 12.6-17.4 s | ~12.5 min |
| after | ~21 ms | **1.08 s** |

That is **600-770× per feature**, against 515× at n = 100 000 in the synthetic
benchmark — the gap widens with n, as an O(n²) → O(n·k) change should.

The per-feature figures were measured with nothing else running. The full 50-feature
old-implementation pass was measured at 17.3 minutes, but it ran in the background
while builds and tests competed for the machine, so ~12.5 min is the honest
projection from the uncontended per-feature times. It was run to completion for the
equivalence check, not for its timing.

On mfeat-factors, where n is only 2 000, the same change is worth 10.9×
(762 ms → 70 ms for all 216 features) — consistent with the 13× measured at
n = 1 000 synthetically, and confirmation that the O(k) cost the fix introduces is
not a problem at 10 classes.

### Confirmed on all three platforms

Re-run after Phase 7 with identical dataset checksums and an identical source
hash (`c5cee47706d5`) on all three machines:

| Platform | exponent before | exponent after |
|---|---:|---:|
| Apple M4 Max | 1.98 | **1.17** |
| AMD Ryzen 9 7950X3D | 1.96 | **1.21** |
| AMD Ryzen AI Max+ 395 | 1.93 | **1.23** |

### The Linux advantage on MDLP inverted

This is the most consequential secondary effect, and it was predicted here before
the measurement existed.

`CPPFImdlp::fit` relative to the M4 Max, at n = 100 000:

| | before Phase 7 | after Phase 7 |
|---|---:|---:|
| AMD Ryzen 9 7950X3D | 0.67× (faster) | **1.10× (slower)** |
| AMD Ryzen AI Max+ 395 | 0.68× (faster) | **1.12× (slower)** |

While `fit` was quadratic, the entropy rescans dwarfed everything and the AMD
machines' raw throughput won by a third. With that term gone, what remains is
`std::stable_sort` in `sortIndices` plus linear scans — and the same toolchain
penalty that shows up in `BinDisc::fit (quantile)` (still 2.9-3.2× slower on Linux)
now shows through `fit` itself. The quadratic had been masking it.

**In absolute terms this is not worth worrying about**: `fit` at n = 100 000 costs
16.3 ms on macOS against 17.9-18.1 ms on Linux. Everything got roughly 500×
cheaper, so a 10% relative gap is 1.7 ms.

**In terms of what to optimise next it matters a lot.** `fit` is now sort-dominated,
which means the sort is the next lever — and unlike a toolchain switch, a better
sort would help every platform. Settling whether libstdc++'s `std::stable_sort` is
the culprit is now worth the standalone experiment, because the answer changes what
gets optimised rather than merely which compiler to prefer.

The other two anomalies are unchanged and now proportionally larger, since the
denominator shrank: `CPPFImdlp::transform` is 2.3-2.6× slower on Linux and has gone
from 0.007% of a fit to about 4%; `BinDisc::fit (quantile)` remains 2.9-3.2× slower.

### What this changes downstream

The two Linux anomalies recorded above were measured when `CPPFImdlp::fit`
dominated everything by four orders of magnitude. That is no longer true: at
n = 100 000 `fit` is now 16.5 ms against `transform`'s 0.63 ms, so `transform` has
gone from 0.007% of the work to about 4%. The `std::stable_sort` in `sortIndices`
is now a meaningful share of `fit` itself. **Re-running the cross-platform
comparison is worthwhile, and the toolchain question is now more relevant than it
was**, not less.

### 2. Input copying is negligible, which reframes Phase 5

The `copy X + y` reference row measures exactly what move semantics (T5.1) would
eliminate. At n = 100 000 it is **0.0095 ms against 8 248 ms** of `CPPFImdlp::fit`
— about **0.0001%** of the call.

So T5.1 should be justified on memory and API ergonomics, not throughput. There is
no measurable speed-up available there, and claiming one would be dishonest.
`CPPFImdlp::fit` currently holds `X`, `y`, `indices`, and (since Phase 3) a second
copy of `y` and `indices` inside `Metrics` — that duplication is the real cost, and
it is a memory cost.

The one place copying is a visible share of the work is `BinDisc::fit (uniform)`,
where the 0.0095 ms copy sits against 0.1056 ms of total work (~9%). But that
strategy does not copy the input at all today — the figure is only a scale
reference. `BinDisc::fit (quantile)` genuinely copies to sort, and there a move
overload could avoid the copy when the caller surrenders its data.

### 3. The unsupervised discretizers scale as expected

`BinDisc::fit (uniform)` is linear (a single min/max pass): 10× the data costs
about 10-16× the time. `BinDisc::fit (quantile)` and `PKIDisc::fit` are dominated
by a sort and scale close to n log n. `transform` is linear for both. Nothing here
needs attention.

`PKIDisc::fit (sqrt)` tracks `BinDisc::fit (quantile)` almost exactly, as expected
— it selects a bin count and delegates.

## Implications for the remaining phases

| Phase | What the baseline says |
|---|---|
| 5 — move semantics | Justify on memory and ergonomics. No throughput gain is available; T5.3 should report the measured null result rather than hunt for a delta. |
| 7 — performance | Retarget onto `getCandidate`'s quadratic entropy recomputation. This is the only finding of real magnitude, and it dwarfs everything else in the plan. |
| 8 — docs | Any complexity claim written into `ARCHITECTURE.md` must match this table. MDLP is O(n²) today. |

## Phase 5 — measured impact of move semantics

Measured after T5.1 and T5.2 landed, on the same machine. The Phase 0 table above
stays frozen as the reference; these are the new rows.

### Run-to-run variance comes first

Before reading any delta, note how noisy this machine is at the largest size. The
same unchanged benchmark, `BinDisc::fit (quantile)` at n = 100 000, measured across
three runs:

| run | min (ms) |
|---|---:|
| 1 (Phase 0) | 1.4817 |
| 2 | 1.4347 |
| 3 | 1.5086 |

That is a **~5% spread with nothing changed**. Any effect smaller than that cannot
be claimed from these measurements. `CPPFImdlp::fit` at the same size is steadier
(8248.06 / 8343.75 / 8349.55, ~1.2% spread) because each rep is so long.

### T5.1 — `fit()` with rvalues: no measurable change

| n | fit (copy) | fit (move) | delta |
|---:|---:|---:|---:|
| 100 | 0.0145 | 0.0145 | 0.0% |
| 1 000 | 0.7114 | 0.7080 | −0.5% |
| 10 000 | 79.29 | 79.84 | +0.7% |
| 100 000 | 8 349.55 | 8 410.30 | +0.7% |

Exactly the null result Phase 0 predicted: the copies removed were 0.0001% of the
call, so no delta was available to find. The two directions of the sign here are
noise, not a regression.

For `BinDisc` QUANTILE, where the copy is a real share of a much cheaper operation,
the comparison needs the pool-copy control row — the plain row reuses one hot
buffer, while the move row must touch a fresh pool slot each rep, and that
difference in cache locality is larger than the effect being measured:

| n | quantile (pool copy) | quantile (move) | delta |
|---:|---:|---:|---:|
| 100 | 0.0004 | 0.0004 | 0.0% |
| 1 000 | 0.0047 | 0.0047 | 0.0% |
| 10 000 | 0.0606 | 0.0605 | −0.2% |
| 100 000 | 1.4568 | 1.4328 | −1.6% |

The move version is consistently equal or slightly faster, and at n = 100 000 it
avoids a 400 KB copy. But −1.6% sits well inside the ~5% run-to-run variance, so
**this is a memory result, not a speed result.**

An earlier run without the control row showed the move version 8.5% *slower*,
which is what prompted adding the control. That figure was run-to-run noise
compounded by the pool-locality difference; it did not survive a fair comparison.

### T5.2 — `transform()` into a caller buffer: no measurable change

| n | transform (returns ref) | transform (caller buffer) |
|---:|---:|---:|
| 100 | 0.0001 | 0.0001 |
| 1 000 | 0.0018 | 0.0019 |
| 10 000 | 0.0351 | 0.0350 |
| 100 000 | 0.5547 | 0.5666 |

Also null, and for a reason worth recording: the returning overload already calls
`clear()` rather than reallocating, so repeated calls at the same size reuse the
existing capacity. There was never a per-call allocation to remove.

The real benefit is one this benchmark cannot show — a caller who needs to *own*
the result currently has to copy out of the internal buffer, because the returned
reference is invalidated by the next `transform()`. The two-argument overload
removes that copy and lets the caller keep one buffer across calls.

### Verdict

Phase 5 delivered what Phase 0 said was available: **memory and ergonomics, no
throughput**. All rows sit within the ≤5% regression ceiling. The plan's original
"20-30% performance improvement" would not have come from here, and the honest
report is that it did not come from here.

The performance work that matters is still Phase 7 — the quadratic `getCandidate`.

## Toolchain diagnostic (`make sortbench`)

`bench/sortbench/` is a standalone program that replicates the library's three hot
loops with **zero dependencies** — no libtorch, no conan, no CMake — so one Linux
machine can build it three ways and separate two variables the project itself
cannot:

```
g++     -O3                   GCC   + libstdc++   (what the project uses)
clang++ -O3                   clang + libstdc++   (isolates the compiler)
clang++ -O3 -stdlib=libc++    clang + libc++      (isolates the standard library)
```

The project cannot do this itself: libtorch comes from conan prebuilt against
libstdc++, and libstdc++ and libc++ have incompatible ABIs for `std::string` and
`std::vector`, so building mdlp against libc++ would mean rebuilding libtorch from
source.

Reading it: if clang+libstdc++ matches GCC and clang+libc++ is the fast one, the
**standard library** is responsible. If both clang builds are fast, the
**compiler** is. If all three match, neither is.

Fedora needs `dnf install clang libcxx-devel`; Debian and Ubuntu need
`apt install clang libc++-dev`. Unavailable builds are skipped, not fatal.

Results are stored the same way as the library benchmark, so they travel between
machines through git:

```bash
make sortbench          # runs, and stores docs/benchmarks/sortbench/<slug>__<sha>.json
git add docs/benchmarks/sortbench && git commit && git push
# anywhere, once gathered
make sortbench-report   # regenerates docs/benchmarks-sortbench.md
```

The report states a **verdict per machine** — `stdlib`, `compiler`, `both`,
`neither` or `inconclusive` — by comparing clang+libstdc++ against GCC+libstdc++
(which isolates the compiler) and clang+libc++ against clang+libstdc++ (which
isolates the library), on `stable_sort<index>` at the largest size. A swap has to
beat 15% to count, comfortably above the 0.9-3.5% noise the library benchmark
reports. `SORTBENCH_STORE=0 make sortbench` runs without writing anything.

**Limitation, stated plainly:** these are *replicas* of the hot loops, not the
library. A result is a strong hypothesis about the cause, not a measurement of
mdlp.

### Both Linux machines: the answer is GCC's *version*, not clang

The two Linux runs disagreed, and the disagreement is the finding.

| Machine | GCC | Clang | verdicts |
|---|---|---|---|
| Ryzen 9 7950X3D | **16** | 22 | `sort<float>`=stdlib, everything else `neither` |
| Ryzen AI Max+ 395 | **15** | 21 | `sort<float>`=both, everything else `compiler` |

Comparing one build label across machines separates compiler *vendor* from
compiler *version*. The clang columns agree between the two machines to within
4-13%, so the hardware is comparable. The GCC columns do not:

| Shape (n = 100 000) | GCC 15 vs GCC 16 | Clang 21 vs Clang 22 |
|---|---:|---:|
| `sort<float>` | **+57%** | +9% |
| `stable_sort<index>` | **+58%** | +13% |
| `transform` | **+68%** | +4% |

**GCC 16 produces code as good as clang; GCC 15 does not.** The `compiler` verdict
on the Strix Halo is really a GCC 15 problem, and on the machine already running
GCC 16 there is no compiler penalty left to recover.

This is a limitation of the diagnostic worth stating: it reports `compiler` when
clang beats GCC, and cannot tell on its own whether the cause is the vendor or the
release. The cross-machine table exists to answer that.

### The one finding that holds everywhere: libstdc++'s `std::sort`

Independent of machine, compiler and compiler version:

| Machine | libc++ vs libstdc++ on `sort<float>` |
|---|---:|
| Ryzen 9 7950X3D (Clang 22) | **2.65× faster** |
| Ryzen AI Max+ 395 (Clang 21) | **2.91× faster** |

And the superlinear jump the library benchmark showed for `BinDisc::fit (quantile)`
tracks the same library:

| build | n = 1 000 → 10 000 |
|---|---:|
| GCC 15 + libstdc++ | 122× |
| Clang 21 + libstdc++ | 106× |
| GCC 16 + libstdc++ | 65× |
| Clang 22 + libstdc++ | 59× |
| any + libc++ | **13×** |

Every libstdc++ build blows up; no libc++ build does. GCC 16 halves the magnitude
but does not remove it. This is a property of libstdc++'s `std::sort`, and it is
what makes `BinDisc::fit (quantile)` — and therefore `PKIDisc`, which always
selects the QUANTILE strategy — 3× slower on Linux.

Note the scope: `BinDisc` with `strategy_t::UNIFORM` does **not** sort
(`fit_uniform` only calls `minmax_element`), so it is unaffected. `CPPFImdlp` sorts
twice, but only `sortIndices` matters; the `sort` over `cutPoints` handles a few
dozen elements.

### `transform` is still unexplained

Clang buys 38-40% on GCC 15 and nothing on GCC 16, yet **every** Linux build sits
at 1.04-1.19 ms at n = 100 000 against macOS's 0.22 ms. A 5× gap survives holding
compiler family and standard library constant, so it is microarchitectural. The
toolchain is ruled out rather than implicated, and the function-pointer hypothesis
is dead: `transform(fnptr)` and `transform(direct)` measure within noise on every
build on every machine.

**Caveat throughout:** one run per build per machine, and one cell is visibly
noisy — `transform(direct)` at n = 10 000 under Clang 21 + libstdc++ reads
0.1003 ms against 0.0238 ms for `fnptr`, while at n = 100 000 the two are equal.
Treat single cells with suspicion; the patterns above hold across sizes and across
machines.

### What the macOS run established

macOS cannot answer the toolchain question at all — `g++` is a symlink to clang and
libstdc++ is unavailable, so all three builds resolve to AppleClang + libc++ and
land within noise of each other. The script detects this and says so rather than
presenting three identical columns as a comparison.

It did settle one thing that was previously an assumption. At n = 100 000,
`stable_sort<index>` costs **6.4 ms** against `CPPFImdlp::fit`'s **16.3 ms**: the
sort is roughly **39% of fit** now that Phase 7 removed the quadratic term. Calling
`fit` sort-dominated is a measurement, not a guess.

It also weakens one hypothesis. `transform` is written with the bound function
selected into a *function pointer*, which cannot be inlined; a variant with the
branch hoisted out of the loop measures identically on clang (0.2302 vs 0.2308 ms).
Whether GCC behaves the same is exactly what the Linux run will show.

## Regression policy

The success criteria in the release plan allow at most a **5% regression** against
the minimum times in this table. Re-run `make bench` on the same machine before
tagging 3.0.0 and record the comparison here.
