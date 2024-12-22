#pragma once

#include <cassert>

#include "Common.hpp"
#include "ServiceId.hpp"
#include "flat_map/flat_map.hpp"

class ServiceProvider {
public:
    explicit ServiceProvider(const std::shared_ptr<ServiceDefinitionMap> &serviceDefinitionMap,
                             const std::shared_ptr<ServicesCache> &singletonsCache = std::make_shared<ServicesCache>(),
                             const std::shared_ptr<ServicesCache> &scopedsCache = std::make_shared<ServicesCache>(),
                             const bool isScoped = false)
        : mIsScoped(isScoped), mServiceDefinitionMap(serviceDefinitionMap),
          mSingletonsCache(singletonsCache),
          mScopeCache(scopedsCache) {
    };

    template<typename TService>
    std::shared_ptr<TService> GetService() {
        assert(Contains<TService>() && "Unable to get unregistered type");

        switch (const auto &serviceDefinition = mServiceDefinitionMap->at(GetServiceId<TService>()); serviceDefinition.
            lifetime) {
            case LifeTime::Transient: {
                return std::static_pointer_cast<TService>(
                    mServiceDefinitionMap->at(GetServiceId<TService>()).factory(*this));
            }
            case LifeTime::Singleton: {
                const auto it = mSingletonsCache->find(GetServiceId<TService>());

                if (it == mSingletonsCache->end()) {
                    auto service = serviceDefinition.factory(*this);
                    mSingletonsCache->emplace(GetServiceId<TService>(), service);

                    return std::static_pointer_cast<TService>(service);
                }

                return std::static_pointer_cast<TService>(mSingletonsCache->at(GetServiceId<TService>()));
            }
            case LifeTime::Scoped: {
                assert(mIsScoped && "Unable to get scoped service into Root Service Provider, create an scope first.");

                const auto it = mScopeCache->find(GetServiceId<TService>());

                if (it == mScopeCache->end()) {
                    auto service = serviceDefinition.factory(*this);
                    mScopeCache->emplace(GetServiceId<TService>(), service);

                    return std::static_pointer_cast<TService>(service);
                }

                return std::static_pointer_cast<TService>(mScopeCache->at(GetServiceId<TService>()));
            }
            default: {
                return nullptr;
            }
        }
    }

    template<typename TService>
    [[nodiscard]] bool Contains() const {
        return mServiceDefinitionMap->contains(GetServiceId<TService>());
    }

    std::shared_ptr<ServiceScope> CreateServiceScope();

private:
    bool mIsScoped;

    std::shared_ptr<ServiceDefinitionMap> mServiceDefinitionMap;
    std::shared_ptr<ServicesCache> mSingletonsCache;
    std::shared_ptr<ServicesCache> mScopeCache;
};
