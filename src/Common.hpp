#pragma once
#include <functional>
#include <memory>

#include "ServiceId.hpp"

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

using ServiceDefinitionMap = std::unordered_map<ServideId, ServiceDefinition>;
using ServicesCache = std::unordered_map<ServideId, std::shared_ptr<void> >;