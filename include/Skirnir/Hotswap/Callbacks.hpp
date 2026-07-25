#pragma once

#include <functional>

#include "Skirnir/Hotswap/compiler/ICompiler.hpp"

namespace skr::hotswap
{
    struct Callbacks
    {
        std::function<void(ICompiler::Input&)> BeforeCompile;
        std::function<void()> BeforeSwap;
        std::function<void()> AfterSwap;
    };
} // namespace skr::hotswap
