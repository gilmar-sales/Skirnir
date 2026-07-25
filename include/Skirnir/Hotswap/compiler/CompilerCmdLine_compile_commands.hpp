#pragma once

#include <unordered_map>

#include "Skirnir/Hotswap/Config.hpp"
#include "Skirnir/Hotswap/compiler/ICompilerCmdLine.hpp"

namespace skr::hotswap
{
    // Generates compiler invocations from a compile_commands.json file. Each
    // source file's compile command is looked up by its absolute path.
    class CompilerCmdLine_compile_commands : virtual public ICompilerCmdLine
    {
      public:
        explicit CompilerCmdLine_compile_commands(CompilerConfig* pConfig);

        bool GenerateCommandFile(const fs::path& commandFilePath,
                                 const fs::path& moduleFilePath,
                                 const ICompiler::Input& input) override;
                                 
        std::string GenerateCommand(const fs::path& moduleFilePath,
                                 const ICompiler::Input& input) override;

      private:
        CompilerConfig* m_pConfig = nullptr;
        std::unordered_map<std::string, std::string> m_CommandsMap;
    };
} // namespace skr::hotswap
