#include <benchmark/benchmark.h>

import Skirnir;
import std;

class SimpleService
{
  public:
    SimpleService() : value_(0) {}
    explicit SimpleService(int val) : value_(val) {}
    [[nodiscard]] int getValue() const { return value_; }

  private:
    int value_;
};

class DatabaseConfig
{
  public:
    DatabaseConfig() : host_("localhost"), port_(5432) {}
    [[nodiscard]] const char* getHost() const { return host_; }
    [[nodiscard]] int         getPort() const { return port_; }

  private:
    const char* host_;
    int         port_;
};

class Repository
{
  public:
    explicit Repository(std::shared_ptr<DatabaseConfig> config) :
        config_(std::move(config)), queryCount_(0)
    {
    }

    void              executeQuery() { ++queryCount_; }
    [[nodiscard]] int getQueryCount() const { return queryCount_; }

  private:
    std::shared_ptr<DatabaseConfig> config_;
    int                             queryCount_;
};

class BusinessService
{
  public:
    explicit BusinessService(std::shared_ptr<Repository> repo) :
        repo_(std::move(repo))
    {
    }

    void process() { repo_->executeQuery(); }

  private:
    std::shared_ptr<Repository> repo_;
};

// Root depends on BusinessService (3-level hierarchy)
class ApplicationRoot
{
  public:
    explicit ApplicationRoot(std::shared_ptr<BusinessService> service) :
        service_(std::move(service))
    {
    }

    void run() { service_->process(); }

  private:
    std::shared_ptr<BusinessService> service_;
};

// ============================================================================
// SCENARIO 1: MANUAL INJECTION (Baseline)
// ============================================================================

// Manual injection - Transient
static void BM_ManualInjection_Transient(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto config  = std::make_shared<DatabaseConfig>();
        auto repo    = std::make_shared<Repository>(config);
        auto service = std::make_shared<BusinessService>(repo);
        auto root    = std::make_shared<ApplicationRoot>(service);
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ManualInjection_Transient)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("Manual/Transient_3Level");

// Manual injection - Singleton (simulate by reusing instance)
static void BM_ManualInjection_Singleton(benchmark::State& state)
{
    auto config  = std::make_shared<DatabaseConfig>();
    auto repo    = std::make_shared<Repository>(config);
    auto service = std::make_shared<BusinessService>(repo);

    for (auto _ : state)
    {
        auto root = std::make_shared<ApplicationRoot>(service);
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ManualInjection_Singleton)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("Manual/Singleton_Reuse");

// Manual injection - Simple service
static void BM_ManualInjection_Simple(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto service = std::make_shared<SimpleService>(42);
        benchmark::DoNotOptimize(service);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ManualInjection_Simple)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("Manual/Simple_Transient");

// ============================================================================
// SCENARIO 2: CONSTRUCTOR INJECTION VIA IoC
// ============================================================================

// IoC collection injection - Simple Transient
static void BM_IocInjection_Simple_Transient(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<SimpleService>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto service = provider->GetService<SimpleService>();
        benchmark::DoNotOptimize(service);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_Simple_Transient)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/Simple_Transient");

// IoC collection injection - Simple Singleton
static void BM_IocInjection_Simple_Singleton(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddSingleton<SimpleService>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto service = provider->GetService<SimpleService>();
        benchmark::DoNotOptimize(service);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_Simple_Singleton)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/Simple_Singleton");

// ============================================================================
// SCENARIO 3: COMPLEX DEPENDENCY GRAPH
// ============================================================================

// IoC collection - 3-level deep graph (Transient)
static void BM_IocInjection_DeepGraph_Transient(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<DatabaseConfig>();
    collection.AddTransient<Repository>();
    collection.AddTransient<BusinessService>();
    collection.AddTransient<ApplicationRoot>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto root = provider->GetService<ApplicationRoot>();
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_DeepGraph_Transient)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DeepGraph_Transient_3Level");

// IoC collection - 3-level deep graph (Mixed: Singleton + Transient)
static void BM_IocInjection_DeepGraph_Mixed(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddSingleton<DatabaseConfig>();
    collection.AddTransient<Repository>();
    collection.AddTransient<BusinessService>();
    collection.AddTransient<ApplicationRoot>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto root = provider->GetService<ApplicationRoot>();
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_DeepGraph_Mixed)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DeepGraph_Mixed_SingletonConfig");

// IoC collection - 3-level deep graph (All Singleton)
static void BM_IocInjection_DeepGraph_AllSingleton(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddSingleton<DatabaseConfig>();
    collection.AddSingleton<Repository>();
    collection.AddSingleton<BusinessService>();
    collection.AddSingleton<ApplicationRoot>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto root = provider->GetService<ApplicationRoot>();
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_DeepGraph_AllSingleton)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DeepGraph_AllSingleton");

// ============================================================================
// SCENARIO 4: SCALING TEST - Deep Graph Penalty
// ============================================================================

// Level 1: Just DatabaseConfig
static void BM_IocInjection_Depth_1(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<DatabaseConfig>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto obj = provider->GetService<DatabaseConfig>();
        benchmark::DoNotOptimize(obj);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_Depth_1)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DepthScaling/Level1");

// Level 2: Repository -> DatabaseConfig
static void BM_IocInjection_Depth_2(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<DatabaseConfig>();
    collection.AddTransient<Repository>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto obj = provider->GetService<Repository>();
        benchmark::DoNotOptimize(obj);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_Depth_2)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DepthScaling/Level2");

// Level 3: BusinessService -> Repository -> DatabaseConfig
static void BM_IocInjection_Depth_3(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<DatabaseConfig>();
    collection.AddTransient<Repository>();
    collection.AddTransient<BusinessService>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto obj = provider->GetService<BusinessService>();
        benchmark::DoNotOptimize(obj);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_IocInjection_Depth_3)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("IoC/DepthScaling/Level3");

// ============================================================================
// SCENARIO 5: BATCH RESOLUTION
// ============================================================================

// Batch resolution test - resolve multiple services in a loop
static void BM_IocInjection_BatchResolution(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<SimpleService>();

    auto provider = collection.CreateServiceProvider();

    const int                                   batchSize = 100;
    std::vector<std::shared_ptr<SimpleService>> services;
    services.reserve(batchSize);

    for (auto _ : state)
    {
        services.clear();
        for (int i = 0; i < batchSize; ++i)
        {
            services.push_back(provider->GetService<SimpleService>());
        }
        benchmark::DoNotOptimize(services.data());
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(batchSize * state.iterations());
}
BENCHMARK(BM_IocInjection_BatchResolution)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond)
    ->Name("IoC/BatchResolution_100Items");

// ============================================================================
// SCENARIO 6: COMPARISON TESTS
// ============================================================================

// Direct comparison: Manual vs IoC for 3-level graph
static void BM_Comparison_Manual_3Level(benchmark::State& state)
{
    for (auto _ : state)
    {
        auto config  = std::make_shared<DatabaseConfig>();
        auto repo    = std::make_shared<Repository>(config);
        auto service = std::make_shared<BusinessService>(repo);
        auto root    = std::make_shared<ApplicationRoot>(service);
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Comparison_Manual_3Level)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("Comparison/Manual_3Level");

static void BM_Comparison_Ioc_3Level(benchmark::State& state)
{
    skr::ServiceCollection collection;
    collection.AddTransient<DatabaseConfig>();
    collection.AddTransient<Repository>();
    collection.AddTransient<BusinessService>();
    collection.AddTransient<ApplicationRoot>();

    auto provider = collection.CreateServiceProvider();

    for (auto _ : state)
    {
        auto root = provider->GetService<ApplicationRoot>();
        benchmark::DoNotOptimize(root);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Comparison_Ioc_3Level)
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond)
    ->Name("Comparison/IoC_3Level");

BENCHMARK_MAIN();
