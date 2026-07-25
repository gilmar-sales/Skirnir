#pragma once

#include <unordered_map>
#include <string>

#include "Skirnir/Hotswap/module/IAllocator.hpp"

namespace skr::hotswap
{
    class ITracker;
    class IConstructor;

    class ModuleSharedState
    {
      public:
        // Internal global state required by skr::hotswap. Modify at your own peril.
        static bool* s_pbSwapping;
        static std::unordered_map<std::string, std::vector<ITracker*>>* s_pTrackersByKey;
        static std::unordered_map<std::string, IConstructor*>* s_pConstructorsByKey;
        static IAllocator* s_pAllocator;
    };
} // namespace skr::hotswap
