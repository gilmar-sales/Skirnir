#include <iostream>
#include <string>
#include <vector>

#include <Skirnir/Configuration.hpp>
#include <Skirnir/Skirnir.hpp>

class IPlugin
{
  public:
    virtual ~IPlugin() = default;
    virtual std::string Name() const = 0;
};

class PluginA : public IPlugin
{
  public:
    std::string Name() const override { return "PluginA"; }
};

class PluginB : public IPlugin
{
  public:
    std::string Name() const override { return "PluginB"; }
};

class PluginC : public IPlugin
{
  public:
    std::string Name() const override { return "PluginC"; }
};

class PluginHost
{
  public:
    explicit PluginHost(std::vector<skr::Arc<IPlugin>> plugins) :
        mPlugins(std::move(plugins))
    {
    }

    void List() const
    {
        for (const auto& p : mPlugins)
            std::cout << "  - " << p->Name() << "\n";
    }

  private:
    std::vector<skr::Arc<IPlugin>> mPlugins;
};

class MultiImplApp : public skr::IApplication
{
  public:
    using IApplication::IApplication;

    void Run() override
    {
        auto host = GetRootServiceProvider()->GetService<PluginHost>();
        std::cout << "Discovered PluginHost (" << host.get() << "):\n";
        host->List();
    }
};

int main()
{
    auto sc = skr::ServiceCollection()
                  .AddTransient<IPlugin, PluginA>()
                  .AddTransient<IPlugin, PluginB>()
                  .AddTransient<IPlugin, PluginC>()
                  .AddTransient<PluginHost>();

    auto sp = sc.CreateServiceProvider();

    // Single resolution returns the first registration.
    auto first = sp->GetService<IPlugin>();
    std::cout << "First: " << first->Name() << "\n";

    // Multi resolution returns all registrations.
    auto all = sp->GetServices<IPlugin>();
    std::cout << "All (" << all.size() << "):\n";
    for (const auto& p : all)
        std::cout << "  - " << p->Name() << "\n";

    return 0;
}
