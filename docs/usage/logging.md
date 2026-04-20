# Logging

Skirnir uses [fmt](https://github.com/fmtlib/fmt) for logging when `SKIRNIR_USE_FMT` is defined. Each registered service automatically gets a `skr::Logger<TService>`.

## Logger Injection

```cpp
class Repository : public IRepository
{
  public:
    Repository(Ref<skr::Logger<Repository>> logger) : mLogger(logger) {}

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Ref<skr::Logger<Repository>> mLogger;
};
```

## Log Levels

| Level       | Description                          |
|-------------|--------------------------------------|
| Debug       | Debug messages (default in debug)    |
| Trace       | Trace messages (default in release) |
| Information | General information messages        |
| Warning     | Warning messages                     |
| Error       | Error messages                       |
| Fatal       | Fatal errors (throws exception)      |

## Output Example

```
[Information] 2025-03-12 21:55:19.650921540 'Repository': Add
```