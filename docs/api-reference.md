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

### Lifetime

Enum specifying a service lifetime:

| Value     | Description                     |
| --------- | ------------------------------- |
| Transient | New instance each request       |
| Scoped    | One instance per scope          |
| Singleton | Single instance per application |

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

#### Keyed / Named Registration

Register multiple implementations of one contract distinguished by a
string key. See [Keyed Services](usage/keyed-services.md).

```cpp
ServiceCollection& AddKeyedSingleton<TContract, TService>(std::string key);
ServiceCollection& AddKeyedScoped<TContract, TService>(std::string key);
ServiceCollection& AddKeyedTransient<TContract, TService>(std::string key);
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

template <typename TService>
std::optional<Ref<TService>> TryGetService();

template <typename TService>
std::vector<Ref<TService>> GetServices();

template <typename TService>
Ref<TService> GetKeyedService(std::string_view key);

template <typename TService>
std::optional<Ref<TService>> TryGetKeyedService(std::string_view key);
```

### Validation and Diagnostics

```cpp
void ValidateOnBuild();
void PrintDiagnostics(std::ostream& os) const;
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

| Level       | Description                         |
| ----------- | ----------------------------------- |
| Debug       | Debug messages (default in debug)   |
| Trace       | Trace messages (default in release) |
| Information | General information messages        |
| Warning     | Warning messages                    |
| Error       | Error messages                      |
| Fatal       | Fatal errors (throws exception)     |
| None        | Disables all logging                |

### Logging Methods

Format-string API (existing):

```cpp
template <typename... TArgs>
void LogDebug(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogTrace(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogInformation(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogWarning(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogError(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void LogFatal(std::format_string<TArgs...> fmt, TArgs&&... args);

template <typename... TArgs>
void Assert(bool assertion, std::format_string<TArgs...> fmt, TArgs&&... args);
```

---

## LogRecord

A complete log entry, ready to be handed to a sink.

```cpp
struct LogRecord
{
    LogLevel                              level;
    std::chrono::system_clock::time_point timestamp;
    std::string_view                      category;
    std::string                           message;
    std::vector<std::string_view>         scopes;
    std::source_location                  location;
};
```

---

## ILogSink

```cpp
class ILogSink
{
  public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogRecord& record) = 0;
    virtual void Flush() {}
};
```

### Built-in Sinks

| Class         | Constructor                                          |
|---------------|------------------------------------------------------|
| `NullSink`    | `NullSink()`                                         |
| `ConsoleSink` | `ConsoleSink(bool useColors = true)`                 |
| `FileSink`    | `FileSink(path, bool autoFlush = true)`              |
| `JsonSink`    | `JsonSink(std::ostream&)` or `JsonSink(path)`        |
| `AsyncSink`   | `AsyncSink(Ref<ILogSink> inner, size_t capacity)`    |

`AsyncSink::DroppedCount()` returns the number of records dropped due
to a full queue.

---

## LogScope

RAII handle returned by `LoggerOptions::BeginScope`.

```cpp
class LogScope
{
  public:
    const std::string& Name() const noexcept;
    // Non-copyable, non-movable.
};
```

---

## LoggerOptions

Configuration class for logging behavior:

```cpp
class LoggerOptions {
    LogLevel logLevel;  // Default: Debug (debug build), Trace (release)

    // Sink management
    LoggerOptions& AddSink(Ref<ILogSink> sink);
    const std::vector<Ref<ILogSink>>& Sinks() const noexcept;
    void ClearSinks();

    // Dispatch (internal — called by Logger<T>)
    void Dispatch(const LogRecord& record);

    // Scopes
    Ref<LogScope> BeginScope(std::string name);

    // Configuration
    void ConfigureFrom(Ref<ConfigurationOptions> config,
                       std::string_view path = "logging.logLevel.default");

    template <typename T> LogLevel GetLogLevelFor();
};
```

Inject `Ref<LoggerOptions>` to customize logging.

---

## Configuration

### ConfigurationBuilder

```cpp
ConfigurationBuilder& AddJsonFile(const std::filesystem::path& path);
ConfigurationBuilder& AddJsonString(std::string_view json);
ConfigurationBuilder& AddSource(Ref<IConfigurationSource> source);
ConfigurationBuilder& AddInMemory(
    std::initializer_list<std::pair<std::string, std::string>> entries);

Ref<ConfigurationOptions> Build();
```

### ConfigurationOptions

```cpp
std::optional<std::string> GetValue(std::string_view key) const;
bool                       HasKey(std::string_view key) const;

bool        GetBool(std::string_view key, bool defaultValue = false) const;
int64_t     GetInt(std::string_view key, int64_t defaultValue = 0) const;
double      GetDouble(std::string_view key, double defaultValue = 0.0) const;
std::string GetString(std::string_view key, std::string_view defaultValue = "") const;
std::vector<std::string> GetArray(std::string_view key) const;

Ref<ConfigurationOptions> GetSection(std::string_view key) const;

template <typename T>
T Bind(std::string_view section = "") const;
```

### IConfigurationSource

Abstract base class for custom configuration sources. Override `Load()` to
produce a `simdjson::dom::element` representing the root of your data.

### JsonObjectReader

```cpp
bool Contains(std::string_view key) const;
template <typename T> bool TryGet(std::string_view key, T& out) const;
```

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
