#pragma once
#include <functional>
#include <memory>

#include <tsl/bhopscotch_map.h>

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

using ServiceDefinitionMap = tsl::bhopscotch_map<ServideId, ServiceDefinition>;
using ServicesCache = tsl::bhopscotch_map<ServideId, std::shared_ptr<void> >;