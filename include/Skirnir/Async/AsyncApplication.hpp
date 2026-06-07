#pragma once

#include "Skirnir/Async/Task.hpp"
#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Common/Namespace.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Async counterpart of @c IApplication.
     *
     * Inheriting an application from this class makes it coroutine-
     * driven. The application is built and dispatched by
     * @c ApplicationBuilder::BuildAsync<T>() and is executed to
     * completion by @c AsyncApplicationHost::Run.
     *
     * The application exposes a root @c CancellationToken that can be
     * triggered with @c RequestStop to cancel in-flight work.
     */
    class IAsyncApplication
    {
      public:
        explicit IAsyncApplication(
            const Arc<ServiceProvider>& rootServiceProvider) noexcept :
            mRootServiceProvider(rootServiceProvider)
        {
        }

        virtual ~IAsyncApplication() = default;

        Arc<ServiceProvider> GetRootServiceProvider() const noexcept
        {
            return mRootServiceProvider;
        }

        /**
         * @brief Requests that @c RunAsync cooperate with
         *        cancellation. Any child @c Task<>'s @c stop_token
         *        will report stop-requested.
         */
        void RequestStop() noexcept { mStopSource.request_stop(); }

        [[nodiscard]] CancellationToken Cancellation() const noexcept
        {
            return CancellationToken { mStopSource.get_token() };
        }

        /**
         * @brief The application's coroutine body.
         */
        virtual Task<> RunAsync() = 0;

      protected:
        Arc<ServiceProvider> mRootServiceProvider;

      private:
        std::stop_source mStopSource;
    };
} // namespace SKIRNIR_NAMESPACE
