# Optional Dependencies

A constructor parameter of type `std::optional<Ref<T>>` is treated as
an *optional* dependency: the container resolves the service when it is
registered, and yields `std::nullopt` when it isn't. No exception, no
log-fatal.

## Example

```cpp
class Consumer {
public:
    explicit Consumer(std::optional<Ref<ILogger>> logger)
        : mLogger(std::move(logger)) {}

    std::string Tag() const {
        return mLogger ? (*mLogger)->Tag() : "no-logger";
    }

private:
    std::optional<Ref<ILogger>> mLogger;
};

// Register only the consumer, not the logger:
auto sp = skr::ServiceCollection()
              .AddSingleton<Consumer>()
              .CreateServiceProvider();
auto c = sp->GetService<Consumer>();
// c->Tag() == "no-logger"

// Now register the logger:
sc.AddSingleton<ILogger, ConsoleLogger>();
// c->Tag() == "console"
```

## Interaction with lifetimes

Optional dependencies work for any lifetime (Transient, Scoped,
Singleton). If a `Scoped` optional is resolved at the root provider
the resolution yields `std::nullopt` (the same behavior as
`TryGetService<Scoped>()` at the root).

## Limitations

- Only `std::optional<Ref<T>>` is supported. `std::optional<Ref<T>>`
  inside a vector, or `std::optional<Keyed<T, "k">>`, are not
  special-cased.
- The optional must be declared exactly as `std::optional<Ref<T>>`;
  aliases to that form are not detected by the trait used to
  dispatch resolution.
