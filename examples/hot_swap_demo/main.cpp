#include <Skirnir/DependencyInjection/HotSwapperDescriptor.hpp>
#include <Skirnir/Hotswap.hpp>
#include <Skirnir/Skirnir.hpp>

#include <chrono>
#include <iostream>
#include <thread>

#include "Plugin.hpp"

int main()
{
    std::cout << "=== Skirnir hot-swap demo ===" << std::endl;
    std::cout << "Edit Plugin.cpp and save - the running process will pick it "
                 "up within a second."
                 << std::endl;

    // 1. Build the Hotswapper. The default Config reads compile_commands.json
    //    from the current build directory. The hotswapper's source directory
    //    is set to the same directory that holds Plugin.cpp so file changes
    //    are detected.
    skr::hotswap::Hotswapper swapper;
    auto exampleDir = std::filesystem::path("D:/dev/Skirnir/examples/hot_swap_demo");
    auto buildDir   = std::filesystem::path("D:/dev/Skirnir/build");
    swapper.AddSourceDirectory(exampleDir);
    swapper.AddIncludeDirectory(exampleDir);
    swapper.EnableFeature(skr::hotswap::Feature::Preprocessor);
    swapper.EnableFeature(skr::hotswap::Feature::DependentCompilation);

    // The plugin DLL must resolve skr::hotswap::ModuleSharedState (defined in
    // the skirnir library), so link against the freshly built libskirnir.a.
    swapper.AddLibraryDirectory(buildDir);
    swapper.LocateAndAddLibrary(buildDir, "libskirnir.a");

    // 2. Wire it into Skirnir's IoC container.
    skr::ServiceCollection services;
    skr::AddHotSwapper<IPlugin, Plugin>(services, swapper, "hot_swap_demo::Plugin");
    auto provider = services.CreateServiceProvider();

    // 3. First construction: the Hotswapper allocates a Plugin instance and
    //    starts tracking it. Subsequent swaps replace it in place.
    while (true)
    {
        auto plugin = provider->GetService<IPlugin>();
        if (plugin)
        {
            std::cout << plugin->Greet() << std::endl;
        }
        else
        {
            std::cout << "(no plugin yet)" << std::endl;
        }

        swapper.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
