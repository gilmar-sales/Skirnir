# Skirnir

Skirnir provides an IoC Container, it uses [The C++ Type Loophole](https://alexpolt.github.io/type-loophole.html) to get
constructor parameters and allow registering services for **dependency injection**

# Lifetimes

Skirnir it's inspired by the microsoft dependency injection, services should be registered with a specific lifetime:

| Lifetime  | Description                                                                                                  |
|-----------|--------------------------------------------------------------------------------------------------------------|
| Transient | is instantiated at each invocation                                                                           |
| Scoped    | is instantiated only once per scope, then it is obtained through the scope cache                             |
| Singleton | is instantiated only once in the entire application as long as another root service provider is not created. |

# Usage

## Concrete type
start by creating the ServiceCollection and add your services by the concrete types
```cpp
    auto serviceCollection = ServiceCollection();

    serviceCollection.AddSingleton<Singleton>();
    serviceCollection.AddScoped<Scoped>();
    serviceCollection.AddTransient<Transient>();
```

## Interface/Contract
```cpp
    auto serviceCollection = ServiceCollection();

    serviceCollection.AddSingleton<ISingleton, Singleton>();
    serviceCollection.AddScoped<IScoped, Scoped>();
    serviceCollection.AddTransient<ITransient, Transient>();
```

## Injecting

```cpp
class Scoped {
public:
    Scoped(std::shared_ptr<ITransient> transient) : mTransient(transient) {}
private:
    ITransient mTransient;
}
```