#pragma once

#include <algorithm>
#include <functional>

#include "Skirnir/Hotswap/module/CompileTimeString.hpp"
#include "Skirnir/Hotswap/module/Constructors.hpp"
#include "Skirnir/Hotswap/module/ModuleInterface.hpp"
#include "Skirnir/Hotswap/module/ModuleSharedState.hpp"
#include "Skirnir/Hotswap/module/PreprocessorMacros.hpp"
#include "Skirnir/Hotswap/module/SwapInfo.hpp"

namespace skr::hotswap
{
    //============================================================================
    // Register
    //============================================================================

    template <typename T, typename CompileTimeKey>
    class Register
    {
      public:
        Register()
        {
            const char* pKey = CompileTimeKey().ToString();
            skr::hotswap::Constructors::RegisterConstructor<T>(pKey);
        }

        void ForceInitialization() {}
    };

    //============================================================================
    // Tracker
    //============================================================================

    template <typename T, typename CompileTimeKey>
    class Tracker : public ITracker
    {
      public:
        // The SwapHandler is an optional, user-set callback that will be called
        // on runtime swaps.
        std::function<void(SwapInfo& swapInfo)> SwapHandler;

        Tracker(const Tracker& rhs)            = delete;
        Tracker& operator=(const Tracker& rhs) = delete;

        Tracker(T* pTrackedObj)
        {
            s_Register.ForceInitialization();

            m_pTrackedObj = pTrackedObj;

            const char* pKey = CompileTimeKey().ToString();
            (*ModuleSharedState::s_pTrackersByKey)[pKey].push_back(this);
        }

        ~Tracker()
        {
            const char*             pKey = CompileTimeKey().ToString();
            std::vector<ITracker*>& trackers =
                (*ModuleSharedState::s_pTrackersByKey)[pKey];
            auto trackerIt = std::find(trackers.begin(), trackers.end(), this);
            if (trackerIt != trackers.end())
            {
                trackers.erase(trackerIt);
            }
        }

        std::uint64_t FreeTrackedObject() override
        {
            if (ModuleSharedState::s_pAllocator == nullptr)
            {
                delete m_pTrackedObj;
                return 0;
            }

            m_pTrackedObj->~T();
            return ModuleSharedState::s_pAllocator->Skr_FreeSwap(
                reinterpret_cast<std::uint8_t*>(m_pTrackedObj));
        }

        void CallSwapHandler(SwapInfo& info) override
        {
            if (SwapHandler != nullptr)
            {
                SwapHandler(info);
            }
        }

        std::string GetKey() override { return CompileTimeKey().ToString(); }

      private:
        static Register<T, CompileTimeKey> s_Register;
        T*                                 m_pTrackedObj = nullptr;
    };

    template <typename T, typename CompileTimeKey>
    Register<T, CompileTimeKey> Tracker<T, CompileTimeKey>::s_Register;

} // namespace skr::hotswap

namespace skr::hotswap
{
    class AllocationResolver;
}

#ifndef SKR_DISABLE_HOTSWAP

    #define SKR_TRACK(type, key)                                               \
        friend class skr::hotswap::AllocationResolver;                         \
        static constexpr skr::hotswap::compile_time::KeylenCache<              \
            skr::hotswap::compile_time::Strlen(key)>                           \
            skr_KeylenCache = {};                                              \
        static constexpr skr::hotswap::compile_time::String<                   \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 0, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 1, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 2, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 3, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 4, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 5, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 6, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 7, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 8, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 9, decltype(skr_KeylenCache)::len),                       \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 10, decltype(skr_KeylenCache)::len),                      \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 11, decltype(skr_KeylenCache)::len),                      \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 12, decltype(skr_KeylenCache)::len),                      \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 13, decltype(skr_KeylenCache)::len),                      \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 14, decltype(skr_KeylenCache)::len),                      \
            skr::hotswap::compile_time::StringSegmentToIntegral(               \
                key, 15, decltype(skr_KeylenCache)::len)>                      \
            skr_ClassKey = {};                                                 \
        skr::hotswap::Tracker<type, decltype(skr_ClassKey)>                    \
            skr_ClassTracker = { this };

    #define Skr_SetSwapHandler(cb) skr_ClassTracker.SwapHandler = cb;

    #define Skr_IsSwapping() (*skr::hotswap::ModuleSharedState::s_pbSwapping)

    #define skr_virtual virtual

#else

    #define SKR_TRACK(type, key)
    #define Skr_SetSwapHandler(cb) (void) cb
    #define Skr_IsSwapping()       false
    #define skr_virtual

#endif
