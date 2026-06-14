# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a C++ implementation of the MDLP (Minimum Description Length Principle) discretization algorithm based on Fayyad & Irani's paper. The library provides discretization methods for continuous-valued attributes in classification learning.

## Build System

The project uses CMake with a Makefile wrapper for common tasks:

### Common Commands
- `make build` - Build release version with sample program
- `make test` - Run full test suite with coverage report
- `make install` - Install the library

### Build Configurations
- **Release**: Built in `build_release/` directory
- **Debug**: Built in `build_debug/` directory (for testing)

### Dependencies
- PyTorch (libtorch) - Required dependency
- GoogleTest - Fetched automatically for testing
- Coverage tools: lcov, genhtml

## Code Architecture

### Core Components

1. **Discretizer** (`src/Discretizer.h/cpp`) - Abstract base class for all discretizers
2. **CPPFImdlp** (`src/CPPFImdlp.h/cpp`) - Main MDLP algorithm implementation (supervised)
3. **BinDisc** (`src/BinDisc.h/cpp`) - K-bins discretization (unsupervised, quantile/uniform strategies)
4. **PKIDisc** (`src/PKIDisc.h/cpp`) - Proportional k-Interval Discretization (unsupervised)
5. **Metrics** (`src/Metrics.h/cpp`) - Entropy and information gain calculations

### Key Data Types
- `samples_t` - Input data samples (std::vector<precision_t>)
- `labels_t` - Classification labels (std::vector<label_t>)
- `indices_t` - Index arrays for sorting/processing (std::vector<size_t>)
- `precision_t` - Floating-point precision type (float)
- `cutPoints_t` - Cut point values (std::vector<precision_t>)

### Interface Design Philosophy

**All discretizers share a common `fit(X, y)` signature** for interface consistency:

```cpp
// Supervised: uses y for label information
CPPFImdlp disc1;
disc1.fit(X, y);

// Unsupervised: accepts y but doesn't use it (for interface consistency)
BinDisc disc2(3, strategy_t::QUANTILE);
disc2.fit(X, y);  // y is ignored
```

This enables a single code path in experimentation platforms:

```cpp
void run_experiment(Discretizer& disc, samples_t& X, labels_t& y) {
    disc.fit(X, y);  // Works for ALL discretizer types
    auto result = disc.transform(X);
}
```

### Algorithm Flow

1. Data is sorted using labels as tie-breakers for identical values
2. MDLP recursively finds optimal cut points using entropy-based criteria
3. Cut points are validated to ensure meaningful splits
4. Transform method maps continuous values to discrete bins

## Testing

Tests are built with GoogleTest and include:
- `Metrics_unittest` - Entropy/information gain tests
- `FImdlp_unittest` - Core MDLP algorithm tests
- `BinDisc_unittest` - K-bins discretization tests
- `Discretizer_unittest` - Base class functionality tests

### Running Tests
```bash
make test  # Runs all tests and generates coverage report
cd build_debug/tests && ctest  # Run tests directly
```

Coverage reports are generated at `build_debug/tests/coverage/index.html`.

## Sample Usage

The sample program demonstrates basic usage:
```bash
build_release/sample/sample -f iris -m 2
```

### Python-like Pseudocode for Common Operations

```python
# Create and fit MDLP discretizer
from fimdlp import CPPFImdlp
disc = CPPFImdlp(min_length=3, max_depth=10)
disc.fit(X_train, y_train)
result = disc.transform(X_test)
cut_points = disc.getCutPoints()

# Create and fit Binning discretizer
from fimdlp import BinDisc, strategy_t
disc = BinDisc(n_bins=5, strategy=strategy_t.QUANTILE)
disc.fit(X_train, y_train)  # y_train is ignored but required for interface
result = disc.transform(X_test)

# PyTorch tensor support
disc.fit_t(X_tensor, y_tensor)
result = disc.transform_t(X_tensor)
```

## Development Notes

- The library uses PyTorch tensors for efficient numerical operations
- Code follows C++17 standards
- Coverage is maintained at 100%
- The implementation handles edge cases like duplicate values and small intervals
- Conan package manager support is available via `conanfile.py`
- All discretizers share a common `fit(X, y)` interface for experimentation platform compatibility
- Unsupervised methods accept the y parameter but do not use it in their algorithms
