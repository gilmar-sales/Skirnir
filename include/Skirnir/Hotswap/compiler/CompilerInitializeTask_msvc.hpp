#pragma once

#include "Skirnir/Hotswap/cmd-shell/ICmdShellTask.hpp"

namespace skr::hotswap
{
    class CompilerInitializeTask_msvc : public ICmdShellTask
    {
      public:
        CompilerInitializeTask_msvc();

        void Start(ICmdShell* pCmdShell,
                   std::chrono::milliseconds timeout,
                   const std::function<void(Result)>& doneCb) override;
        void Update() override;

      private:
        bool m_bDone = false;
    };
} // namespace skr::hotswap
