# Concrete Type Registration

Register services by their concrete type:

```cpp
auto serviceCollection = skr::ServiceCollection();

serviceCollection.AddSingleton<Singleton>();
serviceCollection.AddScoped<Scoped>();
serviceCollection.AddTransient<Transient>();
```