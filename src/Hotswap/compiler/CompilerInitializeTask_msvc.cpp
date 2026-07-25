#include "Skirnir/Hotswap/compiler/CompilerInitializeTask_msvc.hpp"

namespace skr::hotswap
{
    CompilerInitializeTask_msvc::CompilerInitializeTask_msvc() = default;

    void CompilerInitializeTask_msvc::Start(ICmdShell* /*pCmdShell*/,
                                           std::chrono::milliseconds /*timeout*/,
                                           const std::function<void(Result)>& doneCb)
    {
        // MSVC mode is unused in this vendored build (compile_commands.json only).
        m_bDone = true;
        if (doneCb)
        {
            doneCb(Result::Success);
        }
    }

    void CompilerInitializeTask_msvc::Update()
    {
    }
} // namespace skr::hotswap
