#pragma once

#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Common/ConstructorArgumentTraits.hpp"
#include "Skirnir/Common/Keyed.hpp"
#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Common/Reflection.hpp"
#include "Skirnir/DependencyInjection/Resolve.hpp"
#include "Skirnir/DependencyInjection/ServiceDescriptor.hpp"
#include "Skirnir/DependencyInjection/ServiceId.hpp"
#include "Skirnir/Logging/Logger.hpp"

namespace SKIRNIR_NAMESPACE::service_registration
{
    template <typename TService, typename... Args>
        requires(std::is_constructible_v<TService, Args...>)
    InternalServiceFactory CreateServiceFactory(std::tuple<Args...>)
    {
        return [](ServiceProvider&              serviceProvider,
                  std::set<ServiceDescription>& servicesDescriptions) {
            servicesDescriptions.erase(ServiceDescription {
                .id   = GetServiceId<TService>(),
                .name = refl::type_name<TService>() });

            return MakeArc<TService>(
                Resolve<Args>(serviceProvider, servicesDescriptions)...);
        };
    }

    template <typename TService>
    std::vector<ServiceId> ComputeCtorServiceIds();

    template <typename TService, typename Arg>
    void ComputeCtorServiceIdsImpl(Arg, std::vector<ServiceId>& ids)
    {
        if constexpr (is_vector_of_arc_v<Arg>)
        {
            using U = typename Arg::value_type::element_type;
            ids.push_back(GetServiceId<U>());
        }
        else if constexpr (is_optional_of_arc_v<Arg>)
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
    void ComputeCtorServiceIdsImpl(std::tuple<Args...>,
                                   std::vector<ServiceId>& ids)
    {
        (ComputeCtorServiceIdsImpl<TService>(Args {}, ids), ...);
    }

    template <typename TService>
    std::vector<ServiceId> ComputeCtorServiceIds()
    {
        std::vector<ServiceId> ids;
        ComputeCtorServiceIdsImpl<TService>(
            refl::first_ctor_params_tuple<TService> {},
            ids);
        return ids;
    }

    template <typename TContract, typename TService>
    void AddService(ServiceDefinitionMap& map, const LifeTime lifeTime,
                    std::string key = {});

    template <typename TContract, typename TService>
        requires(
            std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
    void AddServiceWithConstructorArgs(ServiceDefinitionMap& map,
                                       const LifeTime        lifeTime,
                                       std::string           key = {});

    template <typename TContract, typename TService>
    void AddServiceWithFactory(ServiceDefinitionMap& map,
                               const LifeTime        lifeTime,
                               const ServiceFactory& factory,
                               std::string           key = {});

    template <typename TContract, typename TService>
    void AddServiceWithInstance(ServiceDefinitionMap& map,
                                Arc<TService>         instance,
                                const LifeTime        lifeTime,
                                std::string           key = {});

    template <typename TService>
    void AddTransient(ServiceDefinitionMap& map)
    {
        if constexpr (std::tuple_size_v<
                          refl::first_ctor_params_tuple<TService>> > 0)
        {
            AddServiceWithConstructorArgs<TService, TService>(
                map, LifeTime::Transient);
        }
        else
        {
            AddService<TService, TService>(map, LifeTime::Transient);
        }
    }

    template <typename TContract, typename TService>
    void EnsureLoggers(ServiceDefinitionMap& map)
    {
        if constexpr (!std::is_base_of_v<ILogger, TContract>)
            AddTransient<Logger<TContract>>(map);

        if constexpr (!std::is_base_of_v<ILogger, TService> &&
                      !std::is_same_v<TContract, TService>)
            AddTransient<Logger<TService>>(map);
    }

    template <typename TContract, typename TService>
    void AddService(ServiceDefinitionMap& map, const LifeTime lifeTime,
                    std::string key)
    {
        map.insert({ GetServiceId<TContract>(),
                     { .factory =
                           [](ServiceProvider&, std::set<ServiceDescription>&) {
                               return MakeArc<TService>();
                           },
                       .lifetime = lifeTime,
                       .key      = std::move(key) } });

        EnsureLoggers<TContract, TService>(map);
    }

    template <typename TContract, typename TService>
        requires(
            std::tuple_size_v<refl::first_ctor_params_tuple<TService>> > 0)
    void AddServiceWithConstructorArgs(ServiceDefinitionMap& map,
                                       const LifeTime        lifeTime,
                                       std::string           key)
    {
        auto ctorDeps = ComputeCtorServiceIds<TService>();

        map.insert({ GetServiceId<TContract>(),
                     { .factory = CreateServiceFactory<TService>(
                           refl::first_ctor_params_tuple<TService> {}),
                       .lifetime = lifeTime,
                       .key      = std::move(key),
                       .ctorDeps = std::move(ctorDeps) } });

        EnsureLoggers<TContract, TService>(map);
    }

    template <typename TContract, typename TService>
    void AddServiceWithFactory(ServiceDefinitionMap& map,
                               const LifeTime        lifeTime,
                               const ServiceFactory& factory,
                               std::string           key)
    {
        map.insert(
            { GetServiceId<TContract>(),
              { .factory =
                    [newFactory = factory](
                        ServiceProvider&              serviceProvider,
                        std::set<ServiceDescription>& servicesDescriptions) {
                        servicesDescriptions.erase(ServiceDescription {
                            .id   = GetServiceId<TContract>(),
                            .name = refl::type_name<TContract>() });

                        return newFactory(serviceProvider);
                    },
                .lifetime = lifeTime,
                .key      = std::move(key) } });

        EnsureLoggers<TContract, TService>(map);
    }

    template <typename TContract, typename TService>
    void AddServiceWithInstance(ServiceDefinitionMap& map,
                                Arc<TService>         instance,
                                const LifeTime        lifeTime,
                                std::string           key)
    {
        map.insert(
            { GetServiceId<TContract>(),
              { .factory =
                    [instance = std::move(instance)](
                        ServiceProvider&,
                        std::set<ServiceDescription>& servicesDescriptions) {
                        servicesDescriptions.erase(ServiceDescription {
                            .id   = GetServiceId<TContract>(),
                            .name = refl::type_name<TContract>() });

                        return instance;
                    },
                .lifetime = lifeTime,
                .key      = std::move(key) } });

        EnsureLoggers<TContract, TService>(map);
    }
} // namespace SKIRNIR_NAMESPACE::service_registration
