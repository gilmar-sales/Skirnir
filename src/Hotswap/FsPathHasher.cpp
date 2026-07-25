#include "Skirnir/Hotswap/FsPathHasher.hpp"

namespace skr::hotswap
{
    std::size_t FsPathHasher::operator()(const fs::path& path) const
    {
        return fs::hash_value(path);
    }
} // namespace skr::hotswap
