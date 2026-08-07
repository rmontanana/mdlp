# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.0] - 2026-08-07

### Added

- **Move-aware `fit()`**: `fit(samples_t&&, labels_t&&)` on every discretizer, so a
  caller with no further use for its data can hand over the buffers instead of
  having them copied. It is virtual, so it works through a `Discretizer&` like the
  rest of the interface, and the base provides a forwarding default. `BinDisc` also
  gains an rvalue form of its samples-only `fit()`.
  The benefit is **memory, not speed** — measured, see `docs/benchmarks.md`.
- **`transform(const samples_t&, labels_t& out) const`**: writes into a buffer the
  caller owns, avoiding the copy otherwise needed to keep a result past the next
  `transform()` call. The existing returning overload is unchanged.
- `bench/` benchmark harness and `make bench`, behind `ENABLE_BENCHMARK`
  (default OFF, Release-only). Baseline results in `docs/benchmarks.md`.

### Added — configuration and convenience API

- **`MDLPConfig` and `BinDiscConfig`** in `src/DiscretizerConfig.h`: named,
  chainable parameters, so three positional arguments stop being a guessing game.

  ```cpp
  CPPFImdlp disc(MDLPConfig{}.withMinLength(5).withMaxDepth(10));
  BinDisc bins(BinDiscConfig{}.withNBins(5).withStrategy(strategy_t::QUANTILE));
  ```

  Setters return a modified copy, so a shared baseline can be varied per
  experiment without one variation leaking into the next. `validate()` checks a
  config before it reaches a constructor; the positional constructors delegate to
  the config ones, so validation and its messages have a single implementation.

- **Static `discretize()`** on `CPPFImdlp`, `BinDisc` and `PKIDisc`, returning the
  labels **by value**:

  ```cpp
  auto labels = CPPFImdlp::discretize(X, y);
  ```

  Safer than the member `fit_transform`, which returns a reference into the
  discretizer's storage and therefore dangles when called on a temporary.

- `strategy_t` and `compute_strategy_t` moved to `typesFImdlp.h` so the config
  header can name them. Including `BinDisc.h` or `PKIDisc.h` still brings them in.

### Documentation

- `SECURITY.md`, `ARCHITECTURE.md` and `MIGRATION.md` added.
- `CONAN_README.md` rewritten. It documented `libtorch/2.4.1` and `catch2` as
  dependencies — the project uses libtorch 2.7.1 and GoogleTest — pointed at a
  `tests/lib/Files/` directory that no longer exists, quoted `conan upload --all`
  (Conan 1.x syntax), and carried an example that would not compile.
- `scripts/create_package.sh` had the version hardcoded to `2.1.0` and had gone
  stale across two releases; it now reads it from `CMakeLists.txt`, the same source
  `conanfile.py` uses.
- **The published conan package did not compile for consumers.** `Discretizer.h`
  includes `<torch/torch.h>`, but libtorch was linked `PRIVATE` and required
  without `transitive_headers`, so anyone consuming `fimdlp/2.1.x` got headers
  they could not build. Both are fixed, and `test_package` — which had never
  compiled, and was being skipped with `-tf ""` — now exercises the real public
  API and runs as part of `make conan-create`.
- `scripts/create_package.sh` **removed**, folded into `make conan-create` and the
  new `make conan-upload`. The two paths disagreed on whether to run
  `test_package` and produced different package sets.
- `TECHNICAL_ANALYSIS_REPORT.md` **removed**. It was the origin of this release and
  most of its findings were delivered here, but it had become actively misleading —
  it declared unfixed HIGH-risk memory-safety defects that 3.0.0 fixed. Its content
  now lives in `ARCHITECTURE.md`, `SECURITY.md` and `docs/benchmarks.md`; what it
  flagged that remains open is recorded in Appendix C of `RELEASE_PLAN_V3.md`.

### Changed — exceptions (breaking)

- **New exception hierarchy in `src/Exceptions.h`.** `InvalidParameter`,
  `ValidationError`, `NotFittedError`, `IndexError` and `UnderflowError`, all
  tagged with `DiscretizerError` so one handler can catch anything the library
  throws:

  ```cpp
  try { disc.fit(X, y); }
  catch (const mdlp::DiscretizerError& e) { log(e.message()); }
  ```

  **Existing `catch` blocks keep working.** Each type also derives from the `std::`
  exception it replaces, so `catch (const std::invalid_argument&)`,
  `catch (const std::out_of_range&)` and `catch (const std::exception&)` behave
  exactly as they did in 2.x. `DiscretizerError` itself deliberately does not
  derive from `std::exception` — that is what keeps the `std::` base unambiguous —
  so a tag-typed handler uses `message()` rather than `what()`.

- **Error messages now name the parameter and its value** where a value exists:
  `"min_length must be at least 3, got 2"`,
  `"Index 5 out of bounds for X array of size 2"`,
  `"proposed_cuts (8) cannot exceed the number of samples (7)"`. The previous
  `"wrong proposed num_cuts value"` said nothing at all. **This is the breaking
  part**: code matching on exact message text needs updating. Tensor validation
  messages are unchanged.

### Performance

- **`CPPFImdlp::fit` is no longer quadratic.** `getCandidate()` called
  `Metrics::entropy` for both sides of every candidate split, and each call was a
  distinct cache key that rescanned its whole interval — O(n) work at O(n)
  boundaries. It now carries per-class counts incrementally as the scan advances,
  which is O(n·k) for k classes.

  Measured on an Apple M4 Max: **13× faster at n = 1 000, 78× at n = 10 000 and
  515× at n = 100 000**, where discretizing one feature fell from 8.5 seconds to
  16.5 milliseconds. Confirmed quadratic beforehand on three platforms spanning two
  instruction sets and three compilers.

  Cut points are unchanged — bit-identical, not approximately equal.
  `Metrics::entropyFromCounts` is now the single implementation shared by the batch
  and incremental paths.

### Fixed

- **NaN and infinite samples are now rejected instead of silently processed.**
  `CPPFImdlp` sorts with `std::stable_sort` and `BinDisc`'s QUANTILE strategy with
  `std::sort`; both require a strict weak ordering, which NaN comparisons do not
  provide, so passing NaN was undefined behaviour rather than merely a wrong
  answer. Infinities were rejected only by `BinDisc` UNIFORM and accepted
  everywhere else. `fit` and `transform` now throw `ValidationError` naming the
  index and value of the first offending sample, in every discretizer and through
  the tensor entry points.
- **`safe_X_access` and `safe_y_access` had become too large to inline**, after
  their error messages grew to include the offending index. They are called once
  per element in the MDLP scan, and the bloat cost 12% on `CPPFImdlp::fit`. The
  throw paths are now out of line.
- **Tensor discretization silently produced wrong results for non-contiguous input.**
  `fit_t`, `transform_t` and `fit_transform_t` validated rank, dtype and element
  count but not contiguity, then read `data_ptr()` directly. A 1-D non-contiguous
  view — such as a column of a 2-D dataset, `dataset.select(1, col)` — was read as
  raw interleaved memory, yielding wrong cut points with no error raised. Such
  tensors are now copied into contiguous storage and read correctly. Tensors that
  are already contiguous are unaffected and incur no copy.
- Tensors that do not reside on the CPU are now rejected with a clear error instead
  of having their `data_ptr()` dereferenced.
- `Discretizer::direction` was never initialized. A default-constructed `CPPFImdlp`
  left it indeterminate, so `transform()` branched on an uninitialized value
  (undefined behaviour). It now defaults to `bound_dir_t::RIGHT`.
- `Metrics::entropy` computed its size guard as `end - start < 2` in unsigned
  arithmetic, so an inverted interval wrapped around and slipped past the check.
- Entropy and information-gain cache keys were typed `int` while all callers pass
  `size_t`, silently truncating interval indices. Retyped to `size_t`.
- Fixed a signed/unsigned comparison in `BinDisc::linspace`.
- `BinDisc::min_bins` was a `const` non-static member, which deletes the copy and
  move assignment operators. `BinDisc` and `PKIDisc` were therefore not assignable.
  Changed to `static constexpr`.

- `Metrics::setData()` could not replace the data it worked on. `y` and `indices`
  were reference members, and C++ cannot rebind a reference, so `setData()`
  assigned *through* them: it overwrote the vectors passed at construction instead
  of switching to the new ones. `Metrics` now owns its data and the caller's
  vectors are left untouched.

### Changed

- **`Metrics` is no longer internally synchronized.** The mutex it held guarded
  only the caches and left the data members unguarded, so it advertised a
  thread-safety guarantee it did not deliver while adding lock traffic to every
  lookup in what is a single-threaded recursion. `Metrics` is now documented as
  requiring external synchronization; distinct instances remain independent.
- **`Metrics` and `CPPFImdlp` are now copyable and movable.** The `std::mutex`
  member had implicitly deleted both classes' copy and move constructors.
- Removed the unused `Metrics::numClasses` member. It was written on every
  `setData()` and never read, costing a full pass over the data each time. The
  `computeNumClasses()` method is unchanged.
- Tensor validation in `Discretizer` consolidated into a single helper. All
  existing exception messages are unchanged.

## [2.1.3] - 2025-11-06

### Added

- PKIDisc: Implementation of the Proportional k-Interval (PKI) discretization algorithm based on Ying Yang & Geoffrey I. Webb's method (ECML 2001).
- PKIDisc: Added the compute strategy LOG and SQRT for determining the number of bins.
- PKIDisc: Updated unit tests to cover new functionality and edge cases.

## [2.1.2] - 2025-08-22

### Added

- make info now gives info about the build status

### Fix

- Mistake in entropy computation

## [2.1.1] - 2025-07-17

### Internal Changes

- Updated Libtorch to version 2.7.1
- Updated ArffFiles library to version 1.2.1
- Enhance CMake configuration for better compatibility

## [2.1.0] - 2025-06-28

### Added

- Conan dependency manager support
- Technical analysis report

### Changed

- Updated README.md
- Refactored library version and installation system
- Updated config variable names

### Fixed

- Removed unneeded semicolon

## [2.0.1] - 2024-07-22

### Added

- CMake install target and make install command
- Flag to control sample building in Makefile

### Changed

- Library name changed to `fimdlp`
- Updated version numbers across test files

### Fixed

- Version number consistency in tests

## [2.0.0] - 2024-07-04

### Added

- Makefile with build & test actions for easier development
- PyTorch (libtorch) integration for tensor operations

### Changed

- Major refactoring of build system
- Updated build workflows and CI configuration

### Fixed

- BinDisc quantile calculation errors (#9)
- Error in percentile method calculation
- Integer type issues in calculations
- Multiple GitHub Actions configuration fixes

## [1.2.1] - 2024-06-08

### Added

- PyTorch tensor methods for discretization
- Improved library build system

### Changed

- Refactored sample build process

### Fixed

- Library creation and linking issues
- Multiple GitHub Actions workflow fixes

## [1.2.0] - 2024-06-05

### Added

- **Discretizer** - Abstract base class for all discretization algorithms (#8)
- **BinDisc** - K-bins discretization with quantile and uniform strategies (#7)
- Transform method to discretize values using existing cut points
- Support for multiple datasets in sample program
- Docker development container configuration

### Changed

- Refactored system types throughout the library
- Improved sample program with better dataset handling
- Enhanced build system with debug options

### Fixed

- Transform method initialization issues
- ARFF file attribute name extraction
- Sample program library binary separation

## [1.1.3] - 2024-06-05

### Added

- `max_cutpoints` hyperparameter for controlling algorithm complexity
- `max_depth` and `min_length` as configurable hyperparameters
- Enhanced sample program with hyperparameter support
- Additional datasets for testing

### Changed

- Improved constructor design and parameter handling
- Enhanced test coverage and reporting
- Refactored build system configuration

### Fixed

- Depth initialization in fit method
- Code quality improvements and smell fixes
- Exception handling in value cut point calculations

## [1.1.2] - 2023-04-01

### Added

- Comprehensive test suite with GitHub Actions CI
- SonarCloud integration for code quality analysis
- Enhanced build system with automated testing

### Changed

- Improved GitHub Actions workflow configuration
- Updated project structure for better maintainability

### Fixed

- Build system configuration issues
- Test execution and coverage reporting

## [1.1.1] - 2023-02-22

### Added

- Limits header for proper compilation
- Enhanced build system support

### Changed

- Updated version numbering system
- Improved SonarCloud configuration

### Fixed

- ValueCutPoint exception handling (removed unnecessary exception)
- Build system compatibility issues
- GitHub Actions token configuration

## [1.1.0] - 2023-02-21

### Added

- Classic algorithm implementation for performance comparison
- Enhanced ValueCutPoint logic with same_values detection
- Glass dataset support in sample program
- Debug configuration for development

### Changed

- Refactored ValueCutPoint algorithm for better accuracy
- Improved candidate selection logic
- Enhanced sample program with multiple datasets

### Fixed

- Sign error in valueCutPoint calculation
- Final cut value computation
- Duplicate dataset handling in sample

## [1.0.0.0] - 2022-12-21

### Added

- Initial release of MDLP (Minimum Description Length Principle) discretization library
- Core CPPFImdlp algorithm implementation based on Fayyad & Irani's paper
- Entropy and information gain calculation methods
- Sample program demonstrating library usage
- CMake build system
- Basic test suite
- ARFF file format support for datasets

### Features

- Recursive discretization using entropy-based criteria
- Stable sorting with tie-breaking for identical values
- Configurable algorithm parameters
- Cross-platform C++ implementation

---

## Release Notes

### Version 2.x

- **Breaking Changes**: Library renamed to `fimdlp`
- **Major Enhancement**: PyTorch integration for improved performance
- **New Features**: Comprehensive discretization framework with multiple algorithms

### Version 1.x

- **Core Algorithm**: MDLP discretization implementation
- **Extensibility**: Hyperparameter support and algorithm variants
- **Quality**: Comprehensive testing and CI/CD pipeline

### Version 1.0.x

- **Foundation**: Initial stable implementation
- **Algorithm**: Core MDLP discretization functionality
