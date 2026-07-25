#pragma once

#include "Skirnir/Hotswap/compiler/ICompiler.hpp"

namespace skr::hotswap
{
    class ICompilerCmdLine
    {
      public:
        virtual ~ICompilerCmdLine() = default;

        virtual bool GenerateCommandFile(const fs::path& commandFilePath,
                                         const fs::path& moduleFilePath,
                                         const ICompiler::Input& input) = 0;

        virtual std::string GenerateCommand(const fs::path& moduleFilePath,
                                            const ICompiler::Input& input)
        {
            return "";
        }
    };
} // namespace skr::hotswap
