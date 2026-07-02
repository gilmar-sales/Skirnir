# Logging

Skirnir uses [`std::format`](https://en.cppreference.com/w/cpp/utility/format/format)
for log message formatting. Each registered service automatically gets a
`skr::Logger<TService>`.

## Quick Start

```cpp
class Repository : public IRepository
{
  public:
    Repository(Arc<skr::Logger<Repository>> logger) : mLogger(logger) {}

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Arc<skr::Logger<Repository>> mLogger;
};
```

## Log Levels

| Level       | Description                          |
|-------------|--------------------------------------|
| Debug       | Debug messages (default in debug)    |
| Trace       | Trace messages (default in release)  |
| Information | General information messages         |
| Warning     | Warning messages                     |
| Error       | Error messages                       |
| Fatal       | Fatal errors (throws after logging)  |
| None        | Disables all logging                 |

`LogFatal` dispatches the record first, then throws `std::runtime_error`.

## Output

```
[Information] 2025-03-12 21:55:19.650921540 'Repository': Add
```

The default sink is a `ConsoleSink` that writes to `std::cout`.

---

## Sinks

Sinks are destinations for log records. `LoggerOptions` owns a list of
sinks; every record dispatched by every `Logger<T>` reaches all of them.

```cpp
auto options = skr::MakeArc<skr::LoggerOptions>();
options->AddSink(skr::MakeArc<skr::ConsoleSink>());
options->AddSink(skr::MakeArc<skr::FileSink>("app.log"));
options->AddSink(skr::MakeArc<skr::JsonSink>("app.ndjson"));
```

Or compose them with the `LoggingExtension`:

```cpp
skr::ApplicationBuilder()
    .WithExtension<MyExtension>()
    .WithExtension<skr::LoggingExtension>(
        [](skr::LoggingExtension& l) {
            l.AddConsoleSink()
             .AddFileSink("app.log")
             .AddJsonSink("app.ndjson")
             .WithAsyncQueue();
        })
    .Build<MyApp>();
```

### Built-in sinks

| Class         | Purpose                                                |
|---------------|--------------------------------------------------------|
| `ConsoleSink` | Writes to `std::cout`. Honors `NO_COLOR`.              |
| `FileSink`    | Appends plain-text lines to a file.                    |
| `JsonSink`    | Emits NDJSON (one record per line) for log ingestion.  |
| `AsyncSink`   | Decorator that queues records and forwards from a worker thread. |
| `NullSink`    | Discards everything. Useful in tests.                  |

### Custom sinks

Implement `ILogSink`:

```cpp
class RemoteSink final : public skr::ILogSink
{
  public:
    void Write(const skr::LogRecord& r) override
    {
        // POST r.message to your log aggregator.
    }
};

options->AddSink(skr::MakeArc<RemoteSink>());
```

### Async sink

`AsyncSink` wraps another sink and forwards records from a background
thread. `Write()` never blocks; when the bounded queue is full the
newest record is dropped and a counter is incremented (inspect with
`AsyncSink::DroppedCount()`).

---

## Log scopes

`BeginScope(name)` returns a RAII handle. While alive, every record
dispatched on the same thread has the scope name prepended:

```cpp
auto scope = options->BeginScope("request-42");
logger->LogInformation("processing");        // [scope: request-42]
// ... deep call stack, no need to thread scope through ...
scope = nullptr;  // popped
```

Scopes are **thread-local**, matching .NET `BeginScope` ergonomics. A
scope pushed on one thread is invisible to other threads.

---

## Structured logging

C++ does not provide first-class named-parameter support, so Skirnir
relies on the `std::format` API for the message itself. Format arguments
that need to be queryable as structured data should be embedded in the
message using positional placeholders:

```cpp
logger->LogInformation("user_login user_id={} ip={}", 42, "1.2.3.4");
```

`JsonSink` will then emit the fully-rendered message as a single JSON
string, which downstream log processors (Loki, ELK) can parse if
needed. For richer structured logging in C++26, look for first-class
reflection (`^^T`) and the splice operator (`[: ... :]`) — once those
land in a production compiler, Skirnir can grow a richer fields API.

---

## Configuration

The `logging.logLevel` configuration block controls levels per
namespace. See [Configuration](configuration.md) for the full schema.

```json
{
  "logging": {
    "logLevel": {
      "default": "Information",
      "my_app::services": "Debug",
      "my_app::services::Database": "Trace"
    }
  }
}
```

```cpp
auto options = skr::MakeArc<skr::LoggerOptions>();
options->ConfigureFrom(config);
```
