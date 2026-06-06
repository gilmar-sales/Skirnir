# Keyed / Named Services

When a single contract has multiple implementations, Skirnir lets you
distinguish them by a string key. The container can then resolve the
correct one either explicitly (by calling `GetKeyedService<T>(key)`) or
implicitly (by declaring a `Keyed<T, "key">` constructor parameter).

## Registering keyed services

Use the `AddKeyed*` overloads on `ServiceCollection`. They mirror the
un-keyed `AddSingleton` / `AddScoped` / `AddTransient` family:

```cpp
sc.AddKeyedSingleton<IFoo, FooA>("a");
sc.AddKeyedSingleton<IFoo, FooB>("b");
```

A key can be any `std::string`. Empty keys are reserved for
non-keyed registrations.

## Resolving by key

`ServiceProvider::GetKeyedService<T>(key)` returns the unique
registration whose contract is `T` and key matches. It throws
`std::runtime_error` if no such registration exists. The
non-throwing variant `TryGetKeyedService<T>(key)` returns
`std::optional<Arc<T>>` instead.

```cpp
auto a = sp->GetKeyedService<IFoo>("a");
auto b = sp->GetKeyedService<IFoo>("b");

auto maybe = sp->TryGetKeyedService<IFoo>("c");
if (maybe) { /* use *maybe */ }
```

## Injecting keyed services

Declare a constructor parameter of type `Keyed<T, "key">`. The
container resolves the keyed registration and fills in the wrapper's
`ptr` field for you.

```cpp
inline constexpr char keyA[] = "a";

class Consumer {
public:
    explicit Consumer(skr::Keyed<IFoo, keyA> k) : mFoo(k.ptr) {}
    // ...
};
```

The key must be a constant expression pointing to a static character
array. Wrap your keys in `inline constexpr char name[] = "...";` at
namespace scope and reference them in the `Keyed<...>` template
argument.

> **Note:** The first template argument of `Keyed` is the *contract*
> type, the second is the NTTP key. The wrapper's `ptr` is a
> `Arc<T>` set by the container.

## Multi-registration interaction

Keyed and un-keyed registrations share the same contract id.
`GetService<T>()` returns the *first* registration (matching .NET
semantics). To pick a specific implementation, always use
`GetKeyedService` or the `Keyed<T, "...">` ctor wrapper.
