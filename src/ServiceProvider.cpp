#include "../include/ServiceProvider.hpp"

#include "../include/ServiceScope.hpp"

std::shared_ptr<ServiceScope> ServiceProvider::CreateServiceScope() {
    return std::make_shared<ServiceScope>(mServiceDefinitionMap, mSingletonsCache);
};