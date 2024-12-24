#pragma once

#include <any>
#include <functional>
#include <memory>
#include <cassert>

#include "Reflection.hpp"
#include "ServiceId.hpp"
#include "ServiceProvider.hpp"
#include "Common.hpp"

class ServiceCollection {
public:
    ServiceCollection()
        : mServiceDefinitionMap(std::make_shared<ServiceDefinitionMap>()) {
    };

    ~ServiceCollection() = default;

    template<typename TService>
        requires(std::tuple_size_v<refl::as_tuple<TService> > > 0)
    ServiceCollection &AddSingleton() {
        AddServiceWithConstructorArgs<TService, TService>(LifeTime::Singleton);


        return *this;
    }

    template<typename TService>
    ServiceCollection &AddSingleton() {
        AddService<TService, TService>(LifeTime::Singleton);


        return *this;
    }

    template<typename TService>
    ServiceCollection &AddSingleton(std::shared_ptr<TService> element) {
        AddServiceWithInstance<TService, TService>(element, LifeTime::Singleton);


        return *this;
    }

    template<typename TContract, typename TService>
        requires(not std::is_same_v<TContract, TService> and std::is_base_of_v<TContract, TService>)
    ServiceCollection &AddSingleton(std::shared_ptr<TService> element) {
        AddServiceWithInstance<TContract, TService>(element, LifeTime::Singleton);


        return *this;
    }

    template<typename TService>
    ServiceCollection &AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<TService, TService>(LifeTime::Transient, factory);


        return *this;
    }

    template<typename TContract, typename TService>
        requires(std::is_base_of_v<TContract, TService>)
    ServiceCollection &AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<TContract, TService>(LifeTime::Transient, factory);


        return *this;
    }

    template<typename TContract, typename TService>
        requires(std::is_base_of_v<TContract, TService>)
    ServiceCollection &AddTransient() {
        AddService<TContract, TService>(LifeTime::Transient);


        return *this;
    }

    template<typename TService>
        requires(std::tuple_size_v<refl::as_tuple<TService> > > 0)
    ServiceCollection &AddTransient() {
        AddServiceWithConstructorArgs<TService, TService>(LifeTime::Transient);


        return *this;
    }

    template<typename TService>
    ServiceCollection &AddTransient() {
        AddService<TService, TService>(LifeTime::Transient);


        return *this;
    }

    template<typename TService>
    ServiceCollection &AddScoped(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<TService, TService>(LifeTime::Scoped, factory);


        return *this;
    }

    template<typename TContract, typename TService>
        requires(std::is_base_of_v<TContract, TService>)
    ServiceCollection &AddScoped(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<TContract, TService>(LifeTime::Scoped, factory);


        return *this;
    }

    template<typename TContract, typename TService>
        requires(std::is_base_of_v<TContract, TService>)
    ServiceCollection& AddScoped() {
        AddService<TContract, TService>(LifeTime::Scoped);


        return *this;
    }

    template<typename TService>
        requires(std::tuple_size_v<refl::as_tuple<TService> > > 0)
    ServiceCollection &AddScoped() {
        AddServiceWithConstructorArgs<TService, TService>(LifeTime::Scoped);

        return *this;
    }

    template<typename TService>
    ServiceCollection &AddScoped() {
        AddService<TService, TService>(LifeTime::Scoped);

        return *this;
    }

    template<typename TService>
    [[nodiscard]] bool Contains() const {
        return mServiceDefinitionMap->contains(
            GetServiceId<TService>());
    }

    [[nodiscard]] std::shared_ptr<ServiceProvider> CreateServiceProvider() const {
        return std::make_shared<ServiceProvider>(mServiceDefinitionMap);
    }

protected:
    template<typename TContract, typename TService>
    void AddService(const LifeTime lifeTime) {
        assert(!Contains<TContract>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<TContract>(), {
                .factory = [](ServiceProvider &) { return std::make_shared<TService>(); },
                .lifetime = lifeTime
            }
        });
    }

    template<typename TContract, typename TService>
        requires(std::tuple_size_v<refl::as_tuple<TService> > > 0)
    void AddServiceWithConstructorArgs(const LifeTime lifeTime) {
        assert(!Contains<TContract>() && "Can't register twice");

        using ConstructorArgs = refl::as_tuple<TService>;

        mServiceDefinitionMap->insert({
            GetServiceId<TContract>(), {
                .factory = CreateServiceFactory<TService>(ConstructorArgs{}),
                .lifetime = lifeTime
            }
        });
    }

    template<typename TContract, typename TService>
    void AddServiceWithFactory(const LifeTime lifeTime, const ServiceFactory &factory) {
        assert(!Contains<TContract>() && "Can't register twice");

        mServiceDefinitionMap->insert(
            {GetServiceId<TContract>(), {.factory = factory, .lifetime = lifeTime}});
    }

    template<typename TContract, typename TService>
    void AddServiceWithInstance(std::shared_ptr<TService> instance, const LifeTime lifeTime) {
        assert(!Contains<TContract>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<TContract>(),
            {
                .factory = [instance = instance](ServiceProvider &) { return instance; },
                .lifetime = lifeTime
            }
        });
    }

    template<typename TService, typename... Args>
        requires(std::is_constructible_v<TService, Args...>)
    ServiceFactory CreateServiceFactory(std::tuple<Args...> tuple) {
        return ServiceFactory([](ServiceProvider &serviceProvider) {
            return std::make_shared<TService>(
                serviceProvider.GetService<std::remove_pointer_t<decltype(Args(nullptr).get())> >()...);
        });
    }

private:
    std::shared_ptr<ServiceDefinitionMap> mServiceDefinitionMap;
};