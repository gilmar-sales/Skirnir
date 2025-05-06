#pragma once

#include <type_traits>

#include "ServiceCollection.hpp"

namespace SKIRNIR_NAMESPACE
{
    class Application
    {
      public:
        Application(const Ref<ServiceProvider>& rootServiceProvider) :
            mRootServiceProvider(rootServiceProvider)
        {
        }

        virtual ~Application() = default;

        virtual void Run() = 0;

      protected:
        Ref<ServiceProvider> mRootServiceProvider;
    };

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() : mServiceCollection(MakeRef<ServiceCollection>())
        {
        }

        ServiceCollection& GetServiceCollection()
        {
            return *mServiceCollection;
        }

        template <typename T>
            requires(std::is_base_of_v<Application, T>)
        Ref<T> Build()
        {
            mServiceCollection->AddSingleton<T>();

            auto serviceProvider = mServiceCollection->CreateServiceProvider();

            auto app = serviceProvider->GetService<T>();

            return app;
        }

      private:
        Ref<ServiceCollection> mServiceCollection;
    };

} // namespace SKIRNIR_NAMESPACE
