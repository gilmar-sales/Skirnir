#pragma once

#include <cstdio>
#include <filesystem>
#include <mutex>

#include "ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Appends records as plain text to a file.
     */
    class FileSink final : public ILogSink
    {
      public:
        explicit FileSink(std::filesystem::path path, bool autoFlush = true);
        ~FileSink() override;

        void Write(const LogRecord& record) override;
        void Flush() override;

        const std::filesystem::path& Path() const noexcept
        {
            return mPath;
        }

      private:
        std::filesystem::path mPath;
        std::FILE*            mFile = nullptr;
        std::mutex            mMutex;
        bool                  mAutoFlush;
    };
} // namespace SKIRNIR_NAMESPACE
