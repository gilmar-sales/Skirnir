#pragma once

#include <unordered_map>
#include <vector>
#include <cassert>

#include "Skirnir/Hotswap/module/ModuleSharedState.hpp"
#include "Skirnir/Hotswap/module/GlobalUserData.hpp"
#include "Skirnir/Hotswap/module/Constructors.hpp"
#include "Skirnir/Hotswap/module/ITracker.hpp"

#ifdef _WIN32

#define SKR_HOTSWAP_API __declspec(dllexport)

#else

#define SKR_HOTSWAP_API __attribute__((visibility("default")))

#endif

namespace skr::hotswap
{
    // Interface into the module. Functions are marked 'virtual' to force using the
    // new module's methods. Otherwise, symbol resolution will be ambiguous, and
    // the main executable will continue to use its own implementation.
    class ModuleInterface
    {
      public:
        virtual void SetIsSwapping(bool* pbSwapping)
        {
            ModuleSharedState::s_pbSwapping = pbSwapping;
        }

        virtual void SetTrackersByKey(
            std::unordered_map<std::string, std::vector<ITracker*>>* pTrackersByKey)
        {
            ModuleSharedState::s_pTrackersByKey = pTrackersByKey;
        }

        virtual void SetConstructorsByKey(
            std::unordered_map<std::string, IConstructor*>* pConstructorsByKey)
        {
            ModuleSharedState::s_pConstructorsByKey = pConstructorsByKey;
        }

        virtual void SetAllocator(IAllocator* pAllocator)
        {
            ModuleSharedState::s_pAllocator = pAllocator;
        }

        virtual void SetGlobalUserData(void* pGlobalUserData)
        {
            GlobalUserData::s_pData = pGlobalUserData;
        }

        virtual std::unordered_map<std::string, IConstructor*> GetModuleConstructorsByKey()
        {
            std::unordered_map<std::string, IConstructor*> constructorsByKey;
            std::size_t nConstructorKeys = Constructors::GetNumberOfKeys();
            for (std::size_t iKey = 0; iKey < nConstructorKeys; ++iKey)
            {
                std::string key = Constructors::GetKey(iKey);
                constructorsByKey[key] = Constructors::GetConstructor(key);
            }
            return constructorsByKey;
        }

        virtual void PerformRuntimeSwap()
        {
            *ModuleSharedState::s_pbSwapping = true;

            std::size_t nConstructorKeys = Constructors::GetNumberOfKeys();
            for (std::size_t iKey = 0; iKey < nConstructorKeys; ++iKey)
            {
                std::string key = Constructors::GetKey(iKey);

                (*ModuleSharedState::s_pConstructorsByKey)[key] =
                    Constructors::GetConstructor(key);

                auto trackersIt = ModuleSharedState::s_pTrackersByKey->find(key);
                if (trackersIt != ModuleSharedState::s_pTrackersByKey->end())
                {
                    std::vector<ITracker*>& trackedObjects = trackersIt->second;
                    std::vector<ITracker*> oldTrackedObjects = trackedObjects;

                    std::size_t nInstances = trackedObjects.size();

                    std::vector<SwapInfo> swapInfos(nInstances);
                    std::vector<std::uint64_t> memoryIds(nInstances);

                    for (std::size_t i = 0; i < nInstances; ++i)
                    {
                        swapInfos.at(i).m_Id = i;
                        swapInfos.at(i).m_Phase = SwapPhase::BeforeSwap;

                        ITracker* pTracker = oldTrackedObjects.at(i);
                        pTracker->CallSwapHandler(swapInfos.at(i));
                        memoryIds.at(i) = pTracker->FreeTrackedObject();
                    }

                    assert(trackedObjects.empty());

                    IConstructor* pConstructor = Constructors::GetConstructor(key);
                    for (std::size_t i = 0; i < nInstances; ++i)
                    {
                        pConstructor->AllocateSwap(memoryIds.at(i));

                        ITracker* pTracker = trackedObjects.at(i);
                        swapInfos.at(i).m_Phase = SwapPhase::AfterSwap;
                        pTracker->CallSwapHandler(swapInfos.at(i));
                        swapInfos.at(i).TriggerInitCb();
                    }
                }
            }

            *ModuleSharedState::s_pbSwapping = false;
        }

        virtual std::vector<Constructors::DuplicateKey> GetDuplicateKeys()
        {
            return Constructors::GetDuplicateKeys();
        }
    };
} // namespace skr::hotswap

extern "C"
{
    SKR_HOTSWAP_API skr::hotswap::ModuleInterface* Skr_GetModuleInterface();
}
