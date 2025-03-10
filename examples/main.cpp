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
    ~Repository() override = default;

    void Add() override { std::cout << "Repository::Add" << std::endl; }
};

class OtherRepository : public IRepository
{
  public:
    void Add() override {}
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

    services.AddTransient<IRepository, Repository>().AddSingleton<Singleton>();

    const auto serviceProvider = services.CreateServiceProvider();

    const auto scope = serviceProvider->CreateServiceScope();

    scope->GetServiceProvider()->GetService<std::vector<int>>();

    for (int i = 0; i < 1'000'000; ++i)
    {
        const auto repository =
            scope->GetServiceProvider()->GetService<IRepository>();
    }

    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10'000'000; ++i)
    {
        const auto repository =
            scope->GetServiceProvider()->GetService<IRepository>();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(
                     end - begin)
              << std::endl;

    begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10'000'000; ++i)
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
