#include "Skirnir/Hotswap/compiler/CompilerCmdLine_gcc.hpp"

#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap
{
    CompilerCmdLine_gcc::CompilerCmdLine_gcc(CompilerConfig* pConfig)
        : m_pConfig(pConfig)
    {
    }

    bool CompilerCmdLine_gcc::GenerateCommandFile(const fs::path& commandFilePath,
                                                  const fs::path& moduleFilePath,
                                                  const ICompiler::Input& input)
    {
        // Not used in the compile_commands.json build path.
        SKR_HOTSWAP_UNUSED_PARAM(commandFilePath);
        SKR_HOTSWAP_UNUSED_PARAM(moduleFilePath);
        SKR_HOTSWAP_UNUSED_PARAM(input);
        log::Warning() << SKR_HOTSWAP_LOG_PREFIX
                       << "CompilerCmdLine_gcc::GenerateCommandFile is not used; "
                          "configure compile_commands.json instead."
                       << log::End();
        return false;
    }
} // namespace skr::hotswap
