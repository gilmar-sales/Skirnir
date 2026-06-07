# Async / Coroutines

Skirnir provides first-class C++26 coroutine support. The core async types
are header-only.

## Quick Start

```cpp
#include <Skirnir/Async.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>

class MyApp : public skr::IAsyncApplication
{
public:
    explicit MyApp(const skr::Arc<skr::ServiceProvider>& root)
        : IAsyncApplication(root) {}

    skr::Task<> RunAsync() override
    {
        auto logger = co_await mRootServiceProvider
                           ->GetServiceAsync<skr::Logger<MyApp>>();
        for (int i = 0; i < 3; ++i)
        {
            co_await skr::Async::Delay(std::chrono::milliseconds(50));
            logger->LogInformation("tick {}", i);
        }
    }
};

int main()
{
    auto app = skr::ApplicationBuilder()
                   .WithExtension<skr::LoggingExtension>()
                   .BuildAsync<MyApp>();
    return skr::AsyncApplicationHost::Run(*app);
}
```

## `Task<T>`

`Task<T>` is a coroutine return type. It is eagerly started (`initial_suspend
== suspend_never`) and supports `co_await`, cancellation via
`std::stop_token`, and optional scheduler-aware resumption.

```cpp
skr::Task<int> ComputeAsync()
{
    co_await skr::Async::Delay(std::chrono::milliseconds(10));
    co_return 42;
}

skr::Task<> UseIt()
{
    int v = co_await ComputeAsync();   // 42
}
```

## Cancellation

Every `Task<T>` owns a `std::stop_source`. Call `request_stop()` on the
task (or use the application-level `RequestStop()`) to signal cancellation.
Use `Async::CancelIfRequested(token)` or `Async::DelayFor` to cooperate:

```cpp
skr::Task<> CancellableWork()
{
    co_await skr::Async::DelayFor(std::chrono::seconds(10), app.Cancellation());
    // If the app's RequestStop() was called within 10 seconds, this throws
    // std::system_error(errc::operation_canceled).
}
```

## `IAsyncApplication`

`IAsyncApplication` mirrors `IApplication` for coroutine-driven apps:

```cpp
class IAsyncApplication
{
public:
    explicit IAsyncApplication(const skr::Arc<ServiceProvider>& root);
    Arc<ServiceProvider> GetRootServiceProvider() const;
    void RequestStop() noexcept;
    CancellationToken Cancellation() const noexcept;
    virtual Task<> RunAsync() = 0;
};
```

Build it with `ApplicationBuilder::BuildAsync<T>()` and drive it with
`AsyncApplicationHost::Run(app)`.

## `GetServiceAsync<T>()`

The async counterpart of `GetService<T>()` returns a `Task<Arc<T>>`. It
keeps a strong reference to the `ServiceProvider` for the lifetime of the
task, so scoped services stay alive across `co_await` points:

```cpp
skr::Task<> HandleRequest()
{
    auto scope = mRootServiceProvider->CreateServiceScope();
    auto ctx   = co_await scope->GetServiceProvider()
                       ->GetServiceAsync<IRequestContext>();
    co_await skr::Async::Delay(std::chrono::milliseconds(10));
    // ctx is still valid here
}
```

## Scheduler

A `Scheduler` is anything callable as `s.schedule(coroutine_handle)`. The
built-in `InlineScheduler` resumes on the current thread:

```cpp
inline_scheduler.schedule(handle);   // resumes inline
```

Custom schedulers (thread pools, executors) implement the same concept.

## Limitations (v1)

- `when_all` and `when_any` are sequential — a true concurrent fan-out
  will follow in a later version.
- `LoggerOptions::BeginScope` is per-thread. If a task resumes on a
  different thread, the scope name will not be inherited.
- The `Delay` helper uses `std::async` + a detached `std::thread` and is
  intended for tests and simple cases.
