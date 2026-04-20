#pragma once
#include "Common.hpp"

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
         */
        ServiceScope(const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
                     const Ref<ServicesCache>&        singletonsCache);

        /**
         * @brief Gets the ServiceProvider for this scope.
         *
         * @return The scoped service provider
         */
        Ref<skr::ServiceProvider> GetServiceProvider() const
        {
            return mServiceProvider;
        };

      private:
        Ref<skr::ServiceProvider> mServiceProvider;
        Ref<ServiceDefinitionMap> mServiceDefinitionMap;
        Ref<ServicesCache>        mSingletonsCache;
        Ref<ServicesCache>        mScopeCache;
    };

} // namespace SKIRNIR_NAMESPACE
