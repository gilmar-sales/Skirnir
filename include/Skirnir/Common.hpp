#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
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
        LifeTime                   lifetime = LifeTime::Transient;
        std::string                key;
        std::vector<ServiceId>     ctorDeps;
    };

    using ServiceDefinitionMap = std::multimap<ServiceId, ServiceDefinition>;
    using ServicesCache        = std::map<ServiceId, Ref<void>>;
    using KeyedServicesCache   = std::map<std::pair<ServiceId, std::string>,
                                         Ref<void>>;

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
     * @brief Detects whether @c T is @c std::optional<Ref<U>> for some U.
     */
    template <typename T>
    struct is_optional_of_ref : std::false_type
    {
    };

    template <typename U>
    struct is_optional_of_ref<std::optional<Ref<U>>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_optional_of_ref_v = is_optional_of_ref<T>::value;

    namespace detail
    {
        inline constexpr char SkirnirEmptyKey[] = "";
    }

    /**
     * @brief Tagged wrapper used to select a keyed service at injection time.
     *
     * The @c Key template argument is a NTTP that names the registration key.
     * The user must declare a constexpr character array for the key:
     *
     * @code
     * inline constexpr char keyA[] = "a";
     *
     * class Consumer {
     *     Consumer(Keyed<IFoo, keyA> k) : mFoo(k.ptr) {}
     * };
     * @endcode
     *
     * The @c ptr member is filled by the container's resolution logic; the
     * consumer never needs to call into the provider itself.
     */
    template <typename T, const char* Key = detail::SkirnirEmptyKey>
    struct Keyed
    {
        static constexpr const char* key = Key;
        Ref<T>                        ptr;
    };

    template <typename T>
    struct is_keyed : std::false_type
    {
    };

    template <typename T, const char* K>
    struct is_keyed<Keyed<T, K>> : std::true_type
    {
    };

    template <typename T>
    inline constexpr bool is_keyed_v = is_keyed<T>::value;

    template <typename T>
    struct keyed_inner
    {
    };

    template <typename T, const char* K>
    struct keyed_inner<Keyed<T, K>>
    {
        using type = T;
    };

    template <typename T>
    using keyed_inner_t = typename keyed_inner<T>::type;

    /**
     * @brief Resolves a constructor parameter from the ServiceProvider.
     *
     * - If @c Arg is @c std::vector<Ref<U>>, returns all services registered
     *   for @c U via @c GetServices.
     * - If @c Arg is @c std::optional<Ref<U>>, returns a `nullopt` when @c U is
     *   not registered, otherwise the resolved service.
     * - If @c Arg is @c Keyed<U, K>, returns a @c Keyed wrapper whose
     *   @c ptr has been resolved through @c GetKeyedService using @c K.
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
        else if constexpr (is_optional_of_ref_v<Arg>)
        {
            using U = typename Arg::value_type::element_type;
            return sp.template TryGetService<U>();
        }
        else if constexpr (is_keyed_v<Arg>)
        {
            using U       = keyed_inner_t<Arg>;
            using Wrapped = Keyed<U, Arg::key>;
            Wrapped wrapper {};
            if constexpr (std::string_view(Arg::key).empty())
                wrapper.ptr = sp.template GetService<U>();
            else
                wrapper.ptr = sp.template GetKeyedService<U>(Arg::key);
            return wrapper;
        }
        else
        {
            using U = typename Arg::element_type;
            return sp.template GetServiceImpl<U>(desc);
        }
    }

} // namespace SKIRNIR_NAMESPACE
