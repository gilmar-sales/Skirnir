#pragma once

#include <cassert>
#include <source_location>

#include "Common.hpp"
#include "Logger.hpp"
#include "ServiceId.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceProvider : public std::enable_shared_from_this<ServiceProvider>
    {
      public:
        explicit ServiceProvider(
            const std::shared_ptr<ServiceDefinitionMap>& serviceDefinitionMap,
            const std::shared_ptr<ServicesCache>&        singletonsCache =
                std::make_shared<ServicesCache>(),
            const std::shared_ptr<ServicesCache>& scopedsCache =
                std::make_shared<ServicesCache>(),
            const bool isScoped = false) :
            mIsScoped(isScoped), mServiceDefinitionMap(serviceDefinitionMap),
            mSingletonsCache(singletonsCache), mScopeCache(scopedsCache)
        {
            mLogger = GetService<Logger<ServiceProvider>>();
        };

        template <typename TService>
        std::shared_ptr<TService> GetService()
        {
            if constexpr (std::is_same_v<TService, ServiceProvider>)
                return shared_from_this();

            mLogger->Assert(Contains<TService>(),
                            "Unable get unregistered service: '{}'",
                            type_name<TService>());

            switch (const auto& serviceDefinition =
                        mServiceDefinitionMap->at(GetServiceId<TService>());
                    serviceDefinition.lifetime)
            {
                case LifeTime::Transient: {
                    return std::static_pointer_cast<TService>(
                        mServiceDefinitionMap->at(GetServiceId<TService>())
                            .factory(*this));
                }
                case LifeTime::Singleton: {
                    const auto it =
                        mSingletonsCache->find(GetServiceId<TService>());

                    if (it == mSingletonsCache->end())
                    {
                        auto service = serviceDefinition.factory(*this);
                        mSingletonsCache->emplace(GetServiceId<TService>(),
                                                  service);

                        return std::static_pointer_cast<TService>(service);
                    }

                    return std::static_pointer_cast<TService>(
                        mSingletonsCache->at(GetServiceId<TService>()));
                }
                case LifeTime::Scoped: {

                    mLogger->Assert(
                        mIsScoped,
                        "Unable to get 'Scoped' {} service into Root Service "
                        "Provider. Create an scope first.",
                        type_name<TService>());

                    const auto it = mScopeCache->find(GetServiceId<TService>());

                    if (it == mScopeCache->end())
                    {
                        auto service = serviceDefinition.factory(*this);
                        mScopeCache->emplace(GetServiceId<TService>(), service);

                        return std::static_pointer_cast<TService>(service);
                    }

                    return std::static_pointer_cast<TService>(
                        mScopeCache->at(GetServiceId<TService>()));
                }
                default: {
                    return nullptr;
                }
            }
        }

        template <typename TService>
        [[nodiscard]] bool Contains() const
        {
            return mServiceDefinitionMap->contains(GetServiceId<TService>());
        }

        Ref<ServiceScope> CreateServiceScope();

      private:
        bool mIsScoped;

        Ref<Logger<ServiceProvider>> mLogger;
        Ref<ServiceDefinitionMap>    mServiceDefinitionMap;
        Ref<ServicesCache>           mSingletonsCache;
        Ref<ServicesCache>           mScopeCache;
    };

} // namespace SKIRNIR_NAMESPACE