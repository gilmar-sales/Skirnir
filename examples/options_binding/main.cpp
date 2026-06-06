#include <iostream>
#include <ranges>

#include <Skirnir/Skirnir.hpp>

struct DatabaseOptions
{
    std::string host    = "localhost";
    int         port    = 5432;
    bool        ssl     = false;
    double      timeout = 30.0;
};

class OptionsBindingApp : public skr::IApplication
{
  public:
    OptionsBindingApp(
        const skr::Arc<skr::ServiceProvider>& rootServiceProvider) :
        IApplication(rootServiceProvider),
        mDataBaseOptions(rootServiceProvider->GetService<DatabaseOptions>()),
        mLogger(
            rootServiceProvider->GetService<skr::Logger<OptionsBindingApp>>())
    {
    }

    void Run() override
    {
        mLogger->LogInformation("host={}", mDataBaseOptions->host);
        mLogger->LogInformation("port={}", mDataBaseOptions->port);
        mLogger->LogInformation("ssl={}", mDataBaseOptions->ssl);
        mLogger->LogInformation("timeout={}", mDataBaseOptions->timeout);
    }

  private:
    skr::Arc<DatabaseOptions>                mDataBaseOptions;
    skr::Arc<skr::Logger<OptionsBindingApp>> mLogger;
};

int main()
{
    const char* json = R"({
        "Database": {
            "host": "db.example.com",
            "port": 3306,
            "ssl": true,
            "timeout": 5.5
        }
    })";

    auto builder = skr::ApplicationBuilder().WithConfiguration(
        [&](skr::ConfigurationBuilder& configurationBuilder) {
            configurationBuilder.AddJsonString(json);
        });

    builder.GetServiceCollection()->AddSingleton<DatabaseOptions>(
        [](skr::ServiceProvider& serviceProvider) {
            return serviceProvider.GetService<skr::ConfigurationOptions>()
                ->Bind<DatabaseOptions>("Database");
        });

    auto app = builder.Build<OptionsBindingApp>();

    app->Run();

    return 0;
}
