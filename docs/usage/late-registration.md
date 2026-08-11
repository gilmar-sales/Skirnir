# Late Registration

Services can be registered on an already-built `ServiceProvider`, not only
on `ServiceCollection` before `CreateServiceProvider()`. That enables
plugin-style workflows: the host builds the container once, then attached
modules call `Add*` / `Remove` at runtime.

Stable `GetServiceId<T>()` values (derived from `refl::type_name<T>()`)
keep the same dense id for a type across static libs and DSOs in the same
process, which is required for host and plugin to agree on cache keys.

## Registering After Build

`ServiceProvider` exposes the same lifetime APIs as `ServiceCollection`
(`AddSingleton`, `AddTransient`, `AddScoped`), including factory, instance,
contract/implementation, and constructor-injection overloads:

```cpp
auto sp = skr::ServiceCollection()
              .AddSingleton<HostServices>()
              .CreateServiceProvider();

// Plugin attach
sp->AddSingleton<GameplaySystem>();
sp->AddTransient<IEnemyFactory, GoblinFactory>();
sp->AddScoped<RequestContext>();

auto gameplay = sp->GetService<GameplaySystem>();
```

New registrations share the same definition map as the root provider and
any live scopes created from it. A scoped service added after
`CreateServiceScope()` is still resolvable from that existing scope.

### Factories and Instances

```cpp
sp->AddSingleton<IInput>([](skr::ServiceProvider& p) {
    return skr::MakeArc<PluginInput>(p.GetService<Window>());
});

sp->AddSingleton<IAudio>(existingAudioArc);
```

## Unload / Remove

`Remove<T>()` drops **every** registration for `T` and evicts that id from:

- the singleton cache
- the keyed-singleton cache (all keys for that id)
- the root scoped cache
- every **live** scope cache tracked by the provider

```cpp
// Plugin detach — do this before dlclose on code that owns factories /
// vtables for T
sp->Remove<GameplaySystem>();

EXPECT_FALSE(sp->Contains<GameplaySystem>());
EXPECT_FALSE(sp->TryGetService<GameplaySystem>().has_value());
```

Returns `true` if at least one registration was removed.

To replace a service, remove first, then add again. A later
`AddSingleton<T>()` alone does **not** invalidate an instance already
cached under `T` from an earlier registration (first-wins singleton
cache).

## Plugin Sketch

```cpp
// Host
auto sp = builder.Build()->GetRootServiceProvider();
plugin->on_attach(sp.get());

// Plugin
void on_attach(skr::ServiceProvider* sp) {
    sp->AddSingleton<GameplaySystem>();
    // Prefer resolving GameplaySystem here (or handing Arc to Freyr)
    // instead of expecting the host to GetService<GameplaySystem>().
}

void on_detach(skr::ServiceProvider* sp) {
    sp->Remove<GameplaySystem>();
}
```

Keep any `Arc` to plugin types released (or only held by the container)
before `Remove` + `dlclose`, so destructors still run against mapped code.

## Notes

- **ValidateOnBuild**: does not re-run automatically after late `Add*`.
  Call it again if you want eager checks for newly registered singletons
  and captive-dependency rules.
- **Multi-registration**: late `Add*` appends like collection registration;
  `GetService<T>()` still returns the first registration.
- **Loggers**: registering a service still auto-registers
  `Logger<T>` transients, matching `ServiceCollection` behavior.
- **Service ids**: identity is the stable type name, not registration
  order. Unique, stable `refl::type_name<T>()` strings are required across
  modules that share the container.
