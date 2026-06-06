#pragma once

#include "Skirnir/LogLevel.hpp"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

namespace SKIRNIR_NAMESPACE::detail
{
    const char* LevelName(LogLevel lvl);

    std::string SanitizeForLog(std::string_view s);

    struct LogFileOpenResult
    {
        std::FILE*  file        = nullptr;
        std::size_t currentSize = 0;
    };

    /**
     * @brief Opens a log file in append mode with security hardening.
     *
     *  - Refuses to follow symlinks (POSIX) and reparse points (Windows).
     *  - Verifies the opened inode is a regular file.
     *  - On POSIX, creates the file with mode 0640 (subject to umask).
     *  - Returns the current size for rotation logic.
     *
     *  Throws std::runtime_error on any failure.
     */
    LogFileOpenResult OpenSecureLogFile(const std::filesystem::path& path);

    /**
     * @brief Rotates a log file by renaming `path` -> `path.1`,
     *        `path.1` -> `path.2`, ... up to `path.maxFiles`.
     *
     *  The caller must close the file associated with `path` before
     *  calling. `maxFiles` is the total number of rotated copies to
     *  keep; if 0, the function does nothing.
     */
    void RotateLogFile(const std::filesystem::path& path,
                       std::size_t                  maxFiles);
}
