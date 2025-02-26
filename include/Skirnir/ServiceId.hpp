#pragma once

namespace SKIRNIR_NAMESPACE
{
    using ServiceId = unsigned long;

    inline ServiceId DependencyCount = 0;

    template <typename T>
    constexpr auto GetServiceId() -> ServiceId
    {
        const static auto id = DependencyCount++;

        return id;
    }
} // namespace SKIRNIR_NAMESPACE