# Skirnir

Skirnir provides an IoC Container. It uses [C++26 compile-time reflection](https://en.cppreference.com/w/cpp/meta)
(`std::meta::info`, the splice operator `[: ... :]`, and `^^T`) to extract
constructor parameters and to allow registering services for **dependency injection**.

## Features

- **Automatic Dependency Injection**: constructor parameters are resolved
  through compile-time introspection.
- **Multiple Lifetimes**: `Singleton`, `Scoped`, and `Transient`.
- **Multi-Registration**: register and resolve multiple implementations
  of a contract with `GetServices<T>()` or `std::vector<Arc<T>>`
  injection.
- **Keyed / Named Services**: distinguish implementations by string key,
  resolve with `GetKeyedService<T>(key)` or `Keyed<T, "key">` ctor
  injection.
- **Optional Dependencies**: `std::optional<Arc<T>>` ctor parameters
  resolve to `std::nullopt` when `T` is not registered.
- **Non-throwing Resolution**: `TryGetService<T>()` and
  `TryGetKeyedService<T>(key)` return `std::optional<Arc<T>>`.
- **Captive-Dependency Detection**: `ValidateOnBuild()` flags Singletons
  that transitively depend on Scoped services.
- **Configuration**: strongly-typed JSON configuration with `Bind<T>`,
  typed getters, sub-sections, environment variables, and source
  chaining.
- **Diagnostics**: `ValidateOnBuild()` and `PrintDiagnostics(std::ostream&)`
  for early failure detection and dependency-graph inspection.
- **Circular Dependency Detection**: detected and reported at resolution
  time.
- **Logging**: built-in logging with pluggable sinks (`ConsoleSink`,
  `FileSink`, `JsonSink`, `AsyncSink`), thread-local scopes, and a
  ready-made `LoggingExtension`.
- **Applications**: structured application model with `IApplication` and
  `ApplicationBuilder`.
- **Extensions**: modular service registration via composable
  extensions.

# Lifetimes

Skirnir is inspired by the Microsoft dependency injection.

| Lifetime  | Description                                                                                                  |
| --------- | ------------------------------------------------------------------------------------------------------------ |
| Transient | is instantiated at each invocation                                                                           |
| Scoped    | is instantiated only once per scope, then it is obtained through the scope cache                             |
| Singleton | is instantiated only once in the entire application as long as another root service provider is not created. |

# Usage

## Concrete type

Start by creating the ServiceCollection and add your services by the concrete types.

```cpp
    auto serviceCollection = skr::ServiceCollection();

    serviceCollection.AddSingleton<Singleton>();
    serviceCollection.AddScoped<Scoped>();
    serviceCollection.AddTransient<Transient>();
```

## Interface/Contract

```cpp
    auto serviceCollection = skr::ServiceCollection();

    serviceCollection.AddSingleton<ISingleton, Singleton>();
    serviceCollection.AddScoped<IScoped, Scoped>();
    serviceCollection.AddTransient<ITransient, Transient>();
```

## Multiple Implementations

Register as many implementations of a contract as you like. Resolve them
individually with `GetService<T>()` (first registration) or collectively with
`GetServices<T>()`, or inject a `std::vector<Arc<T>>` constructor parameter:

```cpp
class PluginHost {
public:
    explicit PluginHost(std::vector<Arc<IPlugin>> plugins) : mPlugins(std::move(plugins)) {}
private:
    std::vector<Arc<IPlugin>> mPlugins;
};

sc.AddTransient<IPlugin, PluginA>()
  .AddTransient<IPlugin, PluginB>()
  .AddTransient<PluginHost>();
```

For keyed implementations of the same contract, see
[Keyed Services](docs/usage/keyed-services.md). A complete working
example lives in [`examples/multi_impl/`](examples/multi_impl).

## Injecting

```cpp
class Scoped {
public:
    Scoped(Arc<ITransient> transient) : mTransient(transient) {}
private:
    Arc<ITransient> mTransient;
};
```

## Configuration

Skirnir includes a strongly-typed JSON configuration system built on
[simdjson](https://github.com/simdjson/simdjson). Chain multiple sources and
bind sub-trees directly into C++ structs:

```cpp
struct DatabaseOptions {
    std::string host = "localhost";
    int         port = 5432;
    bool        ssl  = false;
};

auto config = skr::ConfigurationBuilder()
                  .AddJsonFile("config.json")
                  .AddJsonString(R"({"db": {"port": 3306}})")
                  .Build();
auto db = config->Bind<DatabaseOptions>("db");
```

## Logging

Skirnir provides global logging capabilities. Every registered service
automatically receives a `skr::Logger<TService>` which can be injected as
a constructor parameter. The logger formats messages with `std::format`
by default (and [fmt](https://github.com/fmtlib/fmt) when the project is
configured with `-DSKIRNIR_USE_FMT=ON`):

```cpp
class Repository : public IRepository
{
  public:
    Repository(Arc<skr::Logger<Repository>> logger) : mLogger(logger) {}
    ~Repository() override = default;

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Arc<skr::Logger<Repository>> mLogger;
};
```

Calling the `Repository::Add()` method will output:

```log
[Information] 2025-03-12 21:55:19.650921540 'Repository': Add
```

## Diagnostics

Validate the service graph and print a diagnostic tree at startup:

```cpp
auto sp = serviceCollection.CreateServiceProvider();
sp->ValidateOnBuild();
sp->PrintDiagnostics(std::cout);
```
