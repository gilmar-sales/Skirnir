#include "ServiceScope.hpp"

#include "ServiceProvider.hpp"

ServiceScope::ServiceScope(const std::shared_ptr<ServiceDefinitionMap> &serviceDefinitionMap,
                           const std::shared_ptr<ServicesCache> &singletonsCache)
    : mServiceDefinitionMap(serviceDefinitionMap),
      mSingletonsCache(singletonsCache),
      mScopeCache(std::make_shared<ServicesCache>()) {
    mServiceProvider = std::make_shared<ServiceProvider>(mServiceDefinitionMap, mSingletonsCache, mScopeCache, true);
}
