#pragma once

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <ostream>

#include "FileSink.hpp"
#include "ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Emits records as NDJSON (one JSON object per line) to either
     *        an existing @c std::ostream or a file path.
     *
     *  When constructed with a path, the file is opened in **append**
     *  mode (not truncating) and is hardened against symlink/reparse-
     *  point substitution. Optional @c FileSinkOptions (rotation, etc.)
     *  apply to the file variant.
     */
    class JsonSink final : public ILogSink
    {
      public:
        explicit JsonSink(std::ostream& os);
        explicit JsonSink(std::filesystem::path path);
        JsonSink(std::filesystem::path path, FileSinkOptions options);

        void Write(const LogRecord& record) override;
        void Flush() override;

        ~JsonSink() override;

      private:
        JsonSink(const JsonSink&)            = delete;
        JsonSink& operator=(const JsonSink&) = delete;

        std::FILE*       mFile = nullptr;   // owned when non-null
        std::ostream*    mOs   = nullptr;   // borrowed when non-null
        std::filesystem::path mPath;
        FileSinkOptions  mOptions;
        std::size_t      mCurrentSize = 0;
        std::mutex       mMutex;
    };
} // namespace SKIRNIR_NAMESPACE
