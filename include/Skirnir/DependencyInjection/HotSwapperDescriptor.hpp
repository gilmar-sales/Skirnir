#pragma once

#include <memory>
#include <string>

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceCollection.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"
#include "Skirnir/Hotswap/Hotswapper.hpp"
#include "Skirnir/Hotswap/ModuleManager.hpp"
#include "Skirnir/Hotswap/module/Tracker.hpp"

namespace skr
{
    /**
     * @brief Registers a hot-swappable service with the IoC container.
     *
     * Skirnir's hot-swap system uses the SKR_TRACK macro to register
     * runtime-reloadable types with a Hotswapper. The first time the service
     * is resolved, the implementation is allocated through the Hotswapper's
     * allocation resolver so that future swaps replace the underlying object
     * in place. Subsequent resolutions return the current live instance
     * (the same Arc, kept alive by Skirnir's tracking).
     *
     * @tparam TInterface The contract/interface to register.
     * @tparam TImpl      The concrete implementation. Must be default-
     *                    constructible and must use SKR_TRACK(TImpl, "key").
     * @param sc          Service collection being built.
     * @param swapper     The Hotswapper that owns the swap lifecycle.
     * @param key         The hot-swap key passed to SKR_TRACK.
     */
    template <class TInterface, class TImpl>
        requires(std::is_base_of_v<TInterface, TImpl> &&
                 std::is_default_constructible_v<TImpl>)
    ServiceCollection& AddHotSwapper(ServiceCollection& sc,
                                    skr::hotswap::Hotswapper& swapper,
                                    std::string key)
    {
        auto* mgr = &swapper.GetModuleManager();
        // ReSharper disable once CppDeclaratorNeverUsed
        (void)mgr;
        // Register TInterface as a transient whose factory always returns the
        // currently-tracked instance for `key`. Each new resolution re-reads
        // ModuleManager::ResolveTracked(key) so swaps are picked up.
        sc.AddSingleton<TInterface>([&swapper, key = std::move(key)](
                                        skr::ServiceProvider&) -> skr::Arc<void> {
            void* raw = swapper.GetModuleManager().ResolveTracked(key);
            if (raw == nullptr)
            {
                // Not yet instantiated (no SKR_TRACK owner has constructed it).
                // Construct the first one ourselves through the resolver so the
                // swapper starts tracking it.
                TImpl* impl = swapper.GetAllocationResolver()->Allocate<TImpl>();
                if (impl == nullptr)
                {
                    return nullptr;
                }
                return skr::MakeArc<TInterface>(static_cast<TInterface*>(impl));
            }
            // Wrap the raw pointer without owning it - the swapper owns
            // the lifetime. Use a no-op deleter.
            return skr::MakeArc<TInterface>(static_cast<TInterface*>(raw));
        });
        return sc;
    }
} // namespace skr
