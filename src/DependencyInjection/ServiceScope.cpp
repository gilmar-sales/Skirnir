#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceScope.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{

    ServiceScope::ServiceScope(
        const Arc<ServiceDefinitionMap>& serviceDefinitionMap,
        const Arc<ServicesCache>&        singletonsCache) :
        mServiceDefinitionMap(serviceDefinitionMap),
        mSingletonsCache(singletonsCache), mScopeCache(MakeArc<ServicesCache>())
    {
        mServiceProvider = MakeArc<ServiceProvider>(
            mServiceDefinitionMap,
            mSingletonsCache,
            mScopeCache,
            MakeArc<KeyedServicesCache>(),
            true);
    }

} // namespace SKIRNIR_NAMESPACE
