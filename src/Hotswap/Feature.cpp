#include "Skirnir/Hotswap/Feature.hpp"

namespace skr::hotswap
{
    std::size_t FeatureHasher::operator()(Feature feature) const
    {
        return static_cast<std::size_t>(feature);
    }
} // namespace skr::hotswap
