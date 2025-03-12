#include "Skirnir/ServiceProvider.hpp"
#include "Skirnir/ServiceScope.hpp"

namespace SKIRNIR_NAMESPACE
{

    Ref<ServiceScope> ServiceProvider::CreateServiceScope()
    {
        return MakeRef<ServiceScope>(mServiceDefinitionMap, mSingletonsCache);
    };

} // namespace SKIRNIR_NAMESPACE