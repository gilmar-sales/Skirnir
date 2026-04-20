# Lifetimes

Skirnir supports three service lifetimes, inspired by Microsoft.Extensions.DependencyInjection:

| Lifetime  | Description                                                                                              |
|-----------|----------------------------------------------------------------------------------------------------------|
| Singleton | Single instance per application (until a new root service provider is created)                         |
| Scoped    | One instance per scope, reused within that scope                                                         |
| Transient | New instance created each time the service is requested                                                 |

## Singleton

A singleton service is created once and shared across the entire application:

```cpp
serviceCollection.AddSingleton<MySingleton>();
```

## Scoped

Scoped services are created once per scope. Create a scope using `CreateServiceScope()`:

```cpp
auto scope = serviceProvider->CreateServiceScope();
auto scopedService = scope->GetServiceProvider()->GetService<MyScoped>();
```

## Transient

A new instance is created each time the service is requested:

```cpp
serviceCollection.AddTransient<MyTransient>();
```

## Choosing a Lifetime

- Use **Singleton** for stateless services or services that are expensive to create
- Use **Scoped** for services that should maintain state within a single request/scope
- Use **Transient** for lightweight, stateless services