#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceId.hpp"

namespace SKIRNIR_NAMESPACE
{
    class ServiceCollection;
    class ServiceProvider;
    class ServiceScope;

    using ServiceFactory = std::function<Arc<void>(ServiceProvider&)>;

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

    using InternalServiceFactory = std::function<Arc<void>(
        ServiceProvider&, std::set<ServiceDescription>&)>;

    struct ServiceDefinition
    {
        std::function<Arc<void>(
            ServiceProvider&, std::set<ServiceDescription>&)>
                                    factory  = nullptr;
        LifeTime                   lifetime = LifeTime::Transient;
        std::string                key;
        std::vector<ServiceId>     ctorDeps;
    };

    using ServiceDefinitionMap = std::multimap<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Arc<void>>;
    using KeyedServicesCache   = std::map<std::pair<ServiceId, std::string>,
                                         Arc<void>>;

    /**
     * @brief Tracks live scoped-instance caches so @c Remove can evict
     *        entries from scopes that outlive a late-unregistered service.
     */
    struct ScopeCacheRegistry
    {
        void Track(const Arc<ServicesCache>& cache)
        {
            std::lock_guard lock(mutex);
            std::erase_if(caches, [](const WeakArc<ServicesCache>& weak) {
                return weak.expired();
            });
            caches.push_back(cache);
        }

        void EraseService(ServiceId id)
        {
            std::lock_guard lock(mutex);
            std::erase_if(caches, [id](WeakArc<ServicesCache>& weak) {
                if (auto cache = weak.lock())
                {
                    cache->erase(id);
                    return false;
                }
                return true;
            });
        }

        std::mutex                          mutex;
        std::vector<WeakArc<ServicesCache>> caches;
    };
} // namespace SKIRNIR_NAMESPACE
