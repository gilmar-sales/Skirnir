#pragma once

#include <chrono>
#include <functional>

#include "Skirnir/Hotswap/cmd-shell/ICmdShell.hpp"

namespace skr::hotswap
{
    class ICmdShellTask
    {
      public:
        enum class Result
        {
            Success,
            Failure,
        };

        virtual ~ICmdShellTask() = default;

        virtual void Start(ICmdShell* pCmdShell,
                           std::chrono::milliseconds timeout,
                           const std::function<void(Result)>& doneCb) = 0;
        virtual void Update() = 0;
    };
} // namespace skr::hotswap
