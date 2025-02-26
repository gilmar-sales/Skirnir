#include "Skirnir/ServiceProvider.hpp"
#include "Skirnir/ServiceScope.hpp"

namespace SKIRNIR_NAMESPACE
{

    std::shared_ptr<ServiceScope> ServiceProvider::CreateServiceScope()
    {
        return std::make_shared<ServiceScope>(
            mServiceDefinitionMap, mSingletonsCache);
    };

} // namespace SKIRNIR_NAMESPACE