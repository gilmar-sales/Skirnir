#pragma once

#include <vector>
#include <string>

#include "Skirnir/Hotswap/preprocessor/LangError.hpp"

namespace skr::hotswap
{
    struct SkrRequire
    {
        enum class Type
        {
            Unknown,
            Source,
            IncludeDir,
            Library,
            LibraryDir,
            PreprocessorDef,
        };

        std::string name; // ex. skr_require_source
        std::size_t line = LangError::NO_VALUE;

        Type type = Type::Unknown;
        std::vector<std::string> values;
    };
} // namespace skr::hotswap
