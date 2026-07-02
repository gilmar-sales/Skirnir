# Getting Started

## Requirements

- C++26 capable compiler (project enables `CXX_STANDARD 26` and uses
  compile-time reflection — Clang/GCC with `-freflection`, MSVC with
  `/experimental:reflection`)
- CMake 3.28 or later
- simdjson (fetched automatically by `FetchContent`)
- Optional: `fmt` library for enhanced logging. Enable with
  `-DSKIRNIR_USE_FMT=ON` when configuring Skirnir as a subdirectory.

## Building

```bash
git clone https://github.com/gilmar-sales/Skirnir.git
mkdir build && cd build
cmake ..
cmake --build .
```

## Running Tests

```bash
cmake .. -DBUILD_SKIRNIR_TESTS=ON
cmake --build .
ctest
```

## Building the Examples

```bash
cmake .. -DBUILD_SKIRNIR_EXAMPLES=ON
cmake --build .
```

The top-level `CMakeLists.txt` enables tests and examples by default when
Skirnir is the project root. When consuming Skirnir as a subdirectory the
options default to `OFF`.

## Basic Usage

1. Create a `ServiceCollection`
2. Register your services with desired lifetimes
3. Build a `ServiceProvider`
4. Request services via `GetService<T>()`

```cpp
#include <Skirnir/Skirnir.hpp>

auto serviceCollection = skr::ServiceCollection();
serviceCollection.AddSingleton<MyService>();

auto serviceProvider = serviceCollection.CreateServiceProvider();
auto service = serviceProvider->GetService<MyService>();
```