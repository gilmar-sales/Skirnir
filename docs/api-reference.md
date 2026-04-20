# API Reference

## Core Types

### Ref<T>

`Ref<T>` is an alias for `std::shared_ptr<T>`. Used to hold references to services.

### MakeRef<T>(args...)

Creates a `Ref<T>` instance. Factory function for service creation.

```cpp
template <typename T, typename... TArgs>
    requires(std::is_constructible_v<T, TArgs...>)
Ref<T> MakeRef(TArgs&&... args);
```

### LifeTime

Enum specifying service lifetime:

| Value      | Description                                      |
|------------|--------------------------------------------------|
| Transient  | New instance each request                        |
| Scoped     | One instance per scope                          |
| Singleton  | Single instance per application                  |

### ServiceFactory

Function type for service factories:

```cpp
using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;
```

---

## ServiceCollection

### Lifetime Registration Methods

#### Concrete Type Registration

```cpp
ServiceCollection& AddSingleton<TService>();
ServiceCollection& AddScoped<TService>();
ServiceCollection& AddTransient<TService>();
```

#### Contract/Interface Registration

```cpp
ServiceCollection& AddSingleton<TContract, TService>();
ServiceCollection& AddScoped<TContract, TService>();
ServiceCollection& AddTransient<TContract, TService>();
```

#### Factory Registration

```cpp
ServiceCollection& AddSingleton<TService>(const ServiceFactory& factory);
ServiceCollection& AddScoped<TService>(const ServiceFactory& factory);
ServiceCollection& AddTransient<TService>(const ServiceFactory& factory);
```

#### Instance Registration

```cpp
ServiceCollection& AddSingleton<TService>(Ref<TService> element);
ServiceCollection& AddSingleton<TContract, TService>(Ref<TService> element);
```

### Utility Methods

```cpp
bool Contains<TService>() const;
Ref<ServiceProvider> CreateServiceProvider();
```

---

## ServiceProvider

### Service Retrieval

```cpp
template <typename TService>
Ref<TService> GetService();
```

### Utility Methods

```cpp
template <typename TService>
bool Contains() const;

Ref<ServiceScope> CreateServiceScope() const;
```

---

## ServiceScope

### Constructor

```cpp
ServiceScope(const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
             const Ref<ServicesCache>&        singletonsCache);
```

### Methods

```cpp
Ref<ServiceProvider> GetServiceProvider() const;
```

---

## Logger

### Log Levels

| Level       | Description                          |
|-------------|--------------------------------------|
| Debug       | Debug messages (default in debug)    |
| Trace       | Trace messages (default in release) |
| Information | General information messages        |
| Warning     | Warning messages                     |
| Error       | Error messages                       |
| Fatal       | Fatal errors (throws exception)      |

### Logging Methods

```cpp
template <typename... TArgs>
void LogDebug(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogTrace(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogInformation(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogWarning(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogError(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogFatal(fmt::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void Assert(bool assertion, fmt::format_string<TArgs...> fmt, TArgs&&... args);
```

---

## LoggerOptions

Configuration class for logging behavior:

```cpp
class LoggerOptions {
    LogLevel logLevel;  // Default: Debug (debug build), Trace (release)
};
```

Inject `Ref<LoggerOptions>` to customize logging level.

---

## Application

### IApplication

Abstract base class for applications:

```cpp
class IApplication {
public:
    IApplication(const Ref<ServiceProvider>& rootServiceProvider);
    virtual ~IApplication() = default;
    Ref<ServiceProvider> GetRootServiceProvider() const;
    virtual void Run() = 0;
};
```

The `IApplication` singleton is automatically registered when using `ApplicationBuilder`.

---

## ApplicationBuilder

For registering applications with the container:

```cpp
auto app = ApplicationBuilder().Build<MyApplication>();
```

### Methods

```cpp
template <typename TApplication>
    requires(std::is_base_of_v<IApplication, TApplication>)
Ref<TApplication> Build();
```