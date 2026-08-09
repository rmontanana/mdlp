# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 3.0.x   | ✅ |
| 2.1.x   | ❌ |
| < 2.1   | ❌ |

## Reporting a vulnerability

Do not open a public issue. Email the maintainer at <rmontanana@gmail.com> with a
description, steps to reproduce, and the impact you believe it has.

## What the library validates

These are enforced, with tests:

**Tensor inputs** (`fit_t`, `transform_t`, `fit_transform_t`)

- Rank must be 1.
- Element type must be `Float32` for samples, `Int32` for labels.
- The tensor must reside on the CPU. A non-CPU tensor is rejected rather than
  having its `data_ptr()` dereferenced.
- Sizes must match between samples and labels, and neither may be empty.
- **Non-contiguous tensors are copied into contiguous storage, not read raw.**
  Before 3.0.0 a 1-D non-contiguous view — a column of a 2-D dataset, the most
  natural way to pass one feature — passed every check and was then read as raw
  interleaved memory, producing silently wrong cut points with no error. This was
  the most serious defect fixed in this release.

**Vector inputs**

- Samples and labels must be the same size and non-empty.
- **Every sample must be finite.** NaN and ±infinity are rejected by `fit` and by
  `transform`, in every discretizer and through the tensor entry points, with a
  message naming the index and the value of the first offender. NaN has no strict
  weak ordering, so sorting it would be undefined behaviour rather than merely a
  wrong answer; infinities poison the interval arithmetic that derives cut points.
- `BinDisc` requires at least as many samples as bins.
- Every access into the sample and label arrays goes through bounds-checked
  helpers that throw `IndexError` rather than reading out of range.
- Unsigned subtraction that would wrap throws `UnderflowError`.

**Parameters**

- `min_length >= 3`, `max_depth >= 1`, `proposed_cuts >= 0`, `n_bins >= 3`, each
  rejected with the offending value named in the message.

## Known limitations

Stated plainly rather than implied. None of these is a remote-exploitation
concern — this is a numeric library with no I/O, no network and no deserialization
— but they matter if you feed it data you do not control.

### 1. Coded missing values are still your problem

Non-finite samples are now rejected, but a dataset that encodes "missing" as a
sentinel *number* — `-999` is common in physics data — passes validation, because
`-999` is a perfectly finite float.

MDLP will then spend a cut point separating that block, giving you a bin that means
"missing" in every feature at once. In one 130 000-row dataset, 468 rows carried
`-999` across all 50 columns simultaneously, and all 50 features had their minimum
there, while the dataset's own metadata declared 0% missing.

**Mitigation:** filter or impute sentinel values before discretizing. The library
cannot tell a sentinel from a legitimate measurement.

### 2. Recursion depth is unbounded by default

`CPPFImdlp::computeCutPoints` recurses once per accepted split, so stack depth
grows with the number of cut points found — which grows with the sample count.
`max_depth` defaults to `INT_MAX`, so nothing bounds it.

Measured on structured data (blocks of 20 samples, 7 classes):

| n | recursion depth |
|---:|---:|
| 100 000 | 1 667 |
| 400 000 | **6 671** |

Depth scales roughly linearly with n on this shape. At 8 MB of stack this is
comfortable at 400 000 samples and would not be at several million.

The MDLP stopping criterion limits this on its own for uninformative data — rapidly
alternating labels produce no splits at all — so it takes *structured* data at
scale to reach these depths.

**Mitigation:** set `max_depth` when processing input you do not control:

```cpp
CPPFImdlp disc(MDLPConfig{}.withMaxDepth(64));
```

A test asserts that `max_depth` genuinely bounds the recursion.

### 3. `Metrics` is not thread-safe

Deliberately. `entropy()` and `informationGain()` memoize, so calls that read like
queries mutate state. Distinct instances share nothing and may be used
concurrently; sharing one instance requires external synchronization.

Earlier versions held a mutex around the caches, which advertised a guarantee the
class did not deliver — the data members were unguarded — while costing lock
traffic on every lookup. It was removed and the contract documented instead.

## Security-relevant changes in 3.0.0

- **NaN and infinity are rejected** at every entry point. They were previously
  accepted; NaN in particular made the sort undefined behaviour.
- Non-contiguous tensors no longer read raw memory (see above).
- Non-CPU tensors are rejected.
- `Discretizer::direction` was read uninitialized by a default-constructed
  `CPPFImdlp`, which is undefined behaviour. It now has an initializer.
- Entropy and information-gain cache keys were `int` while all callers passed
  `size_t`, silently truncating indices; past `INT_MAX` elements distinct intervals
  could have collided onto one cache entry and returned a wrong value.
- `Metrics::entropy` computed a size guard in unsigned arithmetic, so an inverted
  interval wrapped past the check.

## Verification practices

- 100% line and function coverage of `src/`, enforced by `make test`.
- The library builds warning-free under `-Wall -Wextra`.
- `tests/Security_unittest.cpp` covers recursion depth, scale, degenerate
  distributions and extreme magnitudes; tensor validation and index bounds are
  covered in `tests/Discretizer_unittest.cpp` and `tests/FImdlp_unittest.cpp`.
- Full real datasets are discretized end to end in
  `tests/RealDatasets_unittest.cpp`.

Not yet in place: sanitizer builds (ASan, UBSan, TSan) and fuzzing. Both would be
worthwhile and neither has been run.
