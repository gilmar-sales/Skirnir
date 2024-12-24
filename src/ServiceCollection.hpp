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

    template<typename Concrete>
        requires(std::tuple_size_v<refl::as_tuple<Concrete> > > 0)
    void AddSingleton() {
        AddServiceWithConstructorArgs<Concrete, Concrete>(LifeTime::Singleton);
    }

    template<typename Concrete>
    void AddSingleton() {
        AddService<Concrete, Concrete>(LifeTime::Singleton);
    }

    template<typename Concrete>
    void AddSingleton(std::shared_ptr<Concrete> element) {
        AddServiceWithInstance<Concrete, Concrete>(element, LifeTime::Singleton);
    }

    template<typename Interface, typename Concrete>
        requires(not std::is_same_v<Interface, Concrete> and std::is_base_of_v<Interface, Concrete>)
    void AddSingleton(std::shared_ptr<Concrete> element) {
        AddServiceWithInstance<Interface, Concrete>(element, LifeTime::Singleton);
    }

    template<typename Concrete>
    void AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<Concrete, Concrete>(LifeTime::Transient, factory);
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<Interface, Concrete>(LifeTime::Transient, factory);
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddTransient() {
        AddService<Interface, Concrete>(LifeTime::Transient);
    }

    template<typename Concrete>
        requires(std::tuple_size_v<refl::as_tuple<Concrete> > > 0)
    void AddTransient() {
        AddServiceWithConstructorArgs<Concrete, Concrete>(LifeTime::Transient);
    }

    template<typename Concrete>
    void AddTransient() {
        AddService<Concrete, Concrete>(LifeTime::Transient);
    }

    template<typename Concrete>
    void AddScoped(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<Concrete, Concrete>(LifeTime::Scoped, factory);
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddScoped(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        AddServiceWithFactory<Interface, Concrete>(LifeTime::Scoped, factory);
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddScoped() {
        AddService<Interface, Concrete>(LifeTime::Scoped);
    }

    template<typename Concrete>
        requires(std::tuple_size_v<refl::as_tuple<Concrete> > > 0)
    void AddScoped() {
        AddServiceWithConstructorArgs<Concrete, Concrete>(LifeTime::Scoped);
    }

    template<typename Concrete>
    void AddScoped() {
        AddService<Concrete, Concrete>(LifeTime::Scoped);
    }

    template<typename Concrete>
    [[nodiscard]] bool Contains() const {
        return mServiceDefinitionMap->contains(
            GetServiceId<Concrete>());
    }

    [[nodiscard]] std::shared_ptr<ServiceProvider> CreateServiceProvider() const { return std::make_shared<ServiceProvider>(mServiceDefinitionMap);}

protected:
    template<typename Interface, typename Concrete>
    void AddService(const LifeTime lifeTime) {
        assert(!Contains<Interface>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Interface>(), {
                .factory = [](ServiceProvider &) { return std::make_shared<Concrete>(); },
                .lifetime = lifeTime
            }
        });
    }

    template<typename Interface, typename Concrete>
        requires(std::tuple_size_v<refl::as_tuple<Concrete> > > 0)
    void AddServiceWithConstructorArgs(const LifeTime lifeTime) {
        assert(!Contains<Interface>() && "Can't register twice");

        using ConstructorArgs = refl::as_tuple<Concrete>;

        mServiceDefinitionMap->insert({
            GetServiceId<Interface>(), {
                .factory = CreateServiceFactory<Concrete>(ConstructorArgs{}),
                .lifetime = lifeTime
            }
        });
    }

    template<typename Interface, typename Concrete>
    void AddServiceWithFactory(const LifeTime lifeTime, const ServiceFactory& factory) {
        assert(!Contains<Interface>() && "Can't register twice");

        mServiceDefinitionMap->insert(
            {GetServiceId<Interface>(), {.factory = factory, .lifetime = lifeTime}});
    }

    template <typename Interface, typename Concrete>
    void AddServiceWithInstance(std::shared_ptr<Concrete> instance, const LifeTime lifeTime) {
        assert(!Contains<Interface>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(),
            {
                .factory = [instance = instance](ServiceProvider &) { return instance; },
                .lifetime = lifeTime
            }
        });
    }

    template<typename Concrete, typename... Args>
        requires(std::is_constructible_v<Concrete, Args...>)
    ServiceFactory CreateServiceFactory(std::tuple<Args...> tuple) {
        return ServiceFactory([](ServiceProvider &serviceProvider) {
            return std::make_shared<Concrete>(
                serviceProvider.GetService<std::remove_pointer_t<decltype(Args(nullptr).get())> >()...);
        });
    }

private:
    std::shared_ptr<ServiceDefinitionMap> mServiceDefinitionMap;
};
