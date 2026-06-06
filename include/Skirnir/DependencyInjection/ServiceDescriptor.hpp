#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Common/Ref.hpp"
#include "Skirnir/DependencyInjection/ServiceId.hpp"

namespace SKIRNIR_NAMESPACE
{
    class ServiceCollection;
    class ServiceProvider;
    class ServiceScope;

    using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;

    struct ServiceDescription
    {
        ServiceId        id;
        std::string_view name;

        bool operator<(const ServiceDescription& other) const
        {
            return id < other.id;
        }

        bool operator==(const ServiceDescription& other) const
        {
            return id == other.id;
        }

        bool operator!=(const ServiceDescription& other) const
        {
            return id != other.id;
        }
    };

    using InternalServiceFactory = std::function<Ref<void>(
        ServiceProvider&, std::set<ServiceDescription>&)>;

    struct ServiceDefinition
    {
        std::function<Ref<void>(
            ServiceProvider&, std::set<ServiceDescription>&)>
                                    factory  = nullptr;
        LifeTime                   lifetime = LifeTime::Transient;
        std::string                key;
        std::vector<ServiceId>     ctorDeps;
    };

    using ServiceDefinitionMap = std::multimap<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;
    using KeyedServicesCache   = std::map<std::pair<ServiceId, std::string>,
                                         Ref<void>>;
} // namespace SKIRNIR_NAMESPACE
