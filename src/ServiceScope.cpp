#include "Skirnir/ServiceScope.hpp"
#include "Skirnir/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{

    ServiceScope::ServiceScope(
        const std::shared_ptr<ServiceDefinitionMap>& serviceDefinitionMap,
        const std::shared_ptr<ServicesCache>&        singletonsCache) :
        mServiceDefinitionMap(serviceDefinitionMap),
        mSingletonsCache(singletonsCache), mScopeCache(MakeRef<ServicesCache>())
    {
        mServiceProvider = MakeRef<ServiceProvider>(
            mServiceDefinitionMap,
            mSingletonsCache,
            mScopeCache,
            true);
    }

} // namespace SKIRNIR_NAMESPACE