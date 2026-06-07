#pragma once

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>

#include "Skirnir/Async/AsyncApplication.hpp"
#include "Skirnir/Async/Scheduler.hpp"
#include "Skirnir/Async/Task.hpp"
#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Common/Namespace.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Drives an @c IAsyncApplication to completion.
     *
     * Holds the application's root @c Task<> and the @c Scheduler used
     * to resume it. Used as the async equivalent of calling
     * @c IApplication::Run().
     */
    class AsyncApplicationHost
    {
      public:
        template <Scheduler S = InlineScheduler>
        static int Run(IAsyncApplication& app, S scheduler = S {})
        {
            auto task = app.RunAsync();
            runToCompletion(task, scheduler);
            return 0;
        }

        template <Scheduler S = InlineScheduler>
        static int Run(const Arc<IAsyncApplication>& app, S scheduler = S {})
        {
            return Run(*app, scheduler);
        }

        template <Scheduler S = InlineScheduler>
        static void runToCompletion(Task<void>& task, S scheduler = S {})
        {
            if (!task.valid())
                return;
            while (!task.is_ready())
            {
                if (!task.handle().promise().scheduler.empty())
                    task.handle().promise().scheduler(task.handle());
                else
                    task.handle().resume();
            }
            (void) scheduler;
        }
    };
} // namespace SKIRNIR_NAMESPACE
