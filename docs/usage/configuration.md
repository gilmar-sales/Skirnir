# Configuration

Skirnir supports JSON-based configuration that can be used to configure various aspects of the application, including logging levels per namespace.

## Configuration Files

Configuration is stored in JSON format. You can load configuration from files or strings.

### Basic Structure

```json
{
    "logging": {
        "logLevel": {
            "default": "Information",
            "MyApp": "Debug"
        }
    }
}
```

### Logger Configuration

The `logging.logLevel` section allows you to configure:

- **default**: Default log level for all namespaces (Debug, Trace, Information, Warning, Error, Fatal, None)
- **`<namespace>`**: Per-namespace log level overrides. Keys are matched against the C++ namespace of the type (e.g. `my_app::services`), with `::` as the separator.

#### Log Levels

| Level       | Description                                    |
|-------------|------------------------------------------------|
| Debug       | Debug messages and above                       |
| Trace       | Trace messages and above                       |
| Information | Informational messages and above               |
| Warning     | Warning messages and above                     |
| Error       | Error messages and above                       |
| Fatal       | Fatal errors only                              |
| None        | No logging                                     |

## Usage

### Loading Configuration from JSON String

```cpp
#include <Skirnir/Configuration.hpp>

const char* configJson = R"({
    "logging": {
        "logLevel": {
            "default": "Information",
            "MyApp": "Debug",
            "MyApp::Services": "Warning"
        }
    }
})";

auto config = skr::ConfigurationBuilder()
                  .AddJsonString(configJson)
                  .Build();
```

### Loading Configuration from File

```cpp
auto config = skr::ConfigurationBuilder()
                  .AddJsonFile("config.json")
                  .Build();
```

Multiple sources may be chained; later sources are appended to earlier ones and can override keys.

### Configuring LoggerOptions

```cpp
#include <Skirnir/Logger.hpp>

auto options = skr::MakeRef<skr::LoggerOptions>();
options->ConfigureFrom(config);
```

`ConfigureFrom` reads the default log level from `logging.logLevel.default` and registers every sibling key under `logging.logLevel` as a namespace-specific level.

### Using Configuration Values

```cpp
// Check if a key exists
if (config->HasKey("logging.logLevel.default"))
{
    // Get a value
    auto value = config->GetValue("logging.logLevel.default");
}

// Access nested values using dot notation
auto namespacedLevel = config->GetValue("logging.logLevel.MyApp");
```

## Namespace Hierarchy

Log levels can be configured hierarchically. When looking up a log level for a type, the type's full namespace (e.g. `my_app::services::DataBase`) is matched against the configured keys as follows:

1. Exact namespace match is checked first.
2. If no exact match, progressively shorter namespace prefixes are tried (e.g. `my_app::services::DataBase` → `my_app::services` → `my_app`).
3. If nothing matches, the `default` level is used.

Example:

```json
{
    "logging": {
        "logLevel": {
            "default": "Error",
            "my_app::services": "Debug"
        }
    }
}
```

- `my_app` → Error (default)
- `my_app::services` → Debug
- `my_app::services::DataBase` → Debug (inherited from the `my_app::services` prefix)
