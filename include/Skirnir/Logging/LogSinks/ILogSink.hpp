#pragma once

#include "../LogRecord.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief A destination for log records.
     *
     * Implementations must be thread-safe; @c Logger<T> dispatches from
     * arbitrary threads.
     */
    class ILogSink
    {
      public:
        virtual ~ILogSink() = default;
        virtual void Write(const LogRecord& record) = 0;
        virtual void Flush()
        {
        }
    };
} // namespace SKIRNIR_NAMESPACE
