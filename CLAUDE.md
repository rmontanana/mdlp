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
2. **CPPFImdlp** (`src/CPPFImdlp.h/cpp`) - Main MDLP algorithm implementation
3. **BinDisc** (`src/BinDisc.h/cpp`) - K-bins discretization (quantile/uniform strategies)
4. **Metrics** (`src/Metrics.h/cpp`) - Entropy and information gain calculations

### Key Data Types
- `samples_t` - Input data samples
- `labels_t` - Classification labels
- `indices_t` - Index arrays for sorting/processing
- `precision_t` - Floating-point precision type

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

## Development Notes

- The library uses PyTorch tensors for efficient numerical operations
- Code follows C++17 standards
- Coverage is maintained at 100%
- The implementation handles edge cases like duplicate values and small intervals
- Conan package manager support is available via `conanfile.py`