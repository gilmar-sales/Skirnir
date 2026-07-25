#pragma once

#include <cstddef>

namespace skr::hotswap
{
    enum class Feature
    {
        // Enable parsing of skr_require_source, skr_require_include_dir,
        // skr_require_library, skr_require_preprocessor_def, and skr_module.
        Preprocessor,

        // When encountering a skr_module, compile all source files using the same
        // module name. Then, walk the #include dependency graph to also compile
        // dependent files.
        DependentCompilation,

        // Do not automatically trigger compilation on file changes. User must call
        // the Hotswapper's TriggerManualBuild method.
        ManualCompilationOnly,
    };

    class FeatureHasher
    {
      public:
        std::size_t operator()(Feature feature) const;
    };
} // namespace skr::hotswap
