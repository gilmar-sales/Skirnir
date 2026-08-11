#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceScope.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{

    ServiceScope::ServiceScope(
        const Arc<ServiceDefinitionMap>& serviceDefinitionMap,
        const Arc<ServicesCache>&        singletonsCache,
        const Arc<KeyedServicesCache>&   keyedSingletonsCache,
        const Arc<ScopeCacheRegistry>&   scopeCacheRegistry,
        const Arc<ServicesCache>&        scopeCache) :
        mServiceDefinitionMap(serviceDefinitionMap),
        mSingletonsCache(singletonsCache), mScopeCache(scopeCache),
        mKeyedSingletonsCache(keyedSingletonsCache),
        mScopeCacheRegistry(scopeCacheRegistry)
    {
        mServiceProvider = MakeArc<ServiceProvider>(
            mServiceDefinitionMap,
            mSingletonsCache,
            mScopeCache,
            mKeyedSingletonsCache,
            mScopeCacheRegistry,
            true);
    }

} // namespace SKIRNIR_NAMESPACE
