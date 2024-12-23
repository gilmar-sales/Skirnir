#include <chrono>
#include <iostream>

#include "ServiceCollection.hpp"
#include "ServiceScope.hpp"

class IRepository {
public:
    virtual ~IRepository() = default;

    virtual void Add() = 0;
};

class Repository : public IRepository {
public:
    ~Repository() override {
    }

    void Add() override {
        std::cout << "Repository::Add" << std::endl;
    }
};

class OtherRepository : public IRepository {
public:
    void Add() override {
    }
};

class Singleton {
public:
    explicit Singleton(const std::shared_ptr<IRepository> &repository) {
        repository->Add();
    }

    ~Singleton() = default;
    void Add() {
    }
};

int main() {
    auto services = ServiceCollection();

    services.AddTransient<IRepository, Repository>();
    services.AddSingleton<Singleton>();

    const auto serviceProvider = services.CreateServiceProvider();

    const auto scope = serviceProvider->CreateServiceScope();

    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        const auto repository = scope->GetServiceProvider()->GetService<IRepository>();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin) << std::endl;

    begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100'000; ++i) {
        const auto repository = serviceProvider->GetService<Singleton>();
    }
    end = std::chrono::high_resolution_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin) << std::endl;

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.