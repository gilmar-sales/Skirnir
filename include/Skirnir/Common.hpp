#pragma once

#include <functional>
#include <map>
#include <memory>

#define SKIRNIR_NAMESPACE skr

#include "ServiceId.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

namespace SKIRNIR_NAMESPACE
{

    enum class LifeTime
    {
        Transient,
        Scoped,
        Singleton
    };

    class ServiceCollection;
    class ServiceProvider;
    class ServiceScope;

    using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;

    struct ServiceDefinition
    {
        ServiceFactory factory  = nullptr;
        LifeTime       lifetime = LifeTime::Transient;
    };

    using ServiceDefinitionMap = std::map<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;

} // namespace SKIRNIR_NAMESPACE