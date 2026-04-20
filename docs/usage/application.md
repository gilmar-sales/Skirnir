# Applications

Skirnir provides the `IApplication` interface and `ApplicationBuilder` to structure your application around the dependency injection container.

## IApplication

`IApplication` is the base interface for your application. It receives the root `ServiceProvider` and must implement the `Run()` method:

```cpp
class MyApp : public skr::IApplication
{
public:
    MyApp(Ref<skr::ServiceProvider> rootServiceProvider) :
        skr::IApplication(rootServiceProvider)
    {
    }

    void Run() override
    {
        auto service = mRootServiceProvider->GetService<MyService>();
        service->DoWork();
    }
};
```

The `IApplication` singleton is automatically registered when using `ApplicationBuilder`.

## ApplicationBuilder

`ApplicationBuilder` helps you construct your application with services and extensions:

```cpp
auto app = skr::ApplicationBuilder()
    .AddExtension<MyExtension>()
    .Build<MyApp>();

app->Run();
```

### Service Registration

You can register services directly via the builder's service collection:

```cpp
auto appBuilder = skr::ApplicationBuilder();

appBuilder.GetServiceCollection()
    ->AddSingleton<MyService>()
    ->AddTransient<IRepository, Repository>();

appBuilder.Build<MyApp>();
```

### Extension Composition

Use `AddExtension<T>()` to add extension modules. Extensions can be added with or without a configuration callback:

```cpp
skr::ApplicationBuilder()
    .AddExtension<DatabaseExtension>()  // Uses defaults
    .AddExtension<LoggingExtension>([](Ref<LoggingExtension> ext) {
        ext->SetLevel(skr::LogLevel::Debug);
    })
    .Build<MyApp>();
```

See [Extensions](extension.md) for more information on creating and using extensions.