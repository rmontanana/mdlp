# Conan packaging for fimdlp

How the library is packaged and consumed with Conan 2.

## Dependencies

Declared in `conanfile.py`:

| Package | Role |
|---|---|
| `libtorch/2.7.1` | Tensor types used by the `*_t` entry points. **Required** — every public header pulls in `torch/torch.h` |
| `arff-files/1.2.1` | ARFF loading, used by the tests and the sample |
| `gtest/1.16.0` | Test framework, only when `enable_testing=True` |

## Building

The Makefile targets are the normal route and handle conan for you:

```bash
make debug      # Debug + tests + coverage
make release    # Release, -O3
```

To drive conan directly:

```bash
conan install . --output-folder=build_conan --build=missing
cd build_conan
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Or use the helper:

```bash
./scripts/build_conan.sh
```

## Creating a package

```bash
conan create . --build=missing -tf "" -s:a build_type=Release
```

`make conan-create` builds both Release and Debug packages. The helper script
`./scripts/create_package.sh` also runs `test_package` and uploads if the remote is
configured.

The version is **not** written in the recipe. `conanfile.py::set_version` reads it
by regex from the `project(fimdlp VERSION ...)` line in `CMakeLists.txt`, so
bumping `CMakeLists.txt` is the only edit a release needs.

## Publishing to Cimmeria

```bash
conan remote add cimmeria https://conan.rmontanana.es/artifactory/api/conan/Cimmeria
conan remote login cimmeria <username>
conan upload fimdlp/3.0.0 --remote=cimmeria
```

## Consuming the package

`conanfile.txt`:

```ini
[requires]
fimdlp/3.0.0

[generators]
CMakeDeps
CMakeToolchain
```

`conanfile.py`:

```python
def requirements(self):
    self.requires("fimdlp/3.0.0")
```

`CMakeLists.txt`:

```cmake
find_package(fimdlp REQUIRED)
target_link_libraries(your_target fimdlp::fimdlp)
```

## Package options

| Option | Values | Default | Description |
|---|---|---|---|
| `shared` | True/False | False | Build a shared library |
| `fPIC` | True/False | True | Position independent code (removed on Windows) |
| `enable_testing` | True/False | False | Build and run the test suite |
| `enable_sample` | True/False | False | Build the sample program |

## Example

```cpp
#include <fimdlp/CPPFImdlp.h>
#include <fimdlp/DiscretizerConfig.h>

#include <iostream>
#include <vector>

int main()
{
    mdlp::samples_t X = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
    mdlp::labels_t  y = { 0, 0, 0, 1, 1, 1 };

    // One call, result returned by value.
    const auto bins = mdlp::CPPFImdlp::discretize(X, y);

    // Or keep the model to transform more data later.
    mdlp::CPPFImdlp disc(mdlp::MDLPConfig{}.withMaxDepth(10));
    disc.fit(X, y);
    const auto cuts = disc.getCutPoints();

    for (const auto bin : bins) {
        std::cout << bin << ' ';
    }
    std::cout << "\ncut points: " << cuts.size() << '\n';
    return 0;
}
```

Everything lives in namespace `mdlp`. See [MIGRATION.md](MIGRATION.md) for the 3.0.0
API and [ARCHITECTURE.md](ARCHITECTURE.md) for how the pieces fit together.

## Requirements

- C++17 compiler
- CMake 3.20 or later
- Conan 2.0 or later

## A note on libtorch

`Discretizer.h` includes `torch/torch.h`, so **every** consumer links libtorch even
if it only uses the `std::vector` API. That coupling is wider than it needs to be
and has a practical consequence: because libtorch arrives prebuilt against
libstdc++, the library cannot be built against libc++ without rebuilding libtorch
from source. See the note at the end of [ARCHITECTURE.md](ARCHITECTURE.md).
