#pragma once

#include <mutex>

#include "Skirnir/Logging/LogSinks/ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Writes records to standard output, optionally with ANSI color.
     */
    class ConsoleSink final : public ILogSink
    {
      public:
        explicit ConsoleSink(bool useColors = true);

        void Write(const LogRecord& record) override;

      private:
        bool              mUseColors;
        std::mutex        mMutex;
    };
} // namespace SKIRNIR_NAMESPACE
