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
            const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
            const Ref<ServicesCache>&        singletonsCache =
                MakeRef<ServicesCache>(),
            const Ref<ServicesCache>& scopedsCache = MakeRef<ServicesCache>(),
            const bool                isScoped     = false) :
            mIsScoped(isScoped), mServiceDefinitionMap(serviceDefinitionMap),
            mSingletonsCache(singletonsCache), mScopeCache(scopedsCache)
        {
            mLogger = GetService<Logger<ServiceProvider>>();
        };

        template <typename TService>
        Ref<TService> GetService()
        {
            auto serviceIds = std::set<ServiceDescription>();
            return GetServiceImpl<TService>(serviceIds);
        }

        template <typename TService>
        [[nodiscard]] bool Contains() const
        {
            return mServiceDefinitionMap->contains(GetServiceId<TService>());
        }

        Ref<ServiceScope> CreateServiceScope();

      private:
        template <typename TService>
        Ref<TService> GetServiceImpl(std::set<ServiceDescription>& serviceIds)
        {
            if constexpr (std::is_same_v<TService, ServiceProvider>)
                return shared_from_this();

            if (serviceIds.contains(
                    ServiceDescription { .id = GetServiceId<TService>() }))
            {
                mLogger->LogFatal(
                    "Circular dependency detected for services: '{}' and '{}'",
                    type_name<TService>(),
                    serviceIds.rbegin()->name);
            }

            serviceIds.insert(ServiceDescription {
                .id   = GetServiceId<TService>(),
                .name = type_name<TService>() });

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
                            .factory(*this, serviceIds));
                }
                case LifeTime::Singleton: {
                    const auto it =
                        mSingletonsCache->find(GetServiceId<TService>());

                    if (it == mSingletonsCache->end())
                    {
                        auto service =
                            serviceDefinition.factory(*this, serviceIds);
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
                        auto service =
                            serviceDefinition.factory(*this, serviceIds);
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

        friend class ServiceCollection;

        bool mIsScoped;

        Ref<Logger<ServiceProvider>> mLogger;
        Ref<ServiceDefinitionMap>    mServiceDefinitionMap;
        Ref<ServicesCache>           mSingletonsCache;
        Ref<ServicesCache>           mScopeCache;
    };

} // namespace SKIRNIR_NAMESPACE