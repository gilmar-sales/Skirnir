#pragma once

// Conceptually, this is part of the platform. Need to put in its own file to avoid
// circular dependency between interfaces (ex. ICompiler) and Platform.hpp.

#ifdef SKR_HOTSWAP_USE_GHC_FILESYSTEM

#include "Skirnir/Hotswap/Private/filesystem/filesystem.hpp"

namespace skr::hotswap
{
    namespace fs = ghc::filesystem;
}

#else

#include <filesystem>

namespace skr::hotswap
{
    namespace fs = std::filesystem;
}

#endif
