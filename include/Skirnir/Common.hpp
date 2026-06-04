#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <type_traits>

#define SKIRNIR_NAMESPACE skr

#include "ServiceId.hpp"

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T>
using WeakRef = std::weak_ptr<T>;

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

    /**
     * @brief Performs a static pointer cast on a shared_ptr.
     *
     * @tparam TDest  The target type to cast to
     * @tparam TSource The source type being cast from
     * @param source  The shared_ptr to cast
     * @return        A shared_ptr of type TDest, or nullptr if cast fails
     */
    template <typename TDest, typename TSource>
    inline Ref<TDest> RefCast(const Ref<TSource>& source)
    {
        return std::static_pointer_cast<TDest>(source);
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

    struct ServiceDescription
    {
        ServiceId        id;
        std::string_view name;

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
        std::function<Ref<void>(
            ServiceProvider&, std::set<ServiceDescription>&)>
                 factory  = nullptr;
        LifeTime lifetime = LifeTime::Transient;
    };

    using ServiceDefinitionMap = std::multimap<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;

    /**
     * @brief Detects whether @c T is @c std::vector<Ref<U>> for some U.
     */
    template <typename T>
    struct is_vector_of_ref : std::false_type
    {
    };

    template <typename U, typename Alloc>
    struct is_vector_of_ref<std::vector<Ref<U>, Alloc>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_vector_of_ref_v = is_vector_of_ref<T>::value;

    /**
     * @brief Resolves a constructor parameter from the ServiceProvider.
     *
     * - If @c Arg is @c std::vector<Ref<U>>, returns all services registered
     *   for @c U via @c GetServices.
     * - Otherwise, treats @c Arg as @c Ref<U> and returns a single service
     *   via @c GetServiceImpl.
     */
    template <typename Arg, typename ServiceProviderT>
    auto Resolve(ServiceProviderT& sp, std::set<ServiceDescription>& desc)
    {
        if constexpr (is_vector_of_ref_v<Arg>)
        {
            using U = typename Arg::value_type::element_type;
            return sp.template GetServices<U>();
        }
        else
        {
            using U = typename Arg::element_type;
            return sp.template GetServiceImpl<U>(desc);
        }
    }

} // namespace SKIRNIR_NAMESPACE
