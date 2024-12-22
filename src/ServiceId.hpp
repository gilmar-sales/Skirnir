#pragma once

using ServideId = unsigned long;

inline ServideId DependencyCount = 0;

template <typename T>
constexpr auto GetServiceId() -> ServideId
{
    const static auto id = DependencyCount++;

    return id;
}