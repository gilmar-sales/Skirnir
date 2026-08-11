#pragma once

#include <functional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Common/Keyed.hpp"
#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Common/Reflection.hpp"
#include "Skirnir/DependencyInjection/ServiceDescriptor.hpp"
#include "Skirnir/DependencyInjection/ServiceId.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"
#include "Skirnir/DependencyInjection/ServiceRegistration.hpp"
#include "Skirnir/Logging/Logger.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceCollection
    {
      public:
        ServiceCollection() :
            mServiceDefinitionMap(MakeArc<ServiceDefinitionMap>())
        {
            AddTransient<Logger<ServiceCollection>>();
            AddTransient<Logger<ServiceProvider>>();
            mLogger =
                MakeArc<Logger<ServiceCollection>>(MakeArc<LoggerOptions>());
        };

        ~ServiceCollection() = default;

        /**
         * @brief Registers a singleton service with a custom factory.
         *
         * @tparam TService The concrete service type to register
         * @param factory   Custom factory function for creating the service
         * @return         Reference to this ServiceCollection for chaining
         */
        template <typename TService>
        ServiceCollection& AddSingleton(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Singleton, factory);

            return *this;
        }

        /**
         * @brief Registers a singleton service with a custom factory.
         *
         * @tparam TContract The contract/interface type
         * @tparam TService  The concrete service type
         * @param factory    Custom factory function for creating the service
         * @return          Reference to this ServiceCollection for chaining
         */
        template <typename TContract, typename TService>
            requires(std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddSingleton(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Singleton, factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddSingleton()
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Singleton);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddSingleton()
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Singleton);
            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddSingleton()
        {
            service_registration::AddServiceWithConstructorArgs<TService,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Singleton);

            return *this;
        }

        template <typename TService>
        ServiceCollection& AddSingleton()
        {
            service_registration::AddService<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Singleton);

            return *this;
        }

        template <typename TService>
        ServiceCollection& AddSingleton(Arc<TService> element)
        {
            service_registration::AddServiceWithInstance<TService, TService>(
                *mServiceDefinitionMap, std::move(element),
                LifeTime::Singleton);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(not std::is_same_v<TContract, TService> and
                     std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddSingleton(Arc<TService> element)
        {
            service_registration::AddServiceWithInstance<TContract, TService>(
                *mServiceDefinitionMap, std::move(element),
                LifeTime::Singleton);

            return *this;
        }

        /**
         * @brief Registers a transient service with a custom factory.
         *
         * @tparam TService The concrete service type to register
         * @param factory   Custom factory function for creating the service
         * @return         Reference to this ServiceCollection for chaining
         */
        template <typename TService>
        ServiceCollection& AddTransient(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Transient, factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddTransient(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Transient, factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddTransient()
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Transient);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddTransient()
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Transient);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddTransient()
        {
            service_registration::AddServiceWithConstructorArgs<TService,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Transient);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddTransient()
        {
            service_registration::AddService<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Transient);

            return *this;
        }

        /**
         * @brief Registers a scoped service with a custom factory.
         *
         * @tparam TService The concrete service type to register
         * @param factory   Custom factory function for creating the service
         * @return         Reference to this ServiceCollection for chaining
         */
        template <typename TService>
        ServiceCollection& AddScoped(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Scoped, factory);

            return *this;
        }

        /**
         * @brief Registers a scoped service with a contract and custom factory.
         *
         * @tparam TContract The contract/interface type
         * @tparam TService  The concrete service type
         * @param factory    Custom factory function for creating the service
         * @return          Reference to this ServiceCollection for chaining
         */
        template <typename TContract, typename TService>
            requires(std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddScoped(const ServiceFactory& factory)
        {
            service_registration::AddServiceWithFactory<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Scoped, factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddScoped()
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Scoped);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddScoped()
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Scoped);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddScoped()
        {
            service_registration::AddServiceWithConstructorArgs<TService,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Scoped);
            return *this;
        }

        template <typename TService>
        ServiceCollection& AddScoped()
        {
            service_registration::AddService<TService, TService>(
                *mServiceDefinitionMap, LifeTime::Scoped);

            return *this;
        }

        // ----- Keyed / named services ----------------------------------

        /**
         * @brief Registers a keyed singleton service.
         *
         * Multiple implementations of @c TContract can be registered under
         * distinct string keys and resolved with @ref GetKeyedService or
         * by injecting @c Keyed<TContract, "key"> as a ctor parameter.
         */
        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddKeyedSingleton(std::string key)
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Singleton, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedSingleton(std::string key)
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Singleton, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddKeyedScoped(std::string key)
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Scoped, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedScoped(std::string key)
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Scoped, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddKeyedTransient(std::string key)
        {
            service_registration::AddServiceWithConstructorArgs<TContract,
                                                                TService>(
                *mServiceDefinitionMap, LifeTime::Transient, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedTransient(std::string key)
        {
            service_registration::AddService<TContract, TService>(
                *mServiceDefinitionMap, LifeTime::Transient, std::move(key));
            return *this;
        }

        /**
         * @brief Checks whether a service type is registered.
         *
         * @tparam TService The service type to check
         * @return          True if the service is registered, false otherwise
         */
        template <typename TService>
        [[nodiscard]] bool Contains() const
        {
            return mServiceDefinitionMap->contains(GetServiceId<TService>());
        }

        /**
         * @brief Creates a new ServiceProvider from this collection.
         *
         * Automatically registers LoggerOptions as a singleton if not already
         * present. The returned provider can resolve services from this
         * collection.
         *
         * @return A new ServiceProvider instance
         */
        [[nodiscard]] Arc<skr::ServiceProvider> CreateServiceProvider()
        {
            if (!Contains<LoggerOptions>())
            {
                AddSingleton<LoggerOptions>();
            }

            return MakeArc<ServiceProvider>(mServiceDefinitionMap);
        }

      private:
        Arc<Logger<ServiceCollection>> mLogger;
        Arc<ServiceDefinitionMap>      mServiceDefinitionMap;
    };

} // namespace SKIRNIR_NAMESPACE
