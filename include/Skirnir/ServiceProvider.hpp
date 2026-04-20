#pragma once

#include "Common.hpp"
#include "Logger.hpp"
#include "ServiceId.hpp"

namespace SKIRNIR_NAMESPACE
{
    class IApplication;

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

        Ref<ServiceScope> CreateServiceScope() const;

      private:
        template <typename TService>
        Ref<TService> GetServiceImpl(
            std::set<ServiceDescription>& servicesDescriptions)
        {
            if constexpr (std::is_same_v<TService, ServiceProvider>)
                return shared_from_this();

            auto serviceDescription =
                ServiceDescription { .id   = GetServiceId<TService>(),
                                     .name = type_name<TService>() };

            if (servicesDescriptions.contains(serviceDescription) &&
                !mSingletonsCache->contains(GetServiceId<TService>()))
            {
                mLogger->LogFatal("Circular dependency detected between "
                                  "services: '{}' and '{}'",
                                  type_name<TService>(),
                                  servicesDescriptions.rbegin()->name);
            }

            servicesDescriptions.insert(serviceDescription);

            mLogger->Assert(Contains<TService>(),
                            "Unable get unregistered service: '{}'",
                            type_name<TService>());

            switch (const auto& serviceDefinition =
                        mServiceDefinitionMap->at(GetServiceId<TService>());
                    serviceDefinition.lifetime)
            {
                case LifeTime::Transient: {
                    auto service =
                        mServiceDefinitionMap->at(GetServiceId<TService>())
                            .factory(*this, servicesDescriptions);

                    return skr::RefCast<TService>(service);
                }
                case LifeTime::Singleton: {
                    const auto it =
                        mSingletonsCache->find(GetServiceId<TService>());

                    if (it == mSingletonsCache->end())
                    {
                        auto service =
                            serviceDefinition.factory(*this,
                                                      servicesDescriptions);

                        if constexpr (std::is_base_of_v<IApplication, TService>)
                        {
                            mApplication = skr::RefCast<IApplication>(service);

                            return skr::RefCast<TService>(service);
                        }

                        mSingletonsCache->emplace(GetServiceId<TService>(),
                                                  service);

                        return skr::RefCast<TService>(service);
                    }

                    if constexpr (std::is_base_of_v<IApplication, TService>)
                    {
                        return skr::RefCast<TService>(mApplication);
                    }

                    servicesDescriptions.erase(serviceDescription);

                    return skr::RefCast<TService>(
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
                            serviceDefinition.factory(*this,
                                                      servicesDescriptions);
                        mScopeCache->emplace(GetServiceId<TService>(), service);

                        return skr::RefCast<TService>(service);
                    }

                    servicesDescriptions.erase(serviceDescription);

                    return skr::RefCast<TService>(
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
        Ref<IApplication>            mApplication;
    };

} // namespace SKIRNIR_NAMESPACE
