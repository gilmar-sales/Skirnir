# Configuration

Skirnir provides a strongly-typed JSON configuration system built on top of
[simdjson](https://github.com/simdjson/simdjson). Configuration is composed
from one or more _sources_ (files, strings, in-memory maps) that are merged
in order — later sources override earlier ones at the leaf level, with object
keys merging recursively.

## Quick Start

```cpp
#include <Skirnir/Configuration.hpp>

auto config = skr::ConfigurationBuilder()
                  .AddJsonFile("config.json")
                  .AddJsonString(R"({"logging": {"level": "Warning"}})")
                  .Build();

// Typed accessors
auto host = config->GetString("db.host", "localhost");
auto port = config->GetInt("db.port", 5432);
auto tags = config->GetArray("db.tags");

// Sub-section (owns its own parser; can outlive the parent)
auto db = config->GetSection("db");
auto dbHost = db->GetString("host");
```

## Sources

`IConfigurationSource` is the abstract base for configuration sources. The
builder accepts any number of them, merged in order:

| Method                       | Description                                |
| ---------------------------- | ------------------------------------------ |
| `AddJsonFile(path)`          | Load a JSON file.                          |
| `AddJsonString(text)`        | Parse a JSON string literal.               |
| `AddInMemory({ {"k","v"} })` | Flat key/value entries (dotted keys nest). |
| `AddSource(ref)`             | Register a custom `IConfigurationSource`.  |

### Chaining

```cpp
auto config = skr::ConfigurationBuilder()
                  .AddJsonString(R"({"db": {"host": "primary", "port": 5432}})")
                  .AddJsonString(R"({"db": {"port": 3306}})")
                  .Build();
config->GetString("db.host"); // "primary" (preserved)
config->GetInt("db.port");    // 3306     (overridden)
```

## Typed Accessors

All accessors take a dot-separated key path. Missing or type-mismatched values
return the supplied default.

| Method                    | Returns                                    |
| ------------------------- | ------------------------------------------ |
| `GetValue(key)`           | `std::optional<std::string>` (string form) |
| `HasKey(key)`             | `bool`                                     |
| `GetBool(key, default)`   | `bool`                                     |
| `GetInt(key, default)`    | `int64_t`                                  |
| `GetDouble(key, default)` | `double`                                   |
| `GetString(key, default)` | `std::string`                              |
| `GetArray(key)`           | `std::vector<std::string>`                 |
| `GetSection(key)`         | `Arc<ConfigurationOptions>`                |

## Strongly-Typed Binding

`Bind<T>(section = "")` produces a default-constructed `T` and populates its
fields from the JSON sub-object at `section`.

```cpp
struct DatabaseOptions
{
    std::string host = "localhost";
    int         port = 5432;
    bool        ssl  = false;
    double      timeout = 30.0;
};

auto db = config->Bind<DatabaseOptions>("database");
```

`JsonObjectReader` supports `bool`, integral, `double`, `std::string`,
`std::vector<std::string>`, and nested reflected structs. Missing or
type-mismatched values leave the destination untouched.

A complete example lives in `examples/options_binding/`.

## Iteration

`ForEachMember(section, fn)` invokes `fn(std::string_view key,
simdjson::dom::element value)` for each member of a sub-object, primarily for
extension authors (e.g. `LoggerOptions::ConfigureFrom` uses it to walk
namespace-level overrides).

## Logger Configuration

`LoggerOptions::ConfigureFrom(config, path)` reads the default log level from
`path` (default: `"logging.logLevel.default"`) and registers every sibling
key in the same object as a namespace-specific level.

```json
{
  "logging": {
    "logLevel": {
      "default": "Information",
      "my_app::services": "Debug"
    }
  }
}
```

## Build Option

Set `SKIRNIR_USE_SIMDJSON=OFF` to build without simdjson. In that mode the
strongly-typed binding and `GetSection` APIs are unavailable; the string
`GetValue`/`HasKey` API still works for basic use cases.
