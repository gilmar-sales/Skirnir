# API Reference

## ServiceCollection

### Lifetime Registration Methods

- `AddSingleton<TService>()` - Register a singleton service by concrete type
- `AddSingleton<TContract, TService>()` - Register a singleton with contract
- `AddSingleton<TService>(factory)` - Register with factory function
- `AddScoped<TService>()` - Register a scoped service
- `AddScoped<TContract, TService>()` - Register a scoped service with contract
- `AddScoped<TService>(factory)` - Register with factory function
- `AddTransient<TService>()` - Register a transient service
- `AddTransient<TContract, TService>()` - Register a transient with contract
- `AddTransient<TService>(factory)` - Register with factory function

### Utility Methods

- `Contains<TService>()` - Check if a service is registered
- `CreateServiceProvider()` - Build a ServiceProvider from the collection

## ServiceProvider

- `GetService<TService>()` - Get an instance of the requested service
- `Contains<TService>()` - Check if a service is registered
- `CreateServiceScope()` - Create a new scope for scoped services

## ServiceScope

- `GetServiceProvider()` - Get the ServiceProvider for this scope

## Logger

- `LogDebug(fmt, args...)` - Log a debug message
- `LogTrace(fmt, args...)` - Log a trace message
- `LogInformation(fmt, args...)` - Log an info message
- `LogWarning(fmt, args...)` - Log a warning message
- `LogError(fmt, args...)` - Log an error message
- `LogFatal(fmt, args...)` - Log a fatal message and throw exception
- `Assert(condition, fmt, args...)` - Assert with message (debug only)