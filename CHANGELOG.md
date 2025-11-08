# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
