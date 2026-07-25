#pragma once

#include <chrono>
#include <memory>

#include "Skirnir/Hotswap/Filesystem.hpp"
#include "Skirnir/Hotswap/compiler/ICompiler.hpp"
#include "Skirnir/Hotswap/file-watcher/IFileWatcher.hpp"

namespace skr::hotswap
{
    struct CompilerConfig
    {
        CompilerConfig();

        int cppStandard = 26;
        std::chrono::milliseconds initializeTimeout = std::chrono::milliseconds(60000);

        // Path to the compiler executable. Defaults to the value provided by
        // the compile_commands.json entry (resolved per source file). When the
        // command line uses an absolute compiler path, it is copied here.
        fs::path executable;

        // compile_commands.json path. The compiler pipeline will look up the
        // command for each source file in this file. If empty at construction
        // time, the configured CMake build's compile_commands.json (if any) is
        // used as a fallback.
        fs::path compileCommandsJson;
    };

    struct FileWatcherConfig
    {
        std::chrono::milliseconds latency = std::chrono::milliseconds(100);
    };

    struct Config
    {
        enum class Flag : std::uint64_t
        {
            None = 0,
            NoDefaultCompileOptions = (1 << 0),
            NoDefaultPreprocessorDefinitions = (1 << 1),
            NoDefaultIncludeDirectories = (1 << 2),
            NoDefaultForceCompiledSourceFiles = (1 << 3),
        };

        CompilerConfig compiler;
        FileWatcherConfig fileWatcher;

        Flag flags = Flag::None;
    };

    Config::Flag operator|(Config::Flag lhs, Config::Flag rhs);
    Config::Flag operator|=(Config::Flag& lhs, Config::Flag rhs);
    bool operator&(Config::Flag lhs, Config::Flag rhs);
} // namespace skr::hotswap
