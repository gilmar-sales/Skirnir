#pragma once

#include <unordered_set>

#include "Skirnir/Hotswap/Feature.hpp"

namespace skr::hotswap
{
    class FeatureManager
    {
      public:
        void EnableFeature(Feature feature);
        void DisableFeature(Feature feature);
        bool IsFeatureEnabled(Feature feature);

      private:
        std::unordered_set<Feature, FeatureHasher> m_Features;
    };
} // namespace skr::hotswap
