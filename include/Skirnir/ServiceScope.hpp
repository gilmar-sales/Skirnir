#pragma once
#include "Common.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceScope
    {
      public:
        ServiceScope(const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
                     const Ref<ServicesCache>&        singletonsCache);

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
