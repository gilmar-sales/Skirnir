# Skirnir AGENTS.md

Guidelines for AI coding agents working on the Skirnir C++ IoC Container library.

## Project Overview

Skirnir is a C++23 dependency injection library using C++20 modules. It provides an IoC container with Singleton, Scoped, and Transient lifetimes.

## Build Commands

```bash
# Configure (from repo root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_SKIRNIR_TESTS=ON -G Ninja

# Build all
cmake --build build --config Debug

# Run all tests
ctest --test-dir build/test --build-config Debug

# Run single test (using gtest filter)
./build/test/SkirnirTest_run --gtest_filter="ServiceProviderSpec.ServiceProviderShouldGetSingleton"

# Format code
clang-format -i src/*.cpp test/src/*.cpp examples/*.cpp
```

## Code Style Guidelines

### Formatting
- Use `.clang-format` (Microsoft-based style, 80 column limit)
- Indent: 4 spaces, never tabs
- Pointer alignment: Left (`Ref<T> ptr`, not `Ref<T>ptr`)
- Namespace indentation: All levels
- Break constructor initializers: After colon
- Short functions: Inline only, never on single line if complex

### Naming Conventions
- **Classes/Structs**: PascalCase (e.g., `ServiceProvider`, `ServiceCollection`)
- **Methods**: PascalCase (e.g., `GetService`, `AddSingleton`)
- **Type aliases**: PascalCase (e.g., `Ref`, `ServiceId`, `ServiceFactory`)
- **Member variables**: Prefix with `m` + PascalCase (e.g., `mServiceProvider`)
- **Template parameters**: PascalCase (e.g., `TService`, `TArgs`)
- **Enums**: PascalCase for enum and values (e.g., `LifeTime::Singleton`)
- **Tests**: Descriptive TEST_F names (e.g., `ServiceProviderShouldGetSameSingletonAtAnyTime`)

### C++ Modules
- All implementation in `.cpp` files using C++20 modules
- No header files (`.hpp`) - everything is module-based
- Main module: `export module Skirnir;`
- Submodules: `export module Skirnir:ServiceProvider;`
- Import standard library: `import std;`
- Export namespace: `export namespace skr { }`

### Code Patterns

**Template requires clauses:**
```cpp
template <typename TContract, typename TService>
    requires(std::is_base_of_v<TContract, TService>)
ServiceCollection& AddSingleton()
{
    // ...
    return *this;  // Return *this for method chaining
}
```

**Type aliases (in Core.cpp):**
```cpp
export template <typename T>
using Ref = std::shared_ptr<T>;
```

**Factory pattern with MakeRef:**
```cpp
template <typename T, typename... TArgs>
    requires(std::is_constructible_v<T, TArgs...>)
Ref<T> MakeRef(TArgs&&... args)
{
    return std::make_shared<T>(std::forward<TArgs>(args)...);
}
```

### Testing
- Use Google Test (gtest)
- Test classes inherit from `::testing::Test`
- Use `SetUp()` and `TearDown()` for fixture setup
- Use `TEST_F(FixtureName, TestName)` for test cases
- Test files: `test/src/*Spec.cpp`

### Error Handling
- Use `mLogger->Assert()` for invariant checks
- Use `mLogger->LogFatal()` for unrecoverable errors
- Throw exceptions for user-facing errors (e.g., circular dependencies)

### Imports
1. Standard library: `import std;`
2. Project modules: `import :Submodule;`
3. Test headers: `#include "gtest/gtest.h"`

## CI/CD

GitHub Actions workflow builds on:
- Windows (MSVC cl)
- Ubuntu (GCC 15, Clang)

Uses Ninja generator and requires C++23 support.

## Architecture Notes

- **ServiceCollection**: Builder pattern for registering services
- **ServiceProvider**: Resolves and caches services
- **ServiceScope**: Scoped lifetime management
- **Ref<T>**: Alias for `std::shared_ptr<T>`
- Services identified by `ServiceId` (auto-incrementing counter)
