# Multi-Registration

A service id can have _multiple_ registrations. `GetService<T>()` returns
the first registration; `GetServices<T>()` returns all of them. Constructor
parameters of type `std::vector<Ref<T>>` are resolved with the same
`GetServices` semantics.

## Registering Multiple Implementations

```cpp
class IPlugin { public: virtual std::string Name() const = 0; };
class PluginA : public IPlugin { public: std::string Name() const override { return "A"; } };
class PluginB : public IPlugin { public: std::string Name() const override { return "B"; } };
class PluginC : public IPlugin { public: std::string Name() const override { return "C"; } };

auto sc = skr::ServiceCollection()
              .AddTransient<IPlugin, PluginA>()
              .AddTransient<IPlugin, PluginB>()
              .AddTransient<IPlugin, PluginC>();
```

## Resolving

```cpp
auto sp = sc.CreateServiceProvider();

auto first = sp->GetService<IPlugin>();  // PluginA
auto all   = sp->GetServices<IPlugin>(); // PluginA, PluginB, PluginC
```

## Injecting a Vector

A consumer that wants every implementation can declare a `std::vector<Ref<T>>`
constructor parameter:

```cpp
class PluginHost
{
public:
    explicit PluginHost(std::vector<Ref<IPlugin>> plugins) : mPlugins(std::move(plugins)) {}
    void List() const { for (const auto& p : mPlugins) std::cout << p->Name() << "\n"; }
private:
    std::vector<Ref<IPlugin>> mPlugins;
};

sc.AddTransient<PluginHost>();
auto host = sp->GetService<PluginHost>();
host->List();
```

## Lifetime Notes

- **Transient**: each registration yields a fresh instance.
- **Singleton**: the first registration wins; subsequent registrations for
  the same id are still enumerated by `GetServices` but yield the same
  cached instance (de-duplicated by pointer).
- **Scoped**: behaves like Singleton within a scope.

A complete example lives in `examples/multi_impl/`.
