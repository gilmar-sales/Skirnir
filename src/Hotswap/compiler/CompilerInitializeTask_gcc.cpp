#include "Skirnir/Hotswap/compiler/CompilerInitializeTask_gcc.hpp"

namespace skr::hotswap
{
    CompilerInitializeTask_gcc::CompilerInitializeTask_gcc(CompilerConfig* pConfig)
        : m_pConfig(pConfig)
    {
    }

    void CompilerInitializeTask_gcc::Start(ICmdShell* /*pCmdShell*/,
                                           std::chrono::milliseconds /*timeout*/,
                                           const std::function<void(Result)>& doneCb)
    {
        // In compile_commands.json mode, no environment probing is required.
        m_bDone = true;
        if (doneCb)
        {
            doneCb(Result::Success);
        }
    }

    void CompilerInitializeTask_gcc::Update()
    {
    }
} // namespace skr::hotswap
