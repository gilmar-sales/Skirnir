#include <chrono>
#include <iostream>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/Logging/LoggingExtension.hpp>
#include <Skirnir/Logging/LogScope.hpp>
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

    void Add() override
    {
        mLogger->LogDebug("Adding record to repository");
        mLogger->LogInformation("Add");
    }

  private:
    Ref<skr::Logger<Repository>> mLogger;
};

class OtherRepository : public IRepository
{
  public:
    OtherRepository(Ref<skr::Logger<OtherRepository>> logger) : mLogger(logger)
    {
    }

    void Add() override
    {
        mLogger->LogTrace("OtherRepository adding record");
        mLogger->LogInformation("Add");
    }

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

namespace example
{

    class ExampleApp : public skr::IApplication
    {
      public:
        ExampleApp(Ref<skr::ServiceProvider> rootServiceProvider) :
            skr::IApplication(rootServiceProvider)
        {
            mLogger =
                rootServiceProvider->GetService<skr::Logger<ExampleApp>>();
        }

        ~ExampleApp() override = default;

        void Run() override
        {
            const auto iterationCount = 100'000;

            auto options = mRootServiceProvider->GetService<skr::LoggerOptions>();
            auto requestScope = options->BeginScope("benchmark");

            auto scope = mRootServiceProvider->CreateServiceScope();

            auto begin = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < iterationCount; ++i)
            {
                const auto repository =
                    scope->GetServiceProvider()->GetService<IRepository>();
            }
            auto end = std::chrono::high_resolution_clock::now();

            mLogger->LogWarning(
                "Time to create {} repositories in scope: {}",
                iterationCount,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - begin));

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
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - begin));

            mLogger->LogInformation("benchmark run complete");
        }

      private:
        Ref<skr::Logger<ExampleApp>> mLogger;
    };
} // namespace example

// JSON configuration string for demonstration
const char* kConfigJson = R"({
    "logging": {
        "logLevel": {
            "default": "Warning",
            "example": "Debug",
            "example::ExampleApp": "Information"
        }
    }
})";

class ExampleExtension final : public skr::IExtension
{
  protected:
    void ConfigureServices(skr::ServiceCollection& services) override
    {
        // Create configuration from JSON
        auto config =
            skr::ConfigurationBuilder().AddJsonString(kConfigJson).Build();

        // Create LoggerOptions and configure from JSON
        services
            .AddSingleton<skr::LoggerOptions>([config](skr::ServiceProvider&) {
                auto options = skr::MakeRef<skr::LoggerOptions>();
                options->ConfigureFrom(config);
                return options;
            })
            .AddTransient<IRepository, Repository>()
            .AddSingleton<Singleton>();
    }
};

int main()
{
    std::cout << "=== Skirnir Configuration Example ===" << std::endl;
    std::cout << "Config: " << kConfigJson << std::endl;
    std::cout << "=====================================" << std::endl;

    auto appBuilder =
        skr::ApplicationBuilder()
            .WithExtension<ExampleExtension>(
                [](ExampleExtension& exampleExtension) {

                })
            .WithExtension<ExampleExtension>()
            .WithExtension<skr::LoggingExtension>(
                [](skr::LoggingExtension& logging) {
                    logging.AddConsoleSink().AddJsonSink("app.ndjson");
                });

    appBuilder.Build<example::ExampleApp>()->Run();

    return 0;
}
