#pragma once

#include <type_traits>

#include "Application.hpp"
#include "Extension.hpp"

namespace SKIRNIR_NAMESPACE
{

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

        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& AddExtension(
            std::function<void(TExtension&)> configureExtensionFunc)
        {
            auto extension = GetOrCreateExtension<TExtension>();

            configureExtensionFunc(*extension);

            extension->Attach(*this);

            extension.ConfigureServices(*mServiceCollection);

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
        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        Ref<TExtension> GetOrCreateExtension()
        {
            if (auto it = mExtensions.find(GetExtensionId<TExtension>());
                it == mExtensions.end())
            {
                auto extension = MakeRef<TExtension>();
                mExtensions[GetExtensionId<TExtension>()] = extension;
                return extension;
            }

            return *it;
        }

        Ref<ServiceCollection>                 mServiceCollection;
        std::map<ExtensionId, Ref<IExtension>> mExtensions;
    };

} // namespace SKIRNIR_NAMESPACE
