#pragma once

#include "Skirnir/Hotswap/cmd-shell/ICmdShellTask.hpp"
#include "Skirnir/Hotswap/Config.hpp"

namespace skr::hotswap
{
    // Initialize task is currently a no-op for compile_commands.json mode - the
    // JSON file is loaded on demand inside CompilerCmdLine_compile_commands.
    class CompilerInitializeTask_gcc : public ICmdShellTask
    {
      public:
        explicit CompilerInitializeTask_gcc(CompilerConfig* pConfig);

        void Start(ICmdShell* pCmdShell,
                   std::chrono::milliseconds timeout,
                   const std::function<void(Result)>& doneCb) override;
        void Update() override;

      private:
        CompilerConfig* m_pConfig = nullptr;
        bool m_bDone = false;
    };
} // namespace skr::hotswap
