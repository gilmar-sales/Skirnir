# Configuration

Skirnir supports JSON-based configuration that can be used to configure various aspects of the application, including logging levels per namespace.

## Configuration Files

Configuration is stored in JSON format. You can load configuration from files or strings.

### Basic Structure

```json
{
    "logging": {
        "level": "Information",
        "namespaces": {
            "MyApp": "Debug",
            "MyApp.Services": "Warning"
        }
    }
}
```

### Logger Configuration

The `logging` section allows you to configure:

- **level**: Default log level for all namespaces (Debug, Trace, Information, Warning, Error, Fatal, None)
- **namespaces**: Per-namespace log level overrides

#### Log Levels

| Level       | Description                                    |
|-------------|------------------------------------------------|
| Debug       | Debug messages and above                       |
| Trace       | Trace messages and above                       |
| Information | Informational messages and above              |
| Warning     | Warning messages and above                      |
| Error       | Error messages and above                       |
| Fatal       | Fatal errors only                              |
| None        | No logging                                     |

## Usage

### Loading Configuration from JSON String

```cpp
#include <Skirnir/Configuration.hpp>

const char* configJson = R"({
    "logging": {
        "level": "Information",
        "namespaces": {
            "MyApp": "Debug",
            "MyApp.Services": "Warning"
        }
    }
})";

auto config = skr::ConfigurationBuilder()
                  .AddJsonString(configJson)
                  ->Build();
```

### Loading Configuration from File

```cpp
auto config = skr::ConfigurationBuilder()
                  .AddJsonFile("config.json")
                  ->Build();
```

### Configuring LoggerOptions

```cpp
#include <Skirnir/Logger.hpp>

services.AddSingleton<skr::LoggerOptions>(
    [config](skr::ServiceProvider&) {
        auto options = skr::MakeRef<skr::LoggerOptions>();
        options->ConfigureFrom(config);
        return options;
    });
```

### Using Configuration Values

```cpp
// Check if a key exists
if (config->HasKey("logging.level"))
{
    // Get a value
    auto value = config->GetValue("logging.level");
}

// Access nested values using dot notation
auto namespacedLevel = config->GetValue("logging.namespaces.MyApp");
```

## Namespace Hierarchy

Log levels can be configured hierarchically. When looking up a log level for a type:

1. First, exact namespace match is checked
2. If no exact match, parent namespace matches are checked (e.g., `MyApp.Services.Database` matches `MyApp.Services` if configured)

Example:

```json
{
    "logging": {
        "level": "Error",
        "namespaces": {
            "MyApp.Services": "Debug"
        }
    }
}
```

- `MyApp` → Error (default)
- `MyApp.Services` → Debug
- `MyApp.Services.Database` → Debug (inherited from parent)