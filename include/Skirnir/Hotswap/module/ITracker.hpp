#pragma once

#include <string>

#include "Skirnir/Hotswap/module/SwapInfo.hpp"

namespace skr::hotswap
{
    // Required to be in its own file to avoid circular dependency with Tracker and ModuleInterface.
    class ITracker
    {
      public:
        virtual std::uint64_t FreeTrackedObject() = 0;
        virtual std::string GetKey() = 0;
        virtual void CallSwapHandler(SwapInfo& info) = 0;
    };
} // namespace skr::hotswap
