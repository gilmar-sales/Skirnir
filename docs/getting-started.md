# Getting Started

## Requirements

- C++20 or later compiler
- CMake 3.16 or later
- Optional: fmt library for enhanced logging (enable with `SKIRNIR_USE_FMT`)

## Building

```bash
git clone https://github.com/Skirnir/Skirnir.git
mkdir build && cd build
cmake ..
cmake --build .
```

## Running Tests

```bash
cmake .. -DSKIRNIR_BUILD_TESTS=ON
cmake --build .
ctest
```

## Basic Usage

1. Create a `ServiceCollection`
2. Register your services with desired lifetimes
3. Build a `ServiceProvider`
4. Request services via `GetService<T>()`

```cpp
auto serviceCollection = skr::ServiceCollection();
serviceCollection.AddSingleton<MyService>();

auto serviceProvider = serviceCollection.CreateServiceProvider();
auto service = serviceProvider->GetService<MyService>();
```