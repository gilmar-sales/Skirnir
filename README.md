# Skirnir

Skirnir provides an IoC Container. It uses [The C++ Type Loophole](https://alexpolt.github.io/type-loophole.html) to get
constructor parameters and to allow registering services for **dependency injection**

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
`GetServices<T>()`, or inject a `std::vector<Ref<T>>` constructor parameter:

```cpp
class PluginHost {
public:
    explicit PluginHost(std::vector<Ref<IPlugin>> plugins) : mPlugins(std::move(plugins)) {}
private:
    std::vector<Ref<IPlugin>> mPlugins;
};

sc.AddTransient<IPlugin, PluginA>()
  .AddTransient<IPlugin, PluginB>()
  .AddTransient<PluginHost>();
```

## Injecting

```cpp
class Scoped {
public:
    Scoped(Ref<ITransient> transient) : mTransient(transient) {}
private:
    Ref<ITransient> mTransient;
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

Skirnir uses the [fmt](https://github.com/fmtlib/fmt) to provide global logging capabilities, every service added will also add a `skr::Logger<TService>`. This logger can be injected and produces logging messages with the typename of the template argument:

```cpp
class Repository : public IRepository
{
  public:
    Repository(Ref<skr::Logger<Repository>> logger) : mLogger(logger) {}
    ~Repository() override = default;

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Ref<skr::Logger<Repository>> mLogger;
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
