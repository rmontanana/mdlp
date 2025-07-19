# Conan Package for fimdlp

This directory contains the Conan package configuration for the fimdlp library.

## Dependencies

The package manages the following dependencies:

### Build Requirements

- **libtorch/2.4.1** - PyTorch C++ library for tensor operations

### Test Requirements (when testing enabled)

- **catch2/3.8.1** - Modern C++ testing framework
- **arff-files** - ARFF file format support (included locally in tests/lib/Files/)

## Building with Conan

### 1. Install Dependencies and Build

```bash
# Install dependencies
conan install . --output-folder=build --build=missing

# Build the project
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### 2. Using the Build Script

```bash
# Build release version
./scripts/build_conan.sh

# Build with tests
./scripts/build_conan.sh --test
```

## Creating a Package

### 1. Create Package Locally

```bash
conan create . --profile:build=default --profile:host=default
```

### 2. Create Package with Options

```bash
# Create with testing enabled
conan create . -o enable_testing=True --profile:build=default --profile:host=default

# Create shared library version
conan create . -o shared=True --profile:build=default --profile:host=default
```

### 3. Using the Package Creation Script

```bash
./scripts/create_package.sh
```

## Uploading to Cimmeria

### 1. Configure Remote

```bash
# Add Cimmeria remote
conan remote add cimmeria https://conan.rmontanana.es/artifactory/api/conan/Cimmeria

# Login to Cimmeria
conan remote login cimmeria <username>
```

### 2. Upload Package

```bash
# Upload the package
conan upload fimdlp/2.1.0 --remote=cimmeria --all

# Or use the script (will configure remote instructions if not set up)
./scripts/create_package.sh
```

## Using the Package

### In conanfile.txt

```ini
[requires]
fimdlp/2.1.0

[generators]
CMakeDeps
CMakeToolchain
```

### In conanfile.py

```python
def requirements(self):
    self.requires("fimdlp/2.1.0")
```

### In CMakeLists.txt

```cmake
find_package(fimdlp REQUIRED)
target_link_libraries(your_target fimdlp::fimdlp)
```

## Package Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| shared | True/False | False | Build shared library |
| fPIC | True/False | True | Position independent code |
| enable_testing | True/False | False | Enable test suite |
| enable_sample | True/False | False | Build sample program |

## Example Usage

```cpp
#include <fimdlp/CPPFImdlp.h>
#include <fimdlp/Metrics.h>

int main() {
    // Create MDLP discretizer
    CPPFImdlp discretizer;
    
    // Calculate entropy
    Metrics metrics;
    std::vector<int> labels = {0, 1, 0, 1, 1};
    double entropy = metrics.entropy(labels);
    
    return 0;
}
```

## Testing

The package includes comprehensive tests that can be enabled with:

```bash
conan create . -o enable_testing=True
```

## Requirements

- C++17 compatible compiler
- CMake 3.20 or later
- Conan 2.0 or later
