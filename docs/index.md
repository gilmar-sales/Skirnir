# Skirnir

Skirnir provides an IoC (Inversion of Control) Container for dependency injection in C++. Skirnir uses [The C++ Type Loophole](https://alexpolt.github.io/type-loophole.html) to automatically extract constructor parameters and enable seamless dependency injection.

## Features

- **Automatic Dependency Injection**: Automatically resolves constructor dependencies
- **Multiple Lifetimes**: Support for Singleton, Scoped, and Transient lifetimes
- **Multi-Registration**: Register and resolve multiple implementations of a contract with `GetServices<T>()` or `std::vector<Arc<T>>` injection
- **Keyed / Named Services**: Distinguish multiple implementations of one contract by string key — resolve via `GetKeyedService<T>(key)` or `Keyed<T, "key">` ctor injection
- **Optional Dependencies**: `std::optional<Arc<T>>` ctor parameters resolve to `nullopt` when `T` is not registered
- **Non-throwing Resolution**: `TryGetService<T>()` and `TryGetKeyedService<T>(key)` return `std::optional<Arc<T>>` instead of throwing
- **Captive-Dependency Detection**: `ValidateOnBuild()` flags Singletons that transitively depend on Scoped services
- **Configuration**: Strongly-typed JSON configuration with `Bind<T>()`, typed getters, sub-sections, and source chaining
- **Diagnostics**: `ValidateOnBuild()` and `PrintDiagnostics(std::ostream&)` for early failure detection
- **Circular Dependency Detection**: Detects and reports circular dependencies
- **Logging**: Built-in logging with pluggable sinks (`ConsoleSink`, `FileSink`, `JsonSink`, `AsyncSink`) and scopes/correlation IDs
- **Reflection**: Uses compile-time type introspection via the C++ Type Loophole to extract service metadata
- **Applications**: Structured application model with `IApplication` and `ApplicationBuilder`
- **Async / Coroutines**: Pure C++26 coroutines (`Task<T>`, `co_await`, cancellation).
- **Extensions**: Modular service registration via composable extensions (includes a ready-made `LoggingExtension`)

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
