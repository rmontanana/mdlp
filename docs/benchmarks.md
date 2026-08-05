# Performance baseline — 3.0.0

Baseline measurements for the 3.0.0 release, taken before any performance work.
This is the reference every later phase compares against, so that improvements can
be reported as measured deltas rather than asserted.

Corresponds to Phase 0 of [RELEASE_PLAN_V3.md](../RELEASE_PLAN_V3.md).

## Running it

```bash
export PATH="$HOME/miniconda3/bin:$PATH"   # conan
make bench
```

`make bench` forces a Release build (`-O3`) into `build_bench/` with
`-DENABLE_BENCHMARK=ON`. The benchmark target is off by default and is never part
of `make test`. A full run takes roughly two minutes, almost all of it in the
largest `CPPFImdlp::fit` cell — see the scaling note below.

The harness has no external dependencies. Data is generated from class-conditional
normal distributions with a fixed seed (42), so every run measures identical work.
Timings are min / median / mean over repetitions after warmup; **compare the
minimum across runs**, as it is the most stable figure on a loaded machine.

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
converges to a constant ≈0.8 — the definition of Θ(n²). Note the shape of it: the
*number* of entropy calls only doubles per doubling (so it is O(n)), but each call
rescans a large interval. Memoization is working — about 40% of calls hit the cache
— and is still powerless against this, because the misses are precisely the
expensive full-interval scans.

The fix is to compute the entropies incrementally — carry running per-class counts
as `idx` advances instead of rescanning from `start` each time — which turns
`getCandidate` into O(n) plus O(k) per step for k classes. This is a Phase 7 item
and is worth far more than the "20-30%" the original plan hoped for: at n = 100 000
the current implementation spends over eight seconds on a single feature.

**Practical consequence today:** discretizing a 100 000-row dataset with MDLP costs
~8 s *per feature*. Users with large datasets should prefer `BinDisc` or `PKIDisc`,
which are four orders of magnitude faster at that size, until Phase 7 lands.

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

## Regression policy

The success criteria in the release plan allow at most a **5% regression** against
the minimum times in this table. Re-run `make bench` on the same machine before
tagging 3.0.0 and record the comparison here.
