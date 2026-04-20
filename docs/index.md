# Skirnir

Skirnir provides an IoC (Inversion of Control) Container for dependency injection in C++. Skirnir uses [The C++ Type Loophole](https://alexpolt.github.io/type-loophole.html) to automatically extract constructor parameters and enable seamless dependency injection.

## Features

- **Automatic Dependency Injection**: Automatically resolves constructor dependencies
- **Multiple Lifetimes**: Support for Singleton, Scoped, and Transient lifetimes
- **Circular Dependency Detection**: Detects and reports circular dependencies
- **Logging**: Built-in logging support with `skr::Logger<T>`
- **Reflection**: Uses compile-time reflection to extract service metadata
- **Applications**: Structured application model with `IApplication` and `ApplicationBuilder`
- **Extensions**: Modular service registration via composable extensions

## Quick Start

```cpp
#include <Skirnir/Skirnir.hpp>

int main() {
    auto serviceCollection = skr::ServiceCollection();

    serviceCollection.AddSingleton<MySingleton>();
    serviceCollection.AddScoped<MyScoped>();
    serviceCollection.AddTransient<MyTransient>();

    auto serviceProvider = serviceCollection.CreateServiceProvider();
    auto myService = serviceProvider->GetService<MySingleton>();

    return 0;
}
```

## Installation

Skirnir uses CMake for building. Add the following to your `CMakeLists.txt`:

```cmake
add_subdirectory(Skirnir)
target_link_libraries(your_target PRIVATE Skirnir)
```

## License

MIT License