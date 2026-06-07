#pragma once

#include <stop_token>
#include <system_error>
#include <utility>

#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Non-owning view of a @c std::stop_source.
     *
     * Returned from @c Task::Cancellation() so user code can `co_await`
     * a Task and react to cancellation:
     * @code
     *   co_await Async::CancelIfRequested(token);
     * @endcode
     *
     * An empty token (default-constructed) is treated as "never
     * cancellable" — `co_await` on it is a no-op.
     */
    class CancellationToken
    {
      public:
        CancellationToken() noexcept = default;

        explicit CancellationToken(std::stop_token token) noexcept :
            mToken(token)
        {
        }

        [[nodiscard]] bool is_cancellation_requested() const noexcept
        {
            return mToken.stop_requested();
        }

        [[nodiscard]] std::stop_token stop_token() const noexcept
        {
            return mToken;
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return mToken.stop_possible();
        }

        void throw_if_cancellation_requested() const
        {
            if (mToken.stop_requested())
                throw std::system_error(
                    std::make_error_code(std::errc::operation_canceled));
        }

      private:
        std::stop_token mToken {};
    };
} // namespace SKIRNIR_NAMESPACE
