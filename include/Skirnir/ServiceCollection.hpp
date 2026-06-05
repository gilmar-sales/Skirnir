#pragma once

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include "Common.hpp"
#include "Reflection.hpp"
#include "ServiceId.hpp"
#include "ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ServiceCollection
    {
      public:
        ServiceCollection() :
            mServiceDefinitionMap(MakeRef<ServiceDefinitionMap>())
        {
            AddTransient<Logger<ServiceCollection>>();
            AddTransient<Logger<ServiceProvider>>();
            mLogger =
                MakeRef<Logger<ServiceCollection>>(MakeRef<LoggerOptions>());
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
            AddServiceWithFactory<TService, TService>(LifeTime::Singleton,
                                                      factory);

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
            AddServiceWithFactory<TContract, TService>(LifeTime::Singleton,
                                                       factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddSingleton()
        {
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Singleton);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddSingleton()
        {
            AddService<TContract, TService>(LifeTime::Singleton);
            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddSingleton()
        {
            AddServiceWithConstructorArgs<TService, TService>(
                LifeTime::Singleton);

            return *this;
        }

        template <typename TService>
        ServiceCollection& AddSingleton()
        {
            AddService<TService, TService>(LifeTime::Singleton);

            return *this;
        }

        template <typename TService>
        ServiceCollection& AddSingleton(Ref<TService> element)
        {
            AddServiceWithInstance<TService, TService>(element,
                                                       LifeTime::Singleton);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(not std::is_same_v<TContract, TService> and
                     std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddSingleton(Ref<TService> element)
        {
            AddServiceWithInstance<TContract, TService>(element,
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
            AddServiceWithFactory<TService, TService>(LifeTime::Transient,
                                                      factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(std::is_base_of_v<TContract, TService>)
        ServiceCollection& AddTransient(const ServiceFactory& factory)
        {
            AddServiceWithFactory<TContract, TService>(LifeTime::Transient,
                                                       factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddTransient()
        {
            AddService<TContract, TService>(LifeTime::Transient);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddTransient()
        {
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Transient);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddTransient()
        {
            AddServiceWithConstructorArgs<TService, TService>(
                LifeTime::Transient);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddTransient()
        {
            AddService<TService, TService>(LifeTime::Transient);

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
            AddServiceWithFactory<TService, TService>(LifeTime::Scoped,
                                                      factory);

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
            AddServiceWithFactory<TContract, TService>(LifeTime::Scoped,
                                                       factory);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddScoped()
        {
            AddService<TContract, TService>(LifeTime::Scoped);

            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddScoped()
        {
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Scoped);

            return *this;
        }

        template <typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddScoped()
        {
            AddServiceWithConstructorArgs<TService, TService>(LifeTime::Scoped);

            return *this;
        }

        template <typename TService>
        ServiceCollection& AddScoped()
        {
            AddService<TService, TService>(LifeTime::Scoped);

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
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Singleton, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedSingleton(std::string key)
        {
            AddService<TContract, TService>(LifeTime::Singleton,
                                            std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddKeyedScoped(std::string key)
        {
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Scoped, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedScoped(std::string key)
        {
            AddService<TContract, TService>(LifeTime::Scoped, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        ServiceCollection& AddKeyedTransient(std::string key)
        {
            AddServiceWithConstructorArgs<TContract, TService>(
                LifeTime::Transient, std::move(key));
            return *this;
        }

        template <typename TContract, typename TService>
            requires(
                std::is_base_of_v<TContract, TService> &&
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> == 0)
        ServiceCollection& AddKeyedTransient(std::string key)
        {
            AddService<TContract, TService>(LifeTime::Transient,
                                            std::move(key));
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
        [[nodiscard]] Ref<skr::ServiceProvider> CreateServiceProvider()
        {
            if (!Contains<LoggerOptions>())
            {
                AddSingleton<LoggerOptions>();
            }

            return MakeRef<ServiceProvider>(mServiceDefinitionMap);
        }

      protected:
        template <typename TContract, typename TService>
        void AddService(const LifeTime lifeTime, std::string key = {})
        {
            mServiceDefinitionMap->insert(
                { GetServiceId<TContract>(),
                  { .factory =
                        [](ServiceProvider&, std::set<ServiceDescription>&) {
                            return MakeRef<TService>();
                        },
                    .lifetime = lifeTime,
                    .key      = std::move(key) } });

            if constexpr (!std::is_base_of_v<ILogger, TContract>)
                AddTransient<Logger<TContract>>();

            if constexpr (!std::is_base_of_v<ILogger, TService> &&
                          !std::is_same_v<TContract, TService>)
                AddTransient<Logger<TService>>();
        }

        template <typename TContract, typename TService>
            requires(
                std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
        void AddServiceWithConstructorArgs(const LifeTime lifeTime,
                                           std::string    key = {})
        {
            auto ctorDeps = ComputeCtorServiceIds<TService>();

            mServiceDefinitionMap->insert(
                { GetServiceId<TContract>(),
                  { .factory = CreateServiceFactory<TService>(
                        refl::first_ctor_params_tuple<TService> {}),
                    .lifetime = lifeTime,
                    .key      = std::move(key),
                    .ctorDeps = std::move(ctorDeps) } });

            if constexpr (!std::is_base_of_v<ILogger, TContract>)
                AddTransient<Logger<TContract>>();

            if constexpr (!std::is_base_of_v<ILogger, TService> &&
                          !std::is_same_v<TContract, TService>)
                AddTransient<Logger<TService>>();
        }

        template <typename TContract, typename TService>
        void AddServiceWithFactory(const LifeTime        lifeTime,
                                   const ServiceFactory& factory,
                                   std::string          key = {})
        {

            mServiceDefinitionMap->insert(
                { GetServiceId<TContract>(),
                  { .factory =
                        [newFactory = std::move(factory)](
                            ServiceProvider& serviceProvider,
                            std::set<ServiceDescription>&
                                servicesDescriptions) {
                            servicesDescriptions.erase(ServiceDescription {
                                .id   = GetServiceId<TContract>(),
                                .name = refl::type_name<TContract>() });

                            return newFactory(serviceProvider);
                        },
                    .lifetime = lifeTime,
                    .key      = std::move(key) } });

            if constexpr (!std::is_base_of_v<ILogger, TContract>)
                AddTransient<Logger<TContract>>();

            if constexpr (!std::is_base_of_v<ILogger, TService> &&
                          !std::is_same_v<TContract, TService>)
                AddTransient<Logger<TService>>();
        }

        template <typename TContract, typename TService>
        void AddServiceWithInstance(Ref<TService>  instance,
                                    const LifeTime lifeTime,
                                    std::string    key = {})
        {
            mServiceDefinitionMap->insert(
                { GetServiceId<TContract>(),
                  { .factory =
                        [instance = instance](ServiceProvider&,
                                              std::set<ServiceDescription>&
                                                  servicesDescriptions) {
                            servicesDescriptions.erase(ServiceDescription {
                                .id   = GetServiceId<TContract>(),
                                .name = refl::type_name<TContract>() });

                            return instance;
                        },
                    .lifetime = lifeTime,
                    .key      = std::move(key) } });

            if constexpr (!std::is_base_of_v<ILogger, TContract>)
                AddTransient<Logger<TContract>>();

            if constexpr (!std::is_base_of_v<ILogger, TService> &&
                          !std::is_same_v<TContract, TService>)
                AddTransient<Logger<TService>>();
        }

        template <typename TService, typename... Args>
            requires(std::is_constructible_v<TService, Args...>)
        InternalServiceFactory CreateServiceFactory(std::tuple<Args...>)
        {
            return [](ServiceProvider&              serviceProvider,
                      std::set<ServiceDescription>& servicesDescriptions) {
                servicesDescriptions.erase(ServiceDescription {
                    .id   = GetServiceId<TService>(),
                    .name = refl::type_name<TService>() });

                return MakeRef<TService>(
                    Resolve<Args>(serviceProvider, servicesDescriptions)...);
            };
        }

        /**
         * @brief Returns the list of @c ServiceId's that @c TService's
         *        first constructor depends on (for captive-dependency
         *        detection and diagnostics).
         */
        template <typename TService>
        static std::vector<ServiceId> ComputeCtorServiceIds()
        {
            std::vector<ServiceId> ids;
            ComputeCtorServiceIdsImpl<TService>(
                refl::first_ctor_params_tuple<TService> {}, ids);
            return ids;
        }

        template <typename TService, typename Arg>
        static void ComputeCtorServiceIdsImpl(Arg, std::vector<ServiceId>& ids)
        {
            if constexpr (is_vector_of_ref_v<Arg>)
            {
                using U = typename Arg::value_type::element_type;
                ids.push_back(GetServiceId<U>());
            }
            else if constexpr (is_optional_of_ref_v<Arg>)
            {
                using U = typename Arg::value_type::element_type;
                ids.push_back(GetServiceId<U>());
            }
            else if constexpr (is_keyed_v<Arg>)
            {
                using U = keyed_inner_t<Arg>;
                ids.push_back(GetServiceId<U>());
            }
            else
            {
                using U = typename Arg::element_type;
                ids.push_back(GetServiceId<U>());
            }
        }

        template <typename TService, typename... Args>
        static void ComputeCtorServiceIdsImpl(
            std::tuple<Args...>, std::vector<ServiceId>& ids)
        {
            (ComputeCtorServiceIdsImpl<TService>(Args {}, ids), ...);
        }

      private:
        Ref<Logger<ServiceCollection>> mLogger;
        Ref<ServiceDefinitionMap>      mServiceDefinitionMap;
    };

} // namespace SKIRNIR_NAMESPACE
