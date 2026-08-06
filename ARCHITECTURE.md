# Architecture

How the library is put together and, more usefully, **why** — including the places
where the obvious design was tried and rejected.

## Class hierarchy

```
Discretizer  (abstract)
├── CPPFImdlp        supervised, Fayyad & Irani MDLP   ── owns a Metrics
├── BinDisc          unsupervised, k-bins
└── PKIDisc          unsupervised, derives k from n    ── inherits BinDisc
```

| Header | Holds |
|---|---|
| `Discretizer.h` | Base class, tensor entry points, `transform` |
| `CPPFImdlp.h` | MDLP algorithm |
| `BinDisc.h` | Uniform and quantile binning |
| `PKIDisc.h` | Bin-count selection, delegates to `BinDisc` |
| `Metrics.h` | Entropy and information gain, memoized |
| `Exceptions.h` | Exception hierarchy |
| `DiscretizerConfig.h` | `MDLPConfig`, `BinDiscConfig`, `MIN_BINS` |
| `typesFImdlp.h` | Type aliases and strategy enums |

## The uniform `fit(X, y)` interface

Every discretizer takes `fit(X, y)`, including the unsupervised ones, which accept
`y` and ignore it. This is deliberate and is the library's central design
commitment: an experimentation platform can drive all of them through one code
path.

```cpp
void run_experiment(Discretizer& disc, samples_t& X, labels_t& y) {
    disc.fit(X, y);
    auto result = disc.transform(X);
}
```

The cost is an unused parameter, marked `[[maybe_unused]]` in `BinDisc` so the
compiler agrees it is deliberate.

## Design decisions

### `Metrics` owns its data

It holds copies of the labels and the index order, not references.

References were the original design and could not work: `setData()` assigned
*through* them, which in C++ overwrites the referenced vectors rather than
rebinding, so the method could not do what its name said. A pointer member would
rebind, but `Metrics` is a member of `CPPFImdlp` and would point at its *sibling*
members — every copy or move of the owner would then need custom constructors to
re-point it, or dangle.

Owning keeps the rule of zero and makes `CPPFImdlp` copyable and movable. The cost
is that the labels and index order exist twice during a fit. Making `Metrics` the
single owner would remove the duplication and is a candidate for a later release.

### `Metrics` has no mutex

It previously held one around the memo caches. That advertised thread safety it did
not provide — the data members were unguarded, so a concurrent `setData()` still
raced — while adding lock traffic to every lookup in what is a single-threaded
recursion. The lock is gone and the contract is stated: **not thread-safe, use one
instance per thread.**

### Exceptions add a handler without removing one

`DiscretizerError` is a tag that derives from **nothing**. Each concrete exception
derives from both it and the `std::` exception it replaces, so 2.x `catch` blocks
keep working while new code can catch everything from the library in one handler.

The plan originally called for a base deriving from `std::runtime_error`, which is
impossible: `std::invalid_argument` and `std::out_of_range` derive from
`std::logic_error`, so one base cannot be both. Had the tag derived from
`std::exception`, that base would be inherited twice and become ambiguous — which
is why a tag-typed handler uses `message()` rather than `what()`.

### Configuration is the builder

`MDLPConfig` and `BinDiscConfig` have setters that return a modified copy, so a
baseline can be varied per experiment without aliasing. No separate `builder()`
exists, because the config already is one.

The positional constructors delegate to the config ones, so validation has a single
implementation and a test asserts both produce the same message.

### `discretize()` returns by value

`fit_transform` returns a reference into the discretizer's own storage, so calling
it on a temporary leaves a dangling reference. The static `discretize()` helpers
return a value and internally use the move and caller-buffer overloads.

### `bound_dir_t` is a knob that cannot be turned

`transform()` selects between `lower_bound` and `upper_bound` on `direction`, but
nothing ever assigns `LEFT`, so the `lower_bound` branch is unreachable. This ships
as-is in 3.0.0 **by decision, not oversight** — building a proper `BoundMode` means
deciding the semantics of each mode for each discretizer and testing them, which is
a feature rather than a cleanup. Deferred to 3.1.0.

## Complexity, as measured

Not as assumed. Superseded documentation claimed `CPPFImdlp::fit` was
"O(n log n) for sorting + O(n) per recursion level"; it was **O(n²)** until 3.0.0.

| Operation | Behaviour |
|---|---|
| `CPPFImdlp::fit` | ~n log n. Log-log exponent **1.17-1.23**, measured on three platforms |
| `BinDisc::fit` (uniform) | Linear — one min/max pass |
| `BinDisc::fit` (quantile) | n log n — one sort |
| `PKIDisc::fit` | As quantile; it selects a bin count and delegates |
| `transform` | Linear in samples, log in cut points |

`fit` used to call `Metrics::entropy` for both sides of every candidate split. Each
call is a distinct cache key, so every one missed and rescanned its whole interval:
O(n) work at O(n) boundaries. Carrying per-class counts across the scan replaced
that with O(n·k) for k classes, which measured **515× faster at n = 100 000** —
8.5 seconds down to 16.5 milliseconds — and 600-770× per feature on a real
130 000-row dataset.

Since then `fit` is dominated by `std::stable_sort` in `sortIndices`, about **37%**
of its cost. Numbers and methodology in [docs/benchmarks.md](docs/benchmarks.md).

## Memory

`CPPFImdlp::fit` copies `X` and `y` into the object, and `Metrics` copies `y` and
the index order again. The rvalue `fit` overloads let a caller hand over its buffers
instead of having them copied; the measured speed benefit is nil — the copies are
~0.0001% of a fit — so they exist for memory and ergonomics.

`transform` reuses its output buffer's capacity across calls, and the two-argument
overload writes into a buffer the caller owns.

## Error handling

```
std::exception
├── std::logic_error
│   ├── std::invalid_argument ── InvalidParameter, ValidationError
│   └── std::out_of_range     ── IndexError
└── std::runtime_error
    ├── (direct)              ── NotFittedError
    └── std::underflow_error  ── UnderflowError

DiscretizerError  (tag, no base) ── all five
```

Messages name the parameter and the offending value where a value exists.

## Testing

| File | Covers |
|---|---|
| `Metrics_unittest` | Entropy, information gain, data rebinding, copy/move |
| `FImdlp_unittest` | MDLP algorithm, sorting, bounds, move semantics |
| `BinDisc_unittest` | Both binning strategies, move semantics |
| `Discretizer_unittest` | Tensor entry points, transform, base-class behaviour |
| `PKIDisc_unittest` | Bin-count selection |
| `Exceptions_unittest` | Hierarchy contract and backward compatibility |
| `Config_unittest` | Configs, validation sharing, `discretize()` |
| `Security_unittest` | Recursion depth, scale, degenerate inputs |
| `RealDatasets_unittest` | Full real datasets end to end |

100% line and function coverage of `src/`, enforced by `make test`.

## Build

| Command | Does |
|---|---|
| `make debug` | Debug build with tests and coverage |
| `make release` | Release build, `-O3`, `-Wall -Wextra` warning-free |
| `make test` | Debug build, run tests, coverage report, update badge |
| `make bench` | Release benchmark, stores a fingerprinted result |
| `make bench-report` | Cross-platform benchmark comparison |
| `make sortbench` | Standalone toolchain diagnostic |
| `make sortbench-report` | Cross-machine toolchain comparison |

Dependencies come from conan: libtorch 2.7.1, GoogleTest, arff-files.

**libtorch is a hard dependency of every header**, because `Discretizer.h` includes
`torch/torch.h`. Nothing else in the benchmark path uses tensors, so this couples
more than it needs to; decoupling it would let the library be built against a
different standard library, which is currently impossible because libtorch arrives
prebuilt against libstdc++.
