# mdlp Library — Release 3.0.0 Plan

**Target version:** 3.0.0 (major — breaking changes)
**Branch:** `release/3.0.0`
**Status:** Phases 0, 1, 2, 3 and 5 complete; Phases 4, 6, 7, 8 pending
**Supersedes:** `RELEASE_PLAN_2.2.0.md` (see [Appendix B](#appendix-b--corrections-to-the-superseded-220-plan))

> **This document is the single source of truth for the 3.0.0 release.**
> `RELEASE_PLAN_2.2.0.md` is superseded and must not be used to drive work: large
> parts of it describe as "pending" things that are already implemented, its
> roadmap contains copy/paste corruption, and two of its code snippets would
> introduce regressions if applied literally. Appendix B records what was salvaged
> from it and what was rejected, so it can be deleted safely.

---

## 1. Verified baseline

Measured on commit `e143bbc`, branch `release/3.0.0`, before any 3.0.0 work:

| Item | State |
|------|-------|
| Debug build (`make debug`) | OK |
| Release build (`make release`) | OK |
| Test suite (`ctest`) | **82/82 passing** |
| Version in `CMakeLists.txt:4` | **2.1.3** — not yet bumped |
| Benchmarks in repo | **none** |

**Toolchain note.** Neither `conan` nor the Perl `GD` module that `genhtml` needs
is reachable from a default shell. `make debug` / `make release` / `make test` all
need this first:

```bash
export PATH="$HOME/miniconda3/bin:$PATH"       # conan 2.26.1
export PERL5LIB="$HOME/perl5/lib/perl5:$PERL5LIB"  # GD.pm, for genhtml
export PATH="$HOME/perl5/bin:$PATH"
```

Without `PERL5LIB`, `make test` runs the suite and then dies at the coverage
report with `required module GD.pm not found` — the tests themselves have already
passed by that point. With it, `make test` completes and updates the README badge.

`conanfile.py:35-40` derives the package version by regex from the
`project(fimdlp VERSION ...)` line in `CMakeLists.txt`, so bumping `CMakeLists.txt`
is sufficient — there is no second place to edit.

---

## 2. Design philosophy (unchanged in 3.0.0)

All discretizers accept the same `fit(X, y)` signature, so an experimentation
platform can use a single code path:

```cpp
void run_experiment(Discretizer& disc, samples_t& X, labels_t& y) {
    disc.fit(X, y);   // works for CPPFImdlp, BinDisc and PKIDisc alike
    auto result = disc.transform(X);
}
```

- **Supervised** (`CPPFImdlp`) uses `y`.
- **Unsupervised** (`BinDisc`, `PKIDisc`) accept `y` and ignore it, deliberately.

This is not up for revision in 3.0.0.

---

## 3. Inventory: what is already implemented

The previous plans repeatedly listed work that was already done. This section is
the corrective — every row below was verified against the source on the baseline
commit and requires **no further work**.

| Item | Where | Notes |
|------|-------|-------|
| Doxygen docs on all discretizer headers | `Discretizer.h`, `CPPFImdlp.h`, `BinDisc.h`, `PKIDisc.h` | Phase 1 deliverable |
| Interface rationale documented | `README.md`, `CLAUDE.md`, `Discretizer.h:23-68` | Phase 1 deliverable |
| Explicit `PKIDisc` constructor | `PKIDisc.cpp:11-12` | Phase 1 deliverable |
| Tensor validation: rank, dtype, numel match, non-empty | `Discretizer.cpp:43-57`, `:67-75`, `:85-99` | **Incomplete** — see D1 |
| `transform()` input validation (empty data, unfitted) | `Discretizer.cpp:14-19` | |
| Bounds-checked `X`/`y` access helpers | `CPPFImdlp.h:135-169` | `safe_X_access`, `safe_y_access` |
| Underflow-checked subtraction | `CPPFImdlp.h:178-184` | `safe_subtract` |
| Constructor parameter validation | `CPPFImdlp.cpp:23-31` | min_length, max_depth, proposed |
| `BinDisc` input validation | `BinDisc.cpp:26-31` | empty, size < n_bins |
| `linspace` NaN/inf validation | `BinDisc.cpp:55-63` | |
| `percentile` input validation | `BinDisc.cpp:82-88` | |
| `sortIndices` bounds check | `CPPFImdlp.cpp:204-206` | |
| Mutex-guarded entropy/IG caches | `Metrics.h:19`, `Metrics.cpp:35,50,97,108,133` | **Design flawed** — see D3/D4 |
| Defensive index guards in `Metrics` | `Metrics.cpp:22,26,61,68-70,82-84` | |

**Not implemented, despite prior claims to the contrary:**

- Move semantics on `fit()` — the old plan marked this `[x] ... (planned)`, a
  self-contradiction. There is not a single `samples_t&&` / `labels_t&&` overload
  in `src/`. Tracked here as T5.1.
- `src/Exceptions.h`, `src/Config.h` — neither file exists.
- `tests/Security_unittest.cpp`, `tests/Performance_unittest.cpp` — neither exists.
- `SECURITY.md`, `ARCHITECTURE.md` — neither exists.
- Version bump to 3.0.0.

---

## 4. Confirmed defects

Each entry below was verified on the baseline commit. D1 and D2 are the reason
this release exists; neither appears in the prior V3 plan.

### D1 — Non-contiguous tensors are silently misread (severity: HIGH)

`fit_t`, `transform_t` and `fit_transform_t` validate rank, dtype and element
count, then do raw pointer arithmetic:

```cpp
// Discretizer.cpp:59-61
auto num_elements = X_.numel();
samples_t X(X_.data_ptr<precision_t>(), X_.data_ptr<precision_t>() + num_elements);
labels_t y(y_.data_ptr<int>(), y_.data_ptr<int>() + num_elements);
```

A 1-D **non-contiguous** view passes every existing check but has a stride greater
than 1, so `data_ptr() + numel` walks over unrelated memory. There is no exception
and no crash — the results are simply wrong.

**Reproduced:** a `{10,2}` tensor whose column 0 is `0..9` and column 1 is
`100..109`; `base.select(1, 0)` yields a 1-D tensor of `0..9` with `numel == 10`
and `is_contiguous() == false`. Feeding it to `BinDisc(3, UNIFORM).fit_t()`
produced cut points **`0, 34.6667, 69.3333, 104`** — derived from the interleaved
buffer `0,100,1,101,…` rather than from the logical values `0..9`. Correct output
would span `0..9`.

This is the most severe issue in the codebase: it corrupts results silently in the
exact integration path (feeding a column view of a 2-D dataset) that callers are
most likely to use. Device residency (`is_cpu()`) is likewise unchecked, and a
CUDA tensor's `data_ptr()` would be far worse than wrong.

Note the superseded 2.2.0 plan *did* call for a contiguity check (its §1.1); the
prior V3 plan dropped it. It is reinstated here.

### D2 — `Discretizer::direction` is never initialized (severity: HIGH, UB)

```cpp
// Discretizer.h:136
bound_dir_t direction;   // no initializer
```

`direction` is read on every `transform()` (`Discretizer.cpp:27`). It is assigned
in only three places — `CPPFImdlp.cpp:33`, `BinDisc.cpp:35`, `BinDisc.cpp:38` —
all inside the *parameterized* `CPPFImdlp` constructor or inside `BinDisc::fit`.
`CPPFImdlp() = default` (`CPPFImdlp.h:77`) therefore leaves it indeterminate, and
`Discretizer_unittest.cpp:270` exercises exactly that path:

```cpp
Discretizer* disc = new CPPFImdlp();
disc->fit(X[1], y);
auto computed = disc->transform(X[1]);   // reads indeterminate `direction`
```

The test passes today only because the garbage value happens not to compare equal
to `bound_dir_t::LEFT`. This is undefined behaviour, not a latent style issue.

**Related dead code:** all three assignment sites set `RIGHT`. `bound_dir_t::LEFT`
is never assigned anywhere in the library, tests or sample, so the `std::lower_bound`
branch of `transform()` is unreachable. `direction` is currently a knob that cannot
be turned. This matters for T6.1 (bound modes): the feature must be *built*, not
merely exposed.

### D3 — `Metrics` reference members make `setData` a no-op (severity: MEDIUM)

`Metrics` holds `labels_t& y` and `indices_t& indices` (`Metrics.h:16-17`), and
`CPPFImdlp` constructs it from its own members (`CPPFImdlp.h:119`):

```cpp
Metrics metrics = Metrics(y, indices);
```

`Metrics::setData` then assigns *through* those references (`Metrics.cpp:36-37`):

```cpp
indices = indices_;
y = y_;
```

Because the caller passes `CPPFImdlp`'s own `y` and `indices`
(`CPPFImdlp.cpp:65`), this is self-assignment. The only effects that survive are
recomputing `numClasses` and clearing the caches. A reference member cannot be
rebound in C++, so the API promises a data swap it structurally cannot perform.
Any future caller that passes *different* vectors will silently overwrite the
originally-referenced ones instead.

### D4 — `Metrics` is non-copyable, so `CPPFImdlp` is too (severity: MEDIUM)

`Metrics` contains a `std::mutex` (`Metrics.h:19`), which is neither copyable nor
movable. `CPPFImdlp` holds a `Metrics` by value, so its copy and move constructors
are implicitly deleted. This blocks T5.1 (move semantics) outright and must be
resolved as part of the `Metrics` redesign, not after it.

### D5 — Cache keys narrow `size_t` to `int` (severity: MEDIUM)

```cpp
// typesFImdlp.h:22-23
typedef std::map<std::pair<int, int>, precision_t> cacheEnt_t;
typedef std::map<std::tuple<int, int, int>, precision_t> cacheIg_t;
```

Every call site passes `size_t` (`Metrics::entropy(size_t, size_t)`,
`informationGain(size_t, size_t, size_t)`). Keys are silently narrowed on insert
and lookup. Beyond `INT_MAX` elements distinct intervals can collide onto the same
key and return a wrong cached entropy. Not reachable at current test sizes, but it
is a correctness bug, not a style preference.

### D6 — `entropy` size guard underflows (severity: LOW)

```cpp
// Metrics.cpp:45
if (end - start < 2) return 0;
```

Computed in `size_t`. When `end < start` the subtraction wraps to a huge value, the
guard does not fire, and execution continues past it. Today the outcome is benign —
the subsequent loops do not execute and the function returns 0 through a different
path — so this is a clarity and robustness defect rather than a live failure. It
should still be an explicit `end <= start || end - start < 2`.

### D7 — Signed/unsigned comparison in `linspace` (severity: LOW)

```cpp
// BinDisc.cpp:70
for (size_t i = 0; i < num; ++i)   // num is int
```

Harmless in practice (`num >= 2` is validated above), but it is a warning the build
should not carry once `-Wall -Wextra` is enabled (T8.3).

---

## 5. Phases and tasks

Ordering is driven by dependency, not by severity alone: Phase 3 (`Metrics`)
unblocks Phase 5 (move semantics), and Phase 0 must precede Phase 7 or the
performance goals stay unmeasurable.

### Phase 0 — Measurement baseline ✅ COMPLETE

- [x] **T0.1** `bench/benchmark.cpp` covers `CPPFImdlp::fit`, `BinDisc::fit` (both
  strategies), `PKIDisc::fit` and `transform` at n = 10², 10³, 10⁴, 10⁵, plus a
  reference row measuring input-copy cost alone. Dependency-free; fixed seed.
- [x] **T0.2** Results recorded in [docs/benchmarks.md](docs/benchmarks.md) with
  machine, compiler and flags.
- [x] **T0.3** Behind `ENABLE_BENCHMARK` (default OFF), Release-only, run via
  `make bench` into `build_bench/`. Never part of `make test`.
- [x] **T0.4** The benchmark emits JSON (`--json`, `--level quick|full`) and
  `scripts/benchmarks.py run` fingerprints the machine — CPU, cores, RAM, OS,
  kernel, compiler, git commit — storing the result under
  `docs/benchmarks/results/<slug>__<sha>.json`, versioned in git.
- [x] **T0.5** `make bench-report` merges every stored result into
  [docs/benchmarks-platforms.md](docs/benchmarks-platforms.md): scaling exponents
  per platform, median times, relative speed, and a per-platform noise fingerprint.

**Methodology, decided deliberately:**

- **Fixed repetition counts on every platform, and the median for cross-platform
  comparison.** The minimum of a sample shrinks as the sample grows, so comparing
  minima taken with different rep counts would favour whichever machine ran more
  of them. The minimum stays the statistic for before/after checks on one machine.
- **The compiler is not unified** (AppleClang on macOS, GCC on Linux). Every
  cross-platform difference is hardware *and* toolchain, and the report says so
  rather than pretending to compare CPUs.
- **Scaling exponents are fitted over the largest three sizes only.** Small-n
  points sit on the flat, overhead-dominated part of the curve and bias the
  exponent downwards — `CPPFImdlp::fit` fits 1.86 over all four sizes but 2.04
  over the largest three, and the latter is the real complexity. Fits whose own
  points are still under 0.01 ms are flagged as indicative.
- **The report refuses to hide inhomogeneity:** it warns when results span
  different commits, `--level` settings, library versions, or a dirty tree.
- **No CI benchmarking.** GitHub's shared runners vary by more than the effects
  being measured; recording those numbers would manufacture false confidence.
- **Portable dataset generation (version 2).** A shared seed is not enough: the
  standard library *distributions* are not specified to produce the same sequence
  across implementations, so the first round of results had each platform
  discretizing different data. Generation now uses integer operations and IEEE-754
  addition only, and each run records an FNV-1a checksum of the bytes it measured
  so the report can state, rather than assume, that platforms did equal work.
- **Homogeneity judged on a hash of `src/` + `bench/`**, not the commit SHA — a
  docs-only commit cannot change a timing, and spurious warnings train people to
  ignore real ones.
- **A 1.5 s clock ramp before anything is timed.** Without it the first cells
  measured are taken at idle clocks; the version 1 Strix Halo run finished 21.7%
  *faster* than it started.

**Cross-platform outcome.** `CPPFImdlp::fit` measures an exponent of 2.03 (M4 Max,
AppleClang), 1.97 (7950X3D, GCC 16) and 1.94 (Ryzen AI Max+ 395, GCC 15), all with
R² = 1.000. Three microarchitectures, two ISAs, three compilers: **the quadratic is
algorithmic**, and Phase 7 is justified without qualification.

**Two findings change the rest of the plan:**

1. **`CPPFImdlp::fit` is O(n²), not O(n log n).** Ten times the data costs ~100×
   the work; n = 100 000 takes **8.2 seconds per feature**. Confirmed by
   instrumenting `Metrics::entropy`: elements scanned quadruples per doubling of n
   and `scanned / n²` converges to ≈0.8. The cost is `getCandidate()` rescanning
   the interval for every boundary point. Memoization is working (~40% hit rate)
   and cannot help, because the misses are the full-interval scans. **This
   retargets Phase 7** and is worth orders of magnitude, not the 20-30% originally
   hoped for.

2. **Input copying is negligible.** Copying X and y costs 0.0095 ms against
   8 248 ms of `fit` at n = 100 000 — 0.0001%. **Phase 5 must be justified on
   memory and ergonomics, not speed**; T5.3 should report the null result honestly
   rather than hunt for a delta that is not there.

### Phase 1 — Documentation & interface clarity ✅ COMPLETE

Delivered in commit `e143bbc`. See the inventory in §3. No open tasks.

### Phase 2 — Confirmed correctness defects ✅ COMPLETE

- [x] **T2.1** (D1) Non-contiguous and non-CPU tensor validation. **Resolution:
  accept non-contiguous input** via `contiguous()` rather than rejecting it — a
  column view of a 2-D dataset is the most natural way to feed one feature at a
  time, and rejecting would force every caller to add `.contiguous()` by hand.
  `contiguous()` is a no-op when the input already is, so the common path costs
  nothing. Non-CPU tensors are rejected. Documented in the Doxygen blocks of all
  three tensor methods.
- [x] **T2.2** (D1) Validation extracted into `Discretizer::validate_tensor` and
  `Discretizer::validate_pair`. The singular/plural empty-tensor messages were
  preserved deliberately: they are correct for their respective arities, not drift.
  All pre-existing exception messages are unchanged, so no existing test needed
  editing.
- [x] **T2.3** (D2) `direction` given an in-class initializer
  (`= bound_dir_t::RIGHT`), plus regression test
  `Discretizer.DefaultConstructedCPPFImdlpTransformsDeterministically`.
- [x] **T2.4** (D6) `entropy` guard is now `end <= start || end - start < 2`.
- [x] **T2.5** (D7) Signed/unsigned comparison in `linspace` fixed.
- [x] **T2.6** (D5) `cacheEnt_t` / `cacheIg_t` keys retyped to `size_t`.

**Verification.** 85/85 tests pass (82 pre-existing, unmodified, plus 3 new).
Debug and release builds are warning-free. Coverage of `src/` is 99.1% lines /
100% functions; the four uncovered lines are gcov closing-brace artifacts in
`CPPFImdlp.h` (:148, :169, :184) and `Discretizer.cpp` (:34), all in code Phase 2
did not touch.

The two contiguity tests were confirmed to have teeth: with `contiguous()`
temporarily reverted they fail, and they pass with it restored. The `direction`
test cannot be shown to fail the same way — the pre-fix behaviour was
*indeterminate*, not reliably wrong, so it passed before the fix too. It locks in
the now-defined behaviour rather than demonstrating the old bug.

### Phase 3 — `Metrics` redesign ✅ COMPLETE

Addressed D3 and D4 together; both stemmed from the same design choice.

- [x] **T3.1** Reference members replaced with **owning copies**, so `setData`
  genuinely switches to the new data and leaves the caller's vectors untouched.
  Owning also avoids the trap a pointer member would have created: `Metrics` is a
  member of `CPPFImdlp` and would have pointed at its *sibling* members, so every
  copy or move of `CPPFImdlp` would need custom constructors to re-point it or the
  pointers would dangle. Owning keeps the rule of zero.
- [x] **T3.2** The mutex is removed, so `Metrics` is copyable and movable by rule
  of zero — and `CPPFImdlp` with it. Asserted by
  `Metrics.IsCopyableAndMovable` and `FImdlp.IsCopyableAndMovable`.
- [x] **T3.3** Threading contract documented in the `Metrics` class doc: not
  thread-safe, external synchronization required, and explicitly noting that
  `entropy()` and `informationGain()` mutate state despite reading like queries.
  Distinct instances share nothing.
- [x] **T3.4** `setData` covered by `SetDataDoesNotClobberTheConstructorVectors`
  (the regression) and `SetDataSwitchesToTheNewDataAndDropsStaleCache` (a guard
  against a stale cache surviving a data switch).

**Also removed:** the `numClasses` member. It was assigned in the constructor and
in `setData` but never read anywhere, so each `setData` paid a full pass plus a
`std::set` allocation for nothing. `computeNumClasses()` itself is unchanged and
is still used by `CPPFImdlp::mdlp()`. Removing it did drop coverage of the
out-of-range guard in `computeNumClasses`, which that dead call had been covering
incidentally; `TestMetrics.NumClassesOutOfRange` now covers it deliberately.

**Trade-off accepted:** `CPPFImdlp` copies `y` and `indices` into its `Metrics`,
so both vectors now exist twice for the duration of a fit. Correctness and safe
copy/move semantics were judged worth the memory. Making `Metrics` the single
owner and having `CPPFImdlp` read through it would remove the duplication; that is
a larger refactor of `safe_X_access`/`safe_y_access` and is a candidate for
Phase 7.

**Verification.** 90/90 tests pass. Debug and release builds are warning-free.
Coverage of `src/` is 99.1% lines / 100% functions, with `Metrics.cpp` at 100%;
the four uncovered lines are the same gcov closing-brace artifacts as before.

The `setData` regression test was confirmed to have teeth: with the reference
members temporarily restored it fails, reporting the constructor's vector clobbered
from `{1,1,1,1,1,2,2,2,2,2}` to `{3,3,3,3,3,3,3,3,3,3}`. The copy/move assertions
are self-proving — with reference members restored the test file no longer
compiles, since `is_copy_assignable_v<Metrics>` becomes false.

### Phase 4 — Error handling

- **T4.1** Create `src/Exceptions.h` with a `DiscretizerException` base deriving
  from `std::runtime_error`, plus `InvalidParameterException`, `ValidationException`,
  `IndexException`, `NotFittedError`.
- **T4.2** Migrate existing throw sites. **Breaking change** — keep each new type
  derived from the `std::` type it replaces (`std::invalid_argument`,
  `std::out_of_range`) so existing `catch` blocks keep working. The two prior plans
  disagreed on this hierarchy; this is the ruling.
- **T4.3** Include the offending value and parameter name in every message.
- **T4.4** Update the tests that assert on exception type and message text —
  several use `EXPECT_THROW_WITH_MESSAGE` and will need revisiting.

### Phase 5 — Move semantics and buffer reuse ✅ COMPLETE

Unblocked by Phase 3, which made `CPPFImdlp` movable.

- [x] **T5.1** `fit(samples_t&&, labels_t&&)` added as a **virtual** overload on
  `Discretizer`, so moving works through a `Discretizer&` and the uniform-interface
  philosophy is preserved. The base provides a default that forwards to the copying
  overload, so any subclass behaves correctly without overriding. `CPPFImdlp`,
  `BinDisc` and `PKIDisc` override it to actually adopt the buffers.
  `BinDisc::fit_quantile` now takes its data **by value**, so one body serves both
  the copying and moving paths.
- [x] **T5.2** `transform(const samples_t&, labels_t& out) const` added; the
  returning overload now delegates to it. Being `const` is a bonus: it touches no
  internal storage.
- [x] **T5.3** Measured against the Phase 0 baseline. See
  [docs/benchmarks.md](docs/benchmarks.md#phase-5--measured-impact-of-move-semantics).

**Also fixed:** `BinDisc::min_bins` was a `const` non-static member, which deletes
the copy and move *assignment* operators for `BinDisc` and `PKIDisc`. Changed to
`static constexpr`. This was the same class of defect as D4 and had gone unnoticed.

**Result: the null result Phase 0 predicted.** `fit` with rvalues is within noise
of `fit` with lvalues at every size (the copies removed were 0.0001% of the call).
`BinDisc` QUANTILE shows −1.6% at n = 100 000 against a fair pool-copy control,
which is inside the ~5% run-to-run variance of this machine and is therefore not
claimable as a speed-up. `transform` into a caller buffer is likewise flat, because
the returning overload already reuses its capacity via `clear()`.

Phase 5 is justified on **memory and ergonomics**, as Phase 0 said it would have to
be. No throughput claim is made.

**Verification.** 102/102 tests pass. Debug and release builds are warning-free.
All rows sit within the ≤5% regression ceiling.

### Phase 6 — API improvements

- **T6.1** Bound modes. `bound_dir_t` already exists but only `RIGHT` is ever used
  (see D2), so this is new functionality rather than a rename. If the richer
  `BoundMode` enum (`LEFT_CLOSED` / `RIGHT_CLOSED` / `BOTH_CLOSED` / `BOTH_OPEN`)
  is adopted, it replaces `bound_dir_t` — **breaking change** — and each mode needs
  its own tests. Deferring this to 3.1.0 is a legitimate option.
- **T6.2** `src/Config.h` with `DiscretizerConfig` / `BinDiscConfig`, fluent
  setters and validation. Add config-taking constructors alongside the existing
  ones rather than replacing them.
- **T6.3** Static factory / builder helpers (`CPPFImdlp::builder()`,
  `fit_transform`) as sugar over the existing API.

### Phase 7 — Performance

- **T7.1** Profile against the Phase 0 baseline and identify the top three costs
  before optimizing anything.
- **T7.2** Optimize the entropy caching path if profiling justifies it.
- **T7.3** Parallelism is **out of scope for 3.0.0** — it interacts directly with
  the unresolved threading contract from T3.3. Revisit for 3.1.0.

### Phase 8 — Testing, docs, release

- **T8.1** `tests/Security_unittest.cpp` — tensor validation (including the D1
  non-contiguous case), bounds checking, large inputs, deep recursion.
- **T8.2** Restore 100% coverage; verify with `make test`.
- **T8.3** Enable `-Wall -Wextra` and clear the resulting warnings.
- **T8.4** Write `SECURITY.md` and `ARCHITECTURE.md`. Both were specified in the
  2.2.0 plan and neither exists.
- **T8.5** Write `MIGRATION.md` covering every breaking change in §6.
- **T8.6** Bump `CMakeLists.txt:4` to `VERSION 3.0.0` and update `CHANGELOG.md`.
- **T8.7** Tag and release.

---

## 6. Breaking changes

Tracked explicitly so `MIGRATION.md` can be generated from this list.

1. **Exception types change** (T4.2). Mitigated by deriving from the same `std::`
   types, so only code catching by exact type is affected.
2. **Non-contiguous / non-CPU tensors change behaviour** (T2.1). Callers currently
   receiving silently wrong results will now get either an exception or correct
   results. Either way the observable output changes — this is the fix, and it must
   be called out prominently in the release notes.
3. **`Metrics` public surface changes** (T3.1/T3.2). It is a low-level class, but
   it is installed as a public header.
4. **`bound_dir_t` → `BoundMode`**, only if T6.1 lands in 3.0.0.

Removed from the prior plan: *"Removed deprecated methods."* Nothing in the
codebase is marked deprecated, so there is nothing to remove.

---

## 7. Success criteria

Replacing the previous unmeasurable targets.

| Criterion | How it is judged |
|-----------|------------------|
| All tests pass | ≥ 82 tests, 0 failures |
| Coverage | 100%, per `make test` |
| D1 fixed | Non-contiguous input either rejected or handled correctly, with a test |
| D2 fixed | `direction` initialized; default-ctor regression test passes |
| D3/D4 fixed | `setData` rebinds correctly; `CPPFImdlp` is movable |
| No performance regression | ≤ 5% vs. the Phase 0 baseline |
| Performance improvement | Reported as a **measured** delta vs. Phase 0. No target is asserted in advance — the prior "20-30%" figure had no baseline behind it and is withdrawn |
| Warnings | Clean under `-Wall -Wextra` |
| Docs | `SECURITY.md`, `ARCHITECTURE.md`, `MIGRATION.md` present; `CHANGELOG.md` updated |
| Version | `3.0.0` in `CMakeLists.txt`, picked up by `conanfile.py` |

---

## Appendix A — Risks

| Risk | Mitigation |
|------|-----------|
| T2.1 changes results for existing callers | Intended; it replaces silently wrong output. Document loudly in release notes |
| `Metrics` redesign is invasive (Phase 3) | Land it as its own commit with tests before Phase 5 depends on it |
| Exception migration breaks downstream `catch` | Derive from the same `std::` types (T4.2) |
| Coverage drops while refactoring | Run `make test` per phase, not once at the end |
| Scope creep from Phases 6-7 | Both are explicitly deferrable to 3.1.0; Phases 0-5 and 8 are the release |

## Appendix B — Corrections to the superseded 2.2.0 plan

Recorded so `RELEASE_PLAN_2.2.0.md` can be deleted without losing anything.

**Salvaged into this plan:**

- Tensor contiguity validation (its §1.1) — reinstated as D1/T2.1. The prior V3
  plan had dropped it; it turned out to be the most severe defect in the codebase.
- `SECURITY.md` and `ARCHITECTURE.md` (its §5.2, §5.3) — now T8.4.
- Security-focused test file (its §4.1) — now T8.1.
- Exception hierarchy (its §2.1) — now Phase 4, with the compatibility ruling in T4.2.

**Rejected, with reasons:**

1. **Its proposed `Metrics::setData` is broken.** The snippet does
   `y = const_cast<labels_t&>(y_)` on a reference member. That assigns *through*
   the reference — it does not rebind it, which C++ does not permit. Applied
   literally it would keep the existing no-op behaviour while looking like a fix.
   Superseded by T3.1, which changes the member design instead.

2. **Its §1.3 `safe_subtract` change would introduce a regression.** It proposes
   wrapping `m = idxNext - cut - 1` (`CPPFImdlp.cpp:108`) in a throwing
   `safe_subtract`. That underflow is *deliberate*: when `idxNext == cut` the next
   line catches it via `int(idxNext - cut - 1) < 0 ? 0 : m` (`CPPFImdlp.cpp:111`).
   A throwing helper there would reject valid input. If this is cleaned up, it needs
   an explicit `idxNext > cut` guard, not `safe_subtract`.

3. **Its roadmap is corrupted.** "Replace direct vector access with .at() in
   BinDisc.cpp" appears three times as unrelated tasks — under Week 2 Days 1-2
   (line 1156), under "Release Preparation" (line 1198), and under "Release &
   Communication" as "Replace direct vector access with .at() in BinDisc.cpp site"
   (line 1204), where the intent was evidently "update documentation site". The
   schedule cannot be followed as written.

4. **Blanket `.at()` migration (its §1.2) is not adopted.** `CPPFImdlp` already has
   `safe_X_access` / `safe_y_access`, which check the *indirection* through
   `indices` — strictly more than `.at()` does. Converting to `.at()` would be a
   downgrade on the hot path. `.at()` remains reasonable in `BinDisc`, folded into
   T8.3.

5. **Version target.** That document targeted 2.2.0 as a compatible minor release.
   The work here is breaking (§6), so it ships as 3.0.0.

---

*Consolidated 2026-08-04 against verified baseline `e143bbc`.*
