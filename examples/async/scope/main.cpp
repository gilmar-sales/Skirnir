#include <Skirnir/Async.hpp>
#include <Skirnir/Common/Arc.hpp>
#include <Skirnir/DependencyInjection/ApplicationBuilder.hpp>
#include <Skirnir/DependencyInjection/ServiceScope.hpp>
#include <Skirnir/Logging/Logger.hpp>
#include <Skirnir/Logging/LoggingExtension.hpp>

#include <chrono>
#include <cstdio>

class IRequestContext
{
  public:
    virtual ~IRequestContext() = default;
    virtual int id() const     = 0;
};

class RequestContext : public IRequestContext
{
  public:
    explicit RequestContext(
        const skr::Arc<skr::Logger<RequestContext>>& logger) : mLogger(logger)
    {
    }
    int id() const override { return 42; }

  private:
    skr::Arc<skr::Logger<RequestContext>> mLogger;
};

class ScopeExt : public skr::IExtension
{
  protected:
    void ConfigureServices(skr::ServiceCollection& sc) override
    {
        sc.AddScoped<IRequestContext, RequestContext>();
    }
};

class ScopeApp : public skr::IAsyncApplication
{
  public:
    explicit ScopeApp(const skr::Arc<skr::ServiceProvider>& root) :
        IAsyncApplication(root)
    {
    }

    skr::Task<> RunAsync() override
    {
        auto logger = co_await mRootServiceProvider
                          ->GetServiceAsync<skr::Logger<ScopeApp>>();
        for (int i = 0; i < 3; ++i)
        {
            auto scope = mRootServiceProvider->CreateServiceScope();
            auto ctx   = co_await scope->GetServiceProvider()
                             ->GetServiceAsync<IRequestContext>();
            co_await skr::Async::Delay(std::chrono::milliseconds(5));
            logger->LogInformation("request {} -> id={}", i, ctx->id());
        }
    }
};

int main()
{
    auto app = skr::ApplicationBuilder()
                   .WithExtension<skr::LoggingExtension>()
                   .WithExtension<ScopeExt>([](ScopeExt&) {})
                   .BuildAsync<ScopeApp>();
    return skr::AsyncApplicationHost::Run(*app);
}
