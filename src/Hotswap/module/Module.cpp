#include "Skirnir/Hotswap/module/ModuleInterface.hpp"
#include "Skirnir/Hotswap/module/GlobalUserData.hpp"
#include "Skirnir/Hotswap/module/ModuleSharedState.hpp"

// The module bridge code that has to live in a .cpp so the host executable and
// every hot-swapped module see exactly one definition of these globals and of
// Skr_GetModuleInterface.

namespace skr::hotswap
{
    void* GlobalUserData::s_pData = nullptr;

    bool* ModuleSharedState::s_pbSwapping = nullptr;
    std::unordered_map<std::string, std::vector<ITracker*>>*
        ModuleSharedState::s_pTrackersByKey = nullptr;
    std::unordered_map<std::string, IConstructor*>*
        ModuleSharedState::s_pConstructorsByKey = nullptr;
    IAllocator* ModuleSharedState::s_pAllocator = nullptr;
} // namespace skr::hotswap

extern "C"
{
    SKR_HOTSWAP_API skr::hotswap::ModuleInterface* Skr_GetModuleInterface()
    {
        static skr::hotswap::ModuleInterface moduleInterface;
        return &moduleInterface;
    }
}
