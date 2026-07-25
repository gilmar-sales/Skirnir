#include <cstring>

#include "Skirnir/Hotswap/Platform.hpp"

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
#include <Windows.h>
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
#include <uuid/uuid.h>
#endif

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
#include "Skirnir/Hotswap/file-watcher/FileWatcher_win32.hpp"
#include "Skirnir/Hotswap/cmd-shell/CmdShell_win32.hpp"
#elif defined(SKR_HOTSWAP_PLATFORM_APPLE)
#include "Skirnir/Hotswap/file-watcher/FileWatcher_apple.hpp"
#include "Skirnir/Hotswap/cmd-shell/CmdShell_unix.hpp"
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
#include "Skirnir/Hotswap/file-watcher/FileWatcher_unix.hpp"
#include "Skirnir/Hotswap/cmd-shell/CmdShell_unix.hpp"
#endif

#include "Skirnir/Hotswap/compiler/Compiler.hpp"
#include "Skirnir/Hotswap/compiler/CompilerCmdLine_compile_commands.hpp"
#include "Skirnir/Hotswap/compiler/CompilerInitializeTask_gcc.hpp"
#include "Skirnir/Hotswap/compiler/CompilerCmdLine_gcc.hpp"

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
#include "Skirnir/Hotswap/compiler/CompilerInitializeTask_msvc.hpp"
#include "Skirnir/Hotswap/compiler/CompilerCmdLine_msvc.hpp"
#endif

namespace skr::hotswap::platform
{
    std::unique_ptr<IFileWatcher> CreateFileWatcher(FileWatcherConfig* pConfig)
    {
        return std::unique_ptr<IFileWatcher>(new FileWatcher(pConfig));
    }

    std::unique_ptr<ICompiler> CreateCompiler(CompilerConfig* pConfig)
    {
        std::unique_ptr<ICmdShellTask> pInitializeTask;
        std::unique_ptr<ICompilerCmdLine> pCompilerCmdLine;

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        pInitializeTask =
            std::unique_ptr<ICmdShellTask>(new CompilerInitializeTask_gcc(pConfig));
        pCompilerCmdLine = std::unique_ptr<ICompilerCmdLine>(
            new CompilerCmdLine_compile_commands(pConfig));
#endif

        return std::unique_ptr<ICompiler>(
            new Compiler(pConfig, std::move(pInitializeTask), std::move(pCompilerCmdLine)));
    }

    std::unique_ptr<ICmdShell> CreateCmdShell()
    {
        return std::unique_ptr<ICmdShell>(new CmdShell());
    }

    namespace
    {
        std::vector<std::string> GetDefaultCompileOptions_msvc(int cppStandard)
        {
            std::vector<std::string> options = {
                "/nologo", "/Z7", "/FC", "/EHsc",
#if !defined(SKR_HOTSWAP_PLATFORM_CLANG_CL)
                "/MP",
#endif
#if defined(SKR_HOTSWAP_DEBUG)
                "/MDd", "/LDd",
#else
                "/MD", "/Zo", "/LD",
#endif
            };
#if (_MSC_VER > 1900)
            if (cppStandard <= 11)
            {
                options.push_back("/std:c" + std::to_string(cppStandard));
            }
            else
            {
                options.push_back("/std:c++" + std::to_string(cppStandard));
            }
#else
            SKR_HOTSWAP_UNUSED_PARAM(cppStandard);
#endif
            return options;
        }

        std::vector<std::string> GetDefaultCompileOptions_gcc(int cppStandard)
        {
            return {
                "-std=c++" + std::to_string(cppStandard),
                "-shared",
                "-fPIC",
                "-fvisibility=hidden",
#if defined(SKR_HOTSWAP_DEBUG)
                "-g",
#endif
            };
        }
    } // namespace

    std::vector<std::string> GetDefaultCompileOptions(int cppStandard)
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        return GetDefaultCompileOptions_msvc(cppStandard);
#else
        return GetDefaultCompileOptions_gcc(cppStandard);
#endif
    }

    std::vector<std::string> GetDefaultPreprocessorDefinitions()
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        return {
#ifdef _DEBUG
            "_DEBUG",
#endif
#ifdef _WIN32
            "_WIN32",
#endif
        };
#else
        return {};
#endif
    }

    void WriteDebugString(const std::string& str)
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        OutputDebugString(str.c_str());
#else
        SKR_HOTSWAP_UNUSED_PARAM(str);
#endif
    }

    std::string CreateGuid()
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        GUID guid;
        CoCreateGuid(&guid);
        char buf[64];
        std::snprintf(buf, sizeof(buf),
                      "%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X",
                      guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                      guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                      guid.Data4[6], guid.Data4[7]);
        return buf;
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
        uuid_t uuid;
        uuid_generate_random(uuid);
        char buf[64];
        uuid_unparse(uuid, buf);
        return buf;
#else
        return "";
#endif
    }

#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
    std::string GetErrorString(int error)
    {
        if (error == ERROR_SUCCESS)
        {
            return "";
        }
        LPVOID buffer = nullptr;
        std::size_t size = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPTSTR>(&buffer), 0, NULL);
        std::string message(static_cast<char*>(buffer), size);
        LocalFree(buffer);
        if (message.size() >= 2 && message.at(message.size() - 2) == '\r' &&
            message.at(message.size() - 1) == '\n')
        {
            message.pop_back();
            message.pop_back();
        }
        return message;
    }

    std::string GetLastErrorString()
    {
        return GetErrorString(GetLastError());
    }
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
    std::string GetErrorString(OsError error)
    {
        return std::strerror(error);
    }

    std::string GetLastErrorString()
    {
        return GetErrorString(errno);
    }
#endif

    std::string GetSharedLibraryExtension()
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        return "dll";
#elif defined(SKR_HOTSWAP_PLATFORM_APPLE)
        return "dylib";
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
        return "so";
#else
        return "so";
#endif
    }

    void* LoadModule(const fs::path& modulePath)
    {
#if defined(SKR_HOTSWAP_PLATFORM_WIN32)
        return LoadLibraryW(modulePath.wstring().c_str());
#elif defined(SKR_HOTSWAP_PLATFORM_UNIX)
        return dlopen(modulePath.string().c_str(), RTLD_NOW);
#else
        return nullptr;
#endif
    }
} // namespace skr::hotswap::platform
