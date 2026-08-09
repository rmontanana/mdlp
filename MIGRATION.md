# Migrating from 2.x to 3.0.0

Most code needs **no changes**. The breaking surface is narrow and is listed first
so you can check it in a minute.

## Do I need to change anything?

| If your code… | Action |
|---|---|
| calls `fit`, `transform`, `fit_transform` | Nothing |
| catches `std::invalid_argument`, `std::out_of_range`, `std::runtime_error`, `std::exception` | Nothing — still works |
| passes contiguous 1-D CPU tensors | Nothing |
| **matches on exception message text** | **Update** — messages changed |
| **passes non-contiguous tensors** | **Results change** — they were wrong before |
| **passes non-CPU tensors** | **Now throws** — it used to read invalid memory |
| **passes NaN or infinite samples** | **Now throws** — it used to produce nonsense |
| subclasses `Discretizer` | Add `using Discretizer::fit;` if you want rvalue support |
| includes `Config.h` | No such header existed in 2.x; the new one is `DiscretizerConfig.h` |

## Breaking changes

### 1. Exception messages changed

Types are backward compatible — every new exception also derives from the `std::`
exception it replaces — but **text changed** where a message can now name the
offending value:

| 2.x | 3.0.0 |
|---|---|
| `min_length must be greater than 2` | `min_length must be at least 3, got 2` |
| `max_depth must be greater than 0` | `max_depth must be at least 1, got 0` |
| `proposed_cuts must be non-negative` | `proposed_cuts must be non-negative, got -1` |
| `wrong proposed num_cuts value` | `proposed_cuts (8) cannot exceed the number of samples (7)` |
| `Input data size must be at least equal to n_bins` | `Input data size (2) must be at least n_bins (3)` |
| `Number of points must be at least 2 for linspace` | `linspace: num must be at least 2, got 1` |
| `Index out of bounds for X array` | `Index 5 out of bounds for X array of size 2` |
| `Index out of bounds for y array` | `Index 5 out of bounds for y array of size 2` |
| `Index out of bounds for indices array` | `Index 2 out of bounds for indices array of size 2` |
| `Subtraction would cause underflow` | `Subtraction would underflow: 3 - 5` |
| `n_bins must be greater than 2` | `n_bins must be at least 3, got 2` |

Tensor validation messages are **unchanged**.

If you match on text, prefer catching the type instead:

```cpp
// 2.x, still valid
catch (const std::invalid_argument& e) { ... }

// 3.0.0, more precise
catch (const mdlp::InvalidParameter& e) { ... }   // bad parameter
catch (const mdlp::ValidationError& e)   { ... }  // bad input data
catch (const mdlp::NotFittedError& e)    { ... }  // transform before fit
catch (const mdlp::DiscretizerError& e)  { ... }  // anything from mdlp
```

A handler typed on `DiscretizerError` uses `e.message()` rather than `e.what()`;
see [ARCHITECTURE.md](ARCHITECTURE.md) for why.

### 2. Non-contiguous tensors now produce different — correct — results

In 2.x, a 1-D non-contiguous tensor passed validation and was then read as raw
memory. Feeding a column view of a 2-D dataset:

```cpp
auto column = dataset.select(1, feature);   // 1-D, non-contiguous
disc.fit_t(column, labels);
```

silently produced cut points derived from interleaved memory rather than from the
column. A `{10,2}` tensor whose first column held `0..9` yielded cut points
spanning `0..104`.

3.0.0 copies such tensors into contiguous storage and reads them correctly. **If
you were passing non-contiguous tensors, your results change — because they were
wrong.** Already-contiguous tensors are unaffected and incur no copy.

### 3. Non-CPU tensors are rejected

They previously had `data_ptr()` dereferenced regardless of device. Move to CPU
before calling:

```cpp
disc.fit_t(X.cpu(), y.cpu());
```

### 4. NaN and infinite samples are rejected

`fit` and `transform` now throw `ValidationError` if any sample is not finite,
naming the index and value:

```
Sample at index 3 is not a finite number: nan
```

In 2.x these were accepted. NaN made the sort undefined behaviour; infinities were
rejected only by `BinDisc` UNIFORM. If your pipeline relied on passing them
through, filter or impute before calling `fit`.

Note that **sentinel values are not caught** — `-999` is a finite number, and the
library cannot tell it from a measurement. See [SECURITY.md](SECURITY.md).

### 5. `Metrics` changed shape

Rarely used directly, but it is an installed header. It now **owns** its data
instead of holding references, is copyable and movable, and has no internal mutex.
`setData()` now genuinely replaces the data rather than overwriting the vectors
passed at construction. The unused `numClasses` member is gone;
`computeNumClasses()` is unchanged.

`Metrics` is **not thread-safe** and never really was — see
[SECURITY.md](SECURITY.md).

### 6. `strategy_t` and `compute_strategy_t` moved headers

They now live in `typesFImdlp.h`. Including `BinDisc.h` or `PKIDisc.h` still brings
them in, so this only matters if you were forward-declaring them.

## New in 3.0.0, all optional

### Named configuration

```cpp
// 2.x, and still valid
CPPFImdlp disc(3, 10, 0.5f);

// 3.0.0
CPPFImdlp disc(MDLPConfig{}.withMinLength(3).withMaxDepth(10).withProposedCuts(0.5f));
```

Setters return a modified copy, so a baseline can be varied per experiment:

```cpp
const auto base = MDLPConfig{}.withMaxDepth(10);
run(base.withMinLength(3));
run(base.withMinLength(7));   // base untouched
```

### One-call discretization

```cpp
auto labels = CPPFImdlp::discretize(X, y);
auto bins   = BinDisc::discretize(X, y, BinDiscConfig{}.withNBins(5));
```

Returns **by value**, unlike the member `fit_transform`, which returns a reference
into the discretizer's storage and therefore dangles on a temporary.

### Move-aware `fit`

```cpp
disc.fit(std::move(X), std::move(y));
```

Avoids copying the inputs when you no longer need them. The benefit is memory, not
speed — measured, see [docs/benchmarks.md](docs/benchmarks.md).

### `transform` into your own buffer

```cpp
labels_t out;
disc.transform(X, out);   // const, reusable across calls
```

## Performance

No action needed; this is what you get for upgrading.

`CPPFImdlp::fit` was quadratic and is not any more:

| n | 2.x | 3.0.0 | |
|---:|---:|---:|---:|
| 1 000 | 0.95 ms | 0.07 ms | 13× |
| 10 000 | 82 ms | 1.05 ms | 78× |
| 100 000 | 8 517 ms | 16.5 ms | **515×** |

On a real 130 000-row, 50-feature dataset, discretizing every feature went from
about 12.5 minutes to 1.08 seconds. Cut points are **bit-identical** to 2.x —
verified across 180 synthetic configurations and every feature of two real
datasets.
