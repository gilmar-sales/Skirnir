export module Skirnir:Core;

import std;

export template <typename T>
using Ref = std::shared_ptr<T>;

export namespace skr
{

    class ServiceProvider;

    template <typename T, typename... TArgs>
        requires(std::is_constructible_v<T, TArgs...>)
    Ref<T> MakeRef(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }

    enum class LifeTime
    {
        Transient,
        Scoped,
        Singleton
    };

    using ServiceId = unsigned long;

    inline ServiceId DependencyCount = 0;

    template <typename T>
    constexpr auto GetServiceId() -> ServiceId
    {
        static auto id = DependencyCount++;

        return id;
    }

    using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;

    struct ServiceDescription
    {
        ServiceId id;
        char*     name;

        bool operator<(const ServiceDescription& other) const
        {
            return id < other.id;
        }

        bool operator==(const ServiceDescription& other) const
        {
            return id == other.id;
        }

        bool operator!=(const ServiceDescription& other) const
        {
            return id != other.id;
        }
    };

    using InternalServiceFactory = std::function<Ref<void>(
        ServiceProvider&, std::set<ServiceDescription>&)>;

    struct ServiceDefinition
    {
        InternalServiceFactory factory  = nullptr;
        LifeTime               lifetime = LifeTime::Transient;
    };

    using ServiceDefinitionMap = std::map<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;

} // namespace skr
