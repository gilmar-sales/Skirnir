#pragma once

#include <type_traits>

#include "ServiceCollection.hpp"

namespace SKIRNIR_NAMESPACE
{
    class IApplication
    {
      public:
        IApplication(const Ref<ServiceProvider>& rootServiceProvider) :
            mRootServiceProvider(rootServiceProvider)
        {
        }

        virtual ~IApplication() = default;

        virtual void Run() = 0;

      protected:
        Ref<ServiceProvider> mRootServiceProvider;
    };

    class IExtension
    {
      public:
        virtual ~IExtension() = default;

        virtual void ConfigureServices(ServiceCollection& services) {};
        virtual void UseServices(ServiceProvider& serviceProvider) {};
    };

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() : mServiceCollection(MakeRef<ServiceCollection>())
        {
        }

        template <typename T>
            requires(std::is_base_of_v<IExtension, T>)
        ApplicationBuilder& AddExtension(T extension)
        {
            extension.ConfigureServices(*mServiceCollection);

            mExtensions.push_back(MakeRef<T>(std::move(extension)));

            return *this;
        }

        template <typename T>
            requires(std::is_base_of_v<IApplication, T>)
        Ref<T> Build()
        {
            mServiceCollection->AddSingleton<T>();

            auto serviceProvider = mServiceCollection->CreateServiceProvider();

            for (const auto& extension : mExtensions)
            {
                extension->UseServices(*serviceProvider);
            }

            auto app = serviceProvider->GetService<T>();

            return app;
        }

      private:
        Ref<ServiceCollection>       mServiceCollection;
        std::vector<Ref<IExtension>> mExtensions;
    };

} // namespace SKIRNIR_NAMESPACE
