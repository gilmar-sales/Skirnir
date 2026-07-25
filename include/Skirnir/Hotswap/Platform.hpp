#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Skirnir/Hotswap/Config.hpp"
#include "Skirnir/Hotswap/Filesystem.hpp"

#include "Skirnir/Hotswap/cmd-shell/ICmdShell.hpp"
#include "Skirnir/Hotswap/compiler/ICompiler.hpp"
#include "Skirnir/Hotswap/file-watcher/IFileWatcher.hpp"

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)

    #include <Windows.h>

#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)

    #include <dlfcn.h>
    #include <errno.h>

#endif

// These are normally injected by CMake. Provide fallbacks so headers parse in
// isolation.
#if !defined(SKR_HOTSWAP_ROOT_PATH)
    #define SKR_HOTSWAP_ROOT_PATH "."
#endif
#if !defined(SKR_HOTSWAP_BUILD_PATH)
    #define SKR_HOTSWAP_BUILD_PATH "."
#endif
#if !defined(SKR_HOTSWAP_CXX_STANDARD)
    #define SKR_HOTSWAP_CXX_STANDARD 17
#endif

namespace skr::hotswap
{
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)

    using TOsError = unsigned long;

    #define SKR_HOTSWAP_ERROR_SUCCESS        ERROR_SUCCESS
    #define SKR_HOTSWAP_ERROR_FILE_NOT_FOUND ERROR_FILE_NOT_FOUND

#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)

    using TOsError = int;

    #define SKR_HOTSWAP_ERROR_SUCCESS        0
    #define SKR_HOTSWAP_ERROR_FILE_NOT_FOUND ENOENT

#endif

    namespace platform
    {
        std::unique_ptr<IFileWatcher> CreateFileWatcher(
            FileWatcherConfig* pConfig);
        std::unique_ptr<ICompiler> CreateCompiler(CompilerConfig* pConfig);
        std::unique_ptr<ICmdShell> CreateCmdShell();

        std::vector<std::string> GetDefaultCompileOptions(
            int cppStandard = SKR_HOTSWAP_CXX_STANDARD);
        std::vector<std::string> GetDefaultPreprocessorDefinitions();

        void        WriteDebugString(const std::string& str);
        std::string CreateGuid();

        std::string GetErrorString(int error);
        std::string GetLastErrorString();

        std::string GetSharedLibraryExtension();
        void*       LoadModule(const fs::path& modulePath);

        template <typename TSignature>
        std::function<TSignature> GetModuleFunction(void*              pModule,
                                                    const std::string& name)
        {
            void* fn = nullptr;
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
            fn = reinterpret_cast<void*>(
                GetProcAddress(static_cast<HMODULE>(pModule), name.c_str()));
#else
            fn = reinterpret_cast<void*>(dlsym(pModule, name.c_str()));
#endif
            return std::function<TSignature>(reinterpret_cast<TSignature*>(fn));
        }
    } // namespace platform

#define SKR_HOTSWAP_UNUSED_PARAM(param) (void) param
#define SKR_HOTSWAP_UNUSED_VAR(param)   (void) param

} // namespace skr::hotswap

#include "Skirnir/Hotswap/Log.hpp"
