#include "Skirnir/Hotswap/compiler/CompilerCmdLine_msvc.hpp"

#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap
{
    CompilerCmdLine_msvc::CompilerCmdLine_msvc(CompilerConfig* pConfig)
        : m_pConfig(pConfig)
    {
    }

    bool CompilerCmdLine_msvc::GenerateCommandFile(const fs::path& commandFilePath,
                                                   const fs::path& moduleFilePath,
                                                   const ICompiler::Input& input)
    {
        // Not used in the compile_commands.json build path.
        SKR_HOTSWAP_UNUSED_PARAM(commandFilePath);
        SKR_HOTSWAP_UNUSED_PARAM(moduleFilePath);
        SKR_HOTSWAP_UNUSED_PARAM(input);
        log::Warning() << SKR_HOTSWAP_LOG_PREFIX
                       << "CompilerCmdLine_msvc::GenerateCommandFile is not used; "
                          "configure compile_commands.json instead."
                       << log::End();
        return false;
    }
} // namespace skr::hotswap
