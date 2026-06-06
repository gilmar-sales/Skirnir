#include "Skirnir/Configuration/InMemorySource.hpp"

#include "Detail.hpp"

namespace SKIRNIR_NAMESPACE
{
    std::string InMemorySource::BuildJson() const
    {
        return detail::BuildJsonFromFlat(mFlat);
    }

    simdjson::dom::element InMemorySource::Load()
    {
        if (mDirty)
        {
            mSerialized = BuildJson();
            mParser     = simdjson::dom::parser {};
            mElement    = detail::ParseOrThrow(
                mParser, mSerialized, "in-memory source");
            mDirty      = false;
        }
        return mElement;
    }
} // namespace SKIRNIR_NAMESPACE
