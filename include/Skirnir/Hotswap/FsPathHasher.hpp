#pragma once

#include "Skirnir/Hotswap/Filesystem.hpp"

namespace skr::hotswap
{
    class FsPathHasher
    {
      public:
        std::size_t operator()(const fs::path& path) const;
    };
} // namespace skr::hotswap
