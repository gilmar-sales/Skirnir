#pragma once

#include "Skirnir/Configuration/Aliases.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Source of configuration data, producing a parsed JSON tree.
     */
    class IConfigurationSource
    {
      public:
        virtual ~IConfigurationSource() = default;

        /**
         * @brief Parse and return the root element of this source.
         *
         * The source owns the underlying parser; the returned element is
         * valid only while this source is alive. @c Load() may be called
         * more than once; subsequent calls should return the cached result.
         */
        virtual simdjson::dom::element Load() = 0;
    };
} // namespace SKIRNIR_NAMESPACE
