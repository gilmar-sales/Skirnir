# Interface/Contract Registration

Register services via interface or contract:

```cpp
auto serviceCollection = skr::ServiceCollection();

serviceCollection.AddSingleton<ISingleton, Singleton>();
serviceCollection.AddScoped<IScoped, Scoped>();
serviceCollection.AddTransient<ITransient, Transient>();
```

The first template parameter is the contract/interface type, and the second is the implementation type.