#pragma once
#include <functional>
#include <memory>
#include <optional>

#include "ServiceId.hpp"
#include "flat_map/flat_map.hpp"

enum class LifeTime {
    Transient,
    Scoped,
    Singleton
};

class ServiceCollection;
class ServiceProvider;
class ServiceScope;

using ServiceFactory = std::function<std::shared_ptr<void>(ServiceProvider &)>;

struct ServiceDefinition {
    ServiceFactory factory = nullptr;
    LifeTime lifetime = LifeTime::Transient;
};

using ServiceDefinitionMap = flat_map::flat_map<ServideId, ServiceDefinition>;
using ServicesCache = flat_map::flat_map<ServideId, std::shared_ptr<void> >;