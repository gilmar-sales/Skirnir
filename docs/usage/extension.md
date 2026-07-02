# Extensions

Extensions allow you to modularize and compose service registration logic. They follow the decorator pattern to add services and configuration to your application.

## IExtension Interface

Create a class that inherits from `IExtension`:

```cpp
class DatabaseExtension : public skr::IExtension
{
public:
    void ConfigureServices(skr::ServiceCollection& services) override
    {
        services.AddSingleton<IDatabase, SqlDatabase>();
    }

    void UseServices(skr::ServiceProvider& serviceProvider) override
    {
        auto db = serviceProvider.GetService<IDatabase>();
        db->Initialize();
    }
};
```

> Note: `Attach`, `ConfigureServices` and `UseServices` are `protected`
> hooks (see `include/Skirnir/DependencyInjection/Extension.hpp`). They take
> the builder/collection/provider **by reference**, not by `Arc`. The
> `ApplicationBuilder` calls them for you; user code never needs to invoke
> them directly.

### Lifecycle Methods

| Method | When Called | Purpose |
|--------|-------------|---------|
| `Attach` | When extension is added to `ApplicationBuilder` (via `WithExtension`) | Configure the builder itself |
| `ConfigureServices` | Before `ServiceProvider` is created | Register services into the `ServiceCollection&` |
| `UseServices` | After `ServiceProvider` is created, before `Build` returns | Initialize services using the `ServiceProvider&` |

### Attaching to ApplicationBuilder

```cpp
class RoutingExtension : public skr::IExtension
{
public:
    void Attach(skr::ApplicationBuilder& applicationBuilder) override
    {
        mBuilder = &applicationBuilder;
    }

    void UseServices(skr::ServiceProvider& serviceProvider) override
    {
        auto router = serviceProvider.GetService<IRouter>();
        mBuilder->GetServiceCollection()->AddSingleton(std::move(router));
    }

private:
    skr::ApplicationBuilder* mBuilder = nullptr;
};
```

## Adding Extensions

Use `WithExtension<T>()` to register an extension:

```cpp
skr::ApplicationBuilder()
    .WithExtension<DatabaseExtension>()
    .WithExtension<LoggingExtension>()
    .Build<MyApp>();
```

### With Configuration

Pass a callback to configure the extension before it's attached:

```cpp
skr::ApplicationBuilder()
    .WithExtension<LoggingExtension>([](skr::LoggingExtension& ext) {
        ext.SetLevel(skr::LogLevel::Debug);
    })
    .Build<MyApp>();
```

## Extension Ordering

Extensions are executed in the order they are added. Each lifecycle method is called sequentially:

1. All `Attach` methods are called in the order extensions were added
2. All `ConfigureServices` methods are called in the order extensions were added
3. All `UseServices` methods are called in the order extensions were added

If an extension's `UseServices` throws, subsequent extensions' `UseServices` methods are **not** called.

The `IApplication` singleton is registered automatically during `ApplicationBuilder::Build`, after all extensions' `ConfigureServices` have run but before any `UseServices` is called.

## Use Cases

- **Modular service registration**: Group related services (database, logging, caching)
- **Environment-based configuration**: Different extensions for dev/prod
- **Service initialization**: Perform setup that requires resolved dependencies

```cpp
class CachingExtension : public skr::IExtension
{
public:
    void ConfigureServices(skr::ServiceCollection& services) override
    {
        services.AddSingleton<ICache, RedisCache>();
    }

    void UseServices(skr::ServiceProvider& serviceProvider) override
    {
        mCache = serviceProvider.GetService<ICache>();
        mCache->Connect("redis://localhost:6379");
    }

private:
    Arc<ICache> mCache;
};
```