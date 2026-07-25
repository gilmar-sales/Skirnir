#pragma once

#include "Skirnir/Hotswap/compiler/ICompilerCmdLine.hpp"
#include "Skirnir/Hotswap/Config.hpp"

namespace skr::hotswap
{
    // Placeholder for parity; in this build we always use compile_commands.json.
    class CompilerCmdLine_msvc : virtual public ICompilerCmdLine
    {
      public:
        explicit CompilerCmdLine_msvc(CompilerConfig* pConfig);

        bool GenerateCommandFile(const fs::path& commandFilePath,
                                 const fs::path& moduleFilePath,
                                 const ICompiler::Input& input) override;

      private:
        CompilerConfig* m_pConfig = nullptr;
    };
} // namespace skr::hotswap
