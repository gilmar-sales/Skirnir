#pragma once

#include "Skirnir/Logging/LogSinks/ILogSink.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Discards every record. Useful in tests and for silencing
     *        a particular category.
     */
    class NullSink final : public ILogSink
    {
      public:
        void Write(const LogRecord&) override {}
    };
} // namespace SKIRNIR_NAMESPACE
