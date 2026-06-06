#pragma once

#include <set>
#include <string_view>
#include <type_traits>

#include "Skirnir/Common/ConstructorArgumentTraits.hpp"
#include "Skirnir/Common/Keyed.hpp"
#include "Skirnir/Common/Ref.hpp"
#include "Skirnir/DependencyInjection/ServiceDescriptor.hpp"

namespace SKIRNIR_NAMESPACE
{
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
