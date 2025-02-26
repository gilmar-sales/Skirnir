#pragma once
#include "Common.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceScope
    {
      public:
        ServiceScope(
            const std::shared_ptr<ServiceDefinitionMap>& serviceDefinitionMap,
            const std::shared_ptr<ServicesCache>&        singletonsCache);

        std::shared_ptr<ServiceProvider> GetServiceProvider()
        {
            return mServiceProvider;
        };

      private:
        std::shared_ptr<ServiceProvider>      mServiceProvider;
        std::shared_ptr<ServiceDefinitionMap> mServiceDefinitionMap;
        std::shared_ptr<ServicesCache>        mSingletonsCache;
        std::shared_ptr<ServicesCache>        mScopeCache;
    };

} // namespace SKIRNIR_NAMESPACE