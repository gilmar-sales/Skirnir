#pragma once

#include <functional>
#include <map>
#include <memory>

#define SKIRNIR_NAMESPACE skr

#include "ServiceId.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

#ifdef _MSC_VER
    #define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace SKIRNIR_NAMESPACE
{
    template <typename T, typename... TArgs>
        requires(std::is_constructible_v<T, TArgs...>)
    inline Ref<T> MakeRef(TArgs&&... args)
    {
        return std::make_shared<T>(std::forward<TArgs>(args)...);
    }
    enum class LifeTime
    {
        Transient,
        Scoped,
        Singleton
    };

    class ServiceCollection;
    class ServiceProvider;
    class ServiceScope;

    using ServiceFactory = std::function<Ref<void>(ServiceProvider&)>;

    struct ServiceDefinition
    {
        ServiceFactory factory  = nullptr;
        LifeTime       lifetime = LifeTime::Transient;
    };

    using ServiceDefinitionMap = std::map<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;

} // namespace SKIRNIR_NAMESPACE