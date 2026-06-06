#pragma once

#include "Skirnir/DependencyInjection/Extension.hpp"
#include "Skirnir/Logging/LogRecord.hpp"
#include "Skirnir/Logging/LogSinks.hpp"
#include "Skirnir/Logging/Logger.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <utility>
#include <vector>

namespace SKIRNIR_NAMESPACE
{
    /**
     * @brief Fluent extension that composes log sinks before the
     *        @c ServiceProvider is built.
     *
     * @code
     * ApplicationBuilder()
     *     .WithExtension<LoggingExtension>([](LoggingExtension& l) {
     *         l.AddConsoleSink()
     *          .AddFileSink("app.log")
     *          .AddJsonSink("app.ndjson")
     *          .WithAsyncQueue();
     *     })
     *     .Build<MyApp>();
     * @endcode
     *
     * If no sinks are added, the default @c ConsoleSink is registered
     * lazily on first dispatch (matches the pre-extension behavior).
     */
    class LoggingExtension final : public IExtension
    {
      public:
        LoggingExtension& AddConsoleSink()
        {
            mSinkBuilders.emplace_back([](Ref<LoggerOptions> o) {
                o->AddSink(MakeRef<ConsoleSink>());
            });
            return *this;
        }

        LoggingExtension& AddFileSink(std::filesystem::path path,
                                      bool                  autoFlush = true)
        {
            mSinkBuilders.emplace_back(
                [path = std::move(path), autoFlush](Ref<LoggerOptions> o) {
                    o->AddSink(MakeRef<FileSink>(path, autoFlush));
                });
            return *this;
        }

        LoggingExtension& AddJsonSink(std::filesystem::path path)
        {
            mSinkBuilders.emplace_back(
                [path = std::move(path)](Ref<LoggerOptions> o) {
                    o->AddSink(MakeRef<JsonSink>(path));
                });
            return *this;
        }

        LoggingExtension& WithAsyncQueue(std::size_t capacity = 8192)
        {
            mWrapAsync = capacity;
            return *this;
        }

        void ConfigureServices(ServiceCollection& sc) override
        {
            // Ensure LoggerOptions exists.
            if (!sc.Contains<LoggerOptions>())
            {
                sc.AddSingleton<LoggerOptions>();
            }
        }

        void UseServices(ServiceProvider& sp) override
        {
            auto options = sp.GetService<LoggerOptions>();
            for (auto& builder : mSinkBuilders)
            {
                builder(options);
            }
            if (mWrapAsync)
            {
                std::vector<Ref<ILogSink>> current = options->Sinks();
                options->ClearSinks();
                auto inner = MakeRef<CompositeSink>(std::move(current));
                options->AddSink(
                    MakeRef<AsyncSink>(std::move(inner), *mWrapAsync));
            }
        }

      private:
        class CompositeSink final : public ILogSink
        {
          public:
            explicit CompositeSink(std::vector<Ref<ILogSink>> sinks) :
                mSinks(std::move(sinks))
            {
            }
            void Write(const LogRecord& r) override
            {
                for (auto& s : mSinks)
                    s->Write(r);
            }
            void Flush() override
            {
                for (auto& s : mSinks)
                    s->Flush();
            }

          private:
            std::vector<Ref<ILogSink>> mSinks;
        };

        std::vector<std::function<void(Ref<LoggerOptions>)>> mSinkBuilders;
        std::optional<std::size_t>                           mWrapAsync;
    };
} // namespace SKIRNIR_NAMESPACE
