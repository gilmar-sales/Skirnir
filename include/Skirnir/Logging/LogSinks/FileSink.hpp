#pragma once

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <mutex>

#include "ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Behavioural options for @c FileSink and the file variant
     *        of @c JsonSink.
     */
    struct FileSinkOptions
    {
        /**
         * @brief Soft cap on the file size in bytes.
         *
         *  When a new record would push the file past this size, the
         *  sink rotates: the current file is renamed to `path.1`
         *  (and older copies to `.2`, `.3`, ...) and a fresh file is
         *  opened. Set to 0 (the default) to disable rotation.
         */
        std::size_t maxBytes = 0;

        /**
         * @brief Number of rotated copies to retain on disk.
         *
         *  Only meaningful when @c maxBytes is non-zero. The oldest
         *  copy is deleted when the cap is reached. Must be at least
         *  1; values greater than 0 are clamped to that.
         */
        std::size_t maxFiles = 0;

        /**
         * @brief Rotate any pre-existing file on construction.
         *
         *  Useful when the caller wants each program run to start with
         *  a fresh file while keeping the previous one as `.1`.
         */
        bool rotateOnOpen = false;
    };

    /**
     * @brief Appends records as plain text to a file.
     *
     *  The file is opened in append mode with hardening against
     *  symlink/reparse-point substitution and with restrictive
     *  permissions on POSIX. If @c FileSinkOptions::maxBytes is set,
     *  the sink rotates the file on size.
     */
    class FileSink final : public ILogSink
    {
      public:
        explicit FileSink(std::filesystem::path path, bool autoFlush = true);
        FileSink(std::filesystem::path path,
                 FileSinkOptions         options,
                 bool                    autoFlush = true);
        ~FileSink() override;

        void Write(const LogRecord& record) override;
        void Flush() override;

        const std::filesystem::path& Path() const noexcept
        {
            return mPath;
        }

      private:
        FileSink(const FileSink&)            = delete;
        FileSink& operator=(const FileSink&) = delete;

        void OpenLocked();

        std::filesystem::path mPath;
        FileSinkOptions       mOptions;
        std::FILE*            mFile        = nullptr;
        std::size_t           mCurrentSize = 0;
        std::mutex            mMutex;
        bool                  mAutoFlush;
    };
} // namespace SKIRNIR_NAMESPACE
