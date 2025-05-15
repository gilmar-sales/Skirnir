#include <chrono>
#include <iostream>

#include <Skirnir/Skirnir.hpp>

class IRepository
{
  public:
    virtual ~IRepository() = default;

    virtual void Add() = 0;
};

class Singleton;

class Repository : public IRepository
{
  public:
    Repository(Ref<skr::Logger<Repository>> logger) : mLogger(logger) {}
    ~Repository() override = default;

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Ref<skr::Logger<Repository>> mLogger;
};

class OtherRepository : public IRepository
{
  public:
    OtherRepository(Ref<skr::Logger<OtherRepository>> logger) : mLogger(logger)
    {
    }

    void Add() override { mLogger->LogInformation("Add"); }

  private:
    Ref<skr::Logger<OtherRepository>> mLogger;
};

class Singleton
{
  public:
    explicit Singleton() {}

    ~Singleton() = default;

    void Add() {}
};

class ExampleApp : public skr::IApplication
{
  public:
    ExampleApp(Ref<skr::ServiceProvider> rootServiceProvider) :
        skr::IApplication(rootServiceProvider)
    {
        mLogger = rootServiceProvider->GetService<skr::Logger<ExampleApp>>();
    }

    ~ExampleApp() override = default;

    void Run() override
    {
        const auto iterationCount = 100'000;

        const auto scope = mRootServiceProvider->CreateServiceScope();

        auto begin = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterationCount; ++i)
        {
            const auto repository =
                scope->GetServiceProvider()->GetService<IRepository>();
        }
        auto end = std::chrono::high_resolution_clock::now();

        mLogger->LogInformation(
            "Time to create {} repositories in scope: {}",
            iterationCount,
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin));

        begin = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterationCount; ++i)
        {
            const auto repository =
                mRootServiceProvider->GetService<Singleton>();
        }
        end = std::chrono::high_resolution_clock::now();

        mLogger->LogInformation(
            "Time to create {} singletons in root: {}",
            iterationCount,
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin));
    }

  private:
    Ref<skr::Logger<ExampleApp>> mLogger;
};

class ExampleExtension : public skr::IExtension
{
  public:
    void ConfigureServices(skr::ServiceCollection& services) const override
    {
        services
            .AddSingleton<skr::LoggerOptions>([](skr::ServiceProvider&) {
                auto options      = skr::MakeRef<skr::LoggerOptions>();
                options->logLevel = skr::LogLevel::Information;
                return options;
            })
            .AddTransient<IRepository, Repository>()
            .AddSingleton<Singleton>();
    }
};

int main()
{
    auto appBuilder =
        skr::ApplicationBuilder().AddExtension(ExampleExtension());

    appBuilder.Build<ExampleApp>()->Run();

    return 0;
}
