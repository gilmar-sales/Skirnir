#pragma once

#include <string_view>

#include "Skirnir/Common/Namespace.hpp"
#include "Skirnir/Common/Reflection.hpp"

namespace SKIRNIR_NAMESPACE
{
    using ServiceId = unsigned long;

    /**
     * @brief Maps a stable type name to a dense @c ServiceId (0..N-1).
     *
     * The same name always yields the same id for the lifetime of the
     * process, which keeps @c GetServiceId consistent across static libs
     * and DSOs (unlike a per-TU counter / static local).
     */
    ServiceId RegisterTypeName(std::string_view typeName);

    template <typename T>
    auto GetServiceId() -> ServiceId
    {
        static const ServiceId id = RegisterTypeName(refl::type_name<T>());
        return id;
    }
} // namespace SKIRNIR_NAMESPACE
