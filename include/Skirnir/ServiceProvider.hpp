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
        /**
         * @brief Constructs a ServiceProvider.
         *
         * @param serviceDefinitionMap Map of service definitions
         * @param singletonsCache      Cache for singleton instances
         * @param scopedsCache         Cache for scoped instances
         * @param isScoped             Whether this provider is for a scope
         */
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

        /**
         * @brief Resolves a service of the specified type.
         *
         * @tparam TService The service type to resolve
         * @return          A shared_ptr to the service instance, or nullptr on
         * error
         */
        template <typename TService>
        Ref<TService> GetService()
        {
            auto serviceIds = std::set<ServiceDescription>();
            return GetServiceImpl<TService>(serviceIds);
        }

        /**
         * @brief Checks whether a service type is registered.
         *
         * @tparam TService The service type to check
         * @return          True if registered, false otherwise
         */
        template <typename TService>
        [[nodiscard]] bool Contains() const
        {
            return mServiceDefinitionMap->contains(GetServiceId<TService>());
        }

        /**
         * @brief Creates a new ServiceScope for scoped service resolution.
         *
         * @return A new ServiceScope instance bound to this provider
         */
        Ref<ServiceScope> CreateServiceScope() const;

      private:
        /**
         * @brief Internal implementation of service resolution.
         *
         * Handles transient, singleton, and scoped service lifecycle.
         * Detects circular dependencies and validates service registration.
         *
         * @tparam TService             The service type to resolve
         * @param servicesDescriptions  Set tracking resolution chain for cycle
         * detection
         * @return                      A shared_ptr to the resolved service
         *
         * @throws std::runtime_error  On circular dependency or unregistered
         * service
         *
         * @note  For scoped services, must be called from a scoped provider
         * @note  The servicesDescriptions set is modified during resolution and
         *        cleaned up on singleton retrieval to prevent false cycle
         * detection
         */
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
