#pragma once

#include <map>
#include <ostream>
#include <set>
#include <string_view>
#include <vector>

#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Common/Ref.hpp"
#include "Skirnir/Common/Reflection.hpp"
#include "Skirnir/DependencyInjection/ServiceDescriptor.hpp"
#include "Skirnir/DependencyInjection/ServiceId.hpp"
#include "Skirnir/Logging/Logger.hpp"


namespace SKIRNIR_NAMESPACE
{
    class IApplication;

    /**
     * @brief Resolves services from a @ref ServiceCollection.
     *
     * Supports both single (@ref GetService) and multi (@ref GetServices)
     * registration. For Singletons with multiple registrations, the first
     * registration wins (matching .NET behavior).
     */
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
            const Ref<KeyedServicesCache>& keyedSingletonsCache =
                MakeRef<KeyedServicesCache>(),
            const bool isScoped = false) :
            mIsScoped(isScoped), mServiceDefinitionMap(serviceDefinitionMap),
            mSingletonsCache(singletonsCache), mScopeCache(scopedsCache),
            mKeyedSingletonsCache(keyedSingletonsCache)
        {
            mLogger = GetService<Logger<ServiceProvider>>();
        };

        /**
         * @brief Resolves a service of the specified type.
         *
         * Throws @c std::runtime_error if the service is not registered.
         * For multi-registered types, returns the first registration.
         */
        template <typename TService>
        Ref<TService> GetService()
        {
            std::set<ServiceDescription> serviceIds;
            auto result = GetServiceImpl<TService>(serviceIds);
            if (result == nullptr)
            {
                mLogger->LogFatal("Unable to get unregistered service: '{}'",
                                  refl::type_name<TService>());
            }
            return result;
        }

        /**
         * @brief Resolves a service, returning @c std::nullopt if it is not
         * registered instead of throwing.
         */
        template <typename TService>
        std::optional<Ref<TService>> TryGetService()
        {
            std::set<ServiceDescription> serviceIds;
            auto result = GetServiceImplNoThrow<TService>(serviceIds);
            if (!result)
                return std::nullopt;
            return result;
        }

        /**
         * @brief Resolves the keyed registration of @c T for @c key.
         *
         * Throws if no registration matches. For unkeyed access, prefer
         * @ref GetService.
         */
        template <typename TService>
        Ref<TService> GetKeyedService(std::string_view key)
        {
            auto result = TryGetKeyedService<TService>(key);
            if (!result.has_value())
            {
                mLogger->LogFatal(
                    "Unable to get keyed service: '{}' with key '{}'",
                    refl::type_name<TService>(), key);
            }
            return *result;
        }

        /**
         * @brief Resolves the keyed registration of @c T for @c key, or
         * @c std::nullopt when no such registration exists.
         */
        template <typename TService>
        std::optional<Ref<TService>> TryGetKeyedService(std::string_view key)
        {
            std::set<ServiceDescription> serviceIds;
            auto                         range =
                mServiceDefinitionMap->equal_range(GetServiceId<TService>());

            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second.key == key)
                {
                    auto result =
                        GetServiceImplNoThrow<TService>(serviceIds, it->second);
                    if (result)
                        return result;
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Resolves all services registered for @p TService.
         *
         * For Singleton/Scoped lifetimes, the same instance is not returned
         * more than once (de-duplicated by pointer). Transient registrations
         * always produce new instances.
         */
        template <typename TService>
        std::vector<Ref<TService>> GetServices()
        {
            std::vector<Ref<TService>> results;
            auto serviceIds = std::set<ServiceDescription>();
            auto range =
                mServiceDefinitionMap->equal_range(GetServiceId<TService>());

            std::set<Ref<void>> seen;
            for (auto it = range.first; it != range.second; ++it)
            {
                auto service =
                    GetServiceImplNoThrow<TService>(serviceIds, it->second);
                if (service && seen.insert(service).second)
                {
                    results.push_back(std::move(service));
                }
            }
            return results;
        }

        /**
         * @brief Checks whether a service type is registered.
         */
        template <typename TService>
        [[nodiscard]] bool Contains() const
        {
            return mServiceDefinitionMap->contains(GetServiceId<TService>());
        }

        /**
         * @brief Creates a new ServiceScope for scoped service resolution.
         */
        Ref<ServiceScope> CreateServiceScope() const;

        /**
         * @brief Validates the service graph.
         *
         * For every Singleton registration, attempts to construct an
         * instance so missing transitive dependencies and unresolved ctors
         * surface eagerly. Aggregates failures and throws a single
         * @c std::runtime_error listing every missing service. Successfully
         * constructed singletons are stored in the cache so subsequent
         * @ref GetService calls are free.
         *
         * Also detects captive-dependency situations: a Singleton that
         * transitively depends on a Scoped service. Such configurations are
         * almost always a bug (the Scoped instance would live for the entire
         * process lifetime).
         */
        void ValidateOnBuild();

        /**
         * @brief Prints a diagnostic tree of registered services to @p os.
         *
         * Includes lifetime, contract name, registration count, and (when
         * available) the constructor parameter types of each registration.
         */
        void PrintDiagnostics(std::ostream& os) const;

      public:
        /**
         * @brief Internal implementation of service resolution.
         *
         * Throws @c std::runtime_error via @c LogFatal when the service is
         * not registered. Public so that @c Resolve<Arg> in @c Common.hpp
         * can dispatch to it; not intended for direct use by application
         * code.
         */
        template <typename TService>
        Ref<TService> GetServiceImpl(
            std::set<ServiceDescription>& servicesDescriptions)
        {
            if constexpr (std::is_same_v<TService, ServiceProvider>)
                return shared_from_this();

            auto serviceDescription =
                ServiceDescription { .id   = GetServiceId<TService>(),
                                     .name = refl::type_name<TService>() };

            if (servicesDescriptions.contains(serviceDescription) &&
                !mSingletonsCache->contains(GetServiceId<TService>()))
            {
                mLogger->LogFatal("Circular dependency detected between "
                                  "services: '{}' and '{}'",
                                  refl::type_name<TService>(),
                                  servicesDescriptions.rbegin()->name);
            }

            servicesDescriptions.insert(serviceDescription);

            // Resolve the first registration (for single GetService).
            auto range =
                mServiceDefinitionMap->equal_range(GetServiceId<TService>());
            if (range.first == range.second)
            {
                mLogger->LogFatal("Unable to get unregistered service: '{}'",
                                  refl::type_name<TService>());
            }
            const auto& serviceDefinition = range.first->second;

            return GetServiceImpl<TService>(servicesDescriptions,
                                            serviceDefinition);
        }

        /**
         * @brief Non-throwing resolution helper. Returns @c nullptr on miss
         * (missing registration, or Scoped service requested at the root
         * provider).
         */
        template <typename TService>
        Ref<TService> GetServiceImplNoThrow(
            std::set<ServiceDescription>& servicesDescriptions)
        {
            if constexpr (std::is_same_v<TService, ServiceProvider>)
                return shared_from_this();

            auto range =
                mServiceDefinitionMap->equal_range(GetServiceId<TService>());
            if (range.first == range.second)
                return nullptr;
            return GetServiceImplNoThrow<TService>(servicesDescriptions,
                                                   range.first->second);
        }

        template <typename TService>
        Ref<TService> GetServiceImplNoThrow(
            std::set<ServiceDescription>& servicesDescriptions,
            const ServiceDefinition&      serviceDefinition)
        {
            // Scoped at root: treat as "not available" for non-throwing
            // callers (TryGet). The throwing GetService() will surface this
            // as a nullptr and log-fatal.
            if (serviceDefinition.lifetime == LifeTime::Scoped && !mIsScoped)
                return nullptr;

            return GetServiceImpl<TService>(servicesDescriptions,
                                            serviceDefinition);
        }

        template <typename TService>
        Ref<TService> GetServiceImpl(
            std::set<ServiceDescription>& servicesDescriptions,
            const ServiceDefinition&      serviceDefinition)
        {
            switch (serviceDefinition.lifetime)
            {
                case LifeTime::Transient: {
                    auto service =
                        serviceDefinition.factory(*this, servicesDescriptions);
                    servicesDescriptions.erase(ServiceDescription {
                        .id   = GetServiceId<TService>(),
                        .name = refl::type_name<TService>() });
                    return skr::RefCast<TService>(service);
                }
                case LifeTime::Singleton: {
                    if (!serviceDefinition.key.empty())
                    {
                        // Keyed singleton: cache by (id, key) so distinct
                        // keys produce distinct instances.
                        const auto cacheKey = std::make_pair(
                            GetServiceId<TService>(), serviceDefinition.key);
                        const auto it = mKeyedSingletonsCache->find(cacheKey);

                        if (it == mKeyedSingletonsCache->end())
                        {
                            auto service = serviceDefinition.factory(
                                *this, servicesDescriptions);
                            mKeyedSingletonsCache->emplace(cacheKey, service);
                            servicesDescriptions.erase(ServiceDescription {
                                .id   = GetServiceId<TService>(),
                                .name = refl::type_name<TService>() });
                            return skr::RefCast<TService>(service);
                        }

                        servicesDescriptions.erase(ServiceDescription {
                            .id   = GetServiceId<TService>(),
                            .name = refl::type_name<TService>() });
                        return skr::RefCast<TService>(it->second);
                    }

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
                            servicesDescriptions.erase(ServiceDescription {
                                .id   = GetServiceId<TService>(),
                                .name = refl::type_name<TService>() });
                            return skr::RefCast<TService>(service);
                        }

                        mSingletonsCache->emplace(GetServiceId<TService>(),
                                                  service);

                        servicesDescriptions.erase(ServiceDescription {
                            .id   = GetServiceId<TService>(),
                            .name = refl::type_name<TService>() });
                        return skr::RefCast<TService>(service);
                    }

                    if constexpr (std::is_base_of_v<IApplication, TService>)
                    {
                        return skr::RefCast<TService>(mApplication.lock());
                    }

                    servicesDescriptions.erase(ServiceDescription {
                        .id   = GetServiceId<TService>(),
                        .name = refl::type_name<TService>() });

                    return skr::RefCast<TService>(
                        mSingletonsCache->at(GetServiceId<TService>()));
                }
                case LifeTime::Scoped: {

                    if (!mIsScoped)
                    {
                        mLogger->LogFatal(
                            "Unable to get 'Scoped' {} service into Root "
                            "Service Provider. Create an scope first.",
                            refl::type_name<TService>());
                    }

                    const auto it = mScopeCache->find(GetServiceId<TService>());

                    if (it == mScopeCache->end())
                    {
                        auto service =
                            serviceDefinition.factory(*this,
                                                      servicesDescriptions);
                        mScopeCache->emplace(GetServiceId<TService>(), service);
                        servicesDescriptions.erase(ServiceDescription {
                            .id   = GetServiceId<TService>(),
                            .name = refl::type_name<TService>() });
                        return skr::RefCast<TService>(service);
                    }

                    servicesDescriptions.erase(ServiceDescription {
                        .id   = GetServiceId<TService>(),
                        .name = refl::type_name<TService>() });

                    return skr::RefCast<TService>(
                        mScopeCache->at(GetServiceId<TService>()));
                }
            }
            return nullptr;
        }

        friend class ServiceCollection;

        bool mIsScoped;

        Ref<Logger<ServiceProvider>> mLogger;
        Ref<ServiceDefinitionMap>    mServiceDefinitionMap;
        Ref<ServicesCache>           mSingletonsCache;
        Ref<ServicesCache>           mScopeCache;
        Ref<KeyedServicesCache>      mKeyedSingletonsCache;
        WeakRef<IApplication>        mApplication;
    };

} // namespace SKIRNIR_NAMESPACE
