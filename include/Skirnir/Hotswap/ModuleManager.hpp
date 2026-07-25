#pragma once

#include <unordered_map>
#include <memory>

#include "Skirnir/Hotswap/Platform.hpp"
#include "Skirnir/Hotswap/module/ITracker.hpp"
#include "Skirnir/Hotswap/module/IAllocator.hpp"
#include "Skirnir/Hotswap/module/Constructors.hpp"
#include "Skirnir/Hotswap/module/ModuleInterface.hpp"

namespace skr::hotswap
{
    class ModuleManager
    {
      public:
        ModuleManager();

        void SetAllocator(IAllocator* pAllocator);
        void SetGlobalUserData(void* pGlobalUserData);

        // Returns the (possibly empty) list of currently tracked instances of a
        // given key. Used by Skirnir's ServiceProvider to surface the latest
        // hot-swapped instance for a registered service.
        std::vector<ITracker*> GetTrackers(const std::string& key) const;

        // Resolves a tracked instance by its SKR_TRACK key. Returns nullptr if
        // no instance exists. The pointer is invalidated by the next swap.
        void* ResolveTracked(const std::string& key) const;

        bool PerformRuntimeSwap(const fs::path& modulePath);

      private:
        bool m_bSwapping = false;
        std::unordered_map<std::string, std::vector<ITracker*>> m_TrackersByKey;

        IAllocator* m_pAllocator = nullptr;
        void* m_pGlobalUserData = nullptr;

        std::unordered_map<std::string, IConstructor*> m_ConstructorsByKey;

        void WarnDuplicateKeys(ModuleInterface* pModuleInterface);
    };
} // namespace skr::hotswap
