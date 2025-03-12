#include <chrono>
#include <iostream>

#include <Skirnir/Skirnir.hpp>

class IRepository
{
  public:
    virtual ~IRepository() = default;

    virtual void Add() = 0;
};

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
    explicit Singleton(const std::shared_ptr<IRepository>& repository)
    {
        repository->Add();
    }

    ~Singleton() = default;

    void Add() {}
};

int main()
{
    auto services = skr::ServiceCollection();

    services
        .AddSingleton<skr::LoggerOptions>([](skr::ServiceProvider&) {
            auto options      = skr::MakeRef<skr::LoggerOptions>();
            options->logLevel = skr::LogLevel::Information;
            return options;
        })
        .AddTransient<IRepository, Repository>()
        .AddSingleton<Singleton>();

    const auto serviceProvider = services.CreateServiceProvider();

    const auto scope = serviceProvider->CreateServiceScope();

    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100'000; ++i)
    {
        const auto repository =
            scope->GetServiceProvider()->GetService<IRepository>();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end - begin)
              << std::endl;

    begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100'000; ++i)
    {
        const auto repository = serviceProvider->GetService<Singleton>();
    }
    end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end - begin)
              << std::endl;

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.
