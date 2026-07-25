#pragma once

#include <functional>

namespace skr::hotswap
{
    class ProtectedFunction
    {
      public:
        enum class Result
        {
            Success,
            Exception,
        };

        static Result Call(const std::function<void()>& cb);
    };
} // namespace skr::hotswap
