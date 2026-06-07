#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>
#include <Skirnir/Logging/Logger.hpp>
#include <Skirnir/Logging/LoggingExtension.hpp>

#include <chrono>
#include <cstdio>

class TimerApp : public skr::IAsyncApplication
{
  public:
    explicit TimerApp(const skr::Arc<skr::ServiceProvider>& root) :
        IAsyncApplication(root)
    {
    }

    skr::Task<> RunAsync() override
    {
        auto logger = co_await mRootServiceProvider
                          ->GetServiceAsync<skr::Logger<TimerApp>>();
        for (int i = 0; i < 3; ++i)
        {
            co_await skr::Async::Delay(std::chrono::milliseconds(50));
            logger->LogInformation("tick {}", i);
        }
    }
};

int main()
{
    auto app = skr::ApplicationBuilder()
                   .WithExtension<skr::LoggingExtension>()
                   .BuildAsync<TimerApp>();

    return skr::AsyncApplicationHost::Run(*app);
}
