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
        assert(!Contains<Concrete>() && "Can't register twice");

        using ConstructorArgs = refl::as_tuple<Concrete>;
        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(), {
                .factory = CreateServiceFactory<Concrete>(ConstructorArgs{}),
                .lifetime = LifeTime::Singleton
            }
        });
    }

    template<typename Concrete>
    void AddSingleton() {
        assert(!Contains<Concrete>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(), {
                .factory = [](ServiceProvider &) { return std::make_shared<Concrete>(); },
                .lifetime = LifeTime::Singleton
            }
        });
    }

    template<typename Concrete>
    void AddSingleton(std::shared_ptr<Concrete> element) {
        assert(!Contains<Concrete>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(), {
                .factory = [element = element](ServiceProvider &) { return element; },
                .lifetime = LifeTime::Singleton
            }
        });
    }

    template<typename Interface, typename Concrete>
        requires(not std::is_same_v<Interface, Concrete> and std::is_base_of_v<Interface, Concrete>)
    void AddSingleton(std::shared_ptr<Concrete> element) {
        assert(!Contains<Interface>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(),
            {
                .factory = [element = element](ServiceProvider &) { return element; },
                .lifetime = LifeTime::Singleton
            }
        });
    }

    template<typename Concrete>
    void AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        assert(!Contains<Concrete>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(), {.factory = factory, .lifetime = LifeTime::Transient}
        });
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddTransient(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        assert(!Contains<Concrete>() && "Can't register twice");

        mServiceDefinitionMap->insert(
            {GetServiceId<Interface>(), {.factory = factory, .lifetime = LifeTime::Transient}});
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
        assert(!Contains<Concrete>() && "Can't register twice");

        mServiceDefinitionMap->insert({
            GetServiceId<Concrete>(), {.factory = factory, .lifetime = LifeTime::Scoped}
        });
    }

    template<typename Interface, typename Concrete>
        requires(std::is_base_of_v<Interface, Concrete>)
    void AddScoped(const std::function<std::shared_ptr<void>(ServiceCollection &)> &factory) {
        assert(!Contains<Interface>() && "Can't register twice");

        mServiceDefinitionMap->insert(
            {GetServiceId<Interface>(), {.factory = factory, .lifetime = LifeTime::Scoped}});
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
