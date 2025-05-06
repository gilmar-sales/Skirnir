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

        virtual void Configure(ServiceCollection& services) = 0;
    };

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() : mServiceCollection(MakeRef<ServiceCollection>())
        {
        }

        template <typename T>
            requires(std::is_base_of_v<IExtension, T>)
        ApplicationBuilder& AddExtension()
        {
            auto extension = MakeRef<T>();

            extension->Configure(*mServiceCollection);

            return *this;
        }

        template <typename T>
            requires(std::is_base_of_v<IApplication, T>)
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
