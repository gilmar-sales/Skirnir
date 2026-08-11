#pragma once

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceDescriptor.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceScope
    {
      public:
        /**
         * @brief Constructs a service scope.
         *
         * @param serviceDefinitionMap Map of service definitions
         * @param singletonsCache      Cache for singleton instances
         * @param keyedSingletonsCache Shared keyed-singleton cache
         * @param scopeCacheRegistry   Registry of live scoped caches
         * @param scopeCache           This scope's instance cache
         */
        ServiceScope(const Arc<ServiceDefinitionMap>& serviceDefinitionMap,
                     const Arc<ServicesCache>&        singletonsCache,
                     const Arc<KeyedServicesCache>&   keyedSingletonsCache,
                     const Arc<ScopeCacheRegistry>&   scopeCacheRegistry,
                     const Arc<ServicesCache>&        scopeCache);

        /**
         * @brief Gets the ServiceProvider for this scope.
         *
         * @return The scoped service provider
         */
        Arc<skr::ServiceProvider> GetServiceProvider() const
        {
            return mServiceProvider;
        };

      private:
        Arc<skr::ServiceProvider> mServiceProvider;
        Arc<ServiceDefinitionMap> mServiceDefinitionMap;
        Arc<ServicesCache>        mSingletonsCache;
        Arc<ServicesCache>        mScopeCache;
        Arc<KeyedServicesCache>   mKeyedSingletonsCache;
        Arc<ScopeCacheRegistry>   mScopeCacheRegistry;
    };

} // namespace SKIRNIR_NAMESPACE
