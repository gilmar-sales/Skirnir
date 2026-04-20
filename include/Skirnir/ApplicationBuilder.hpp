#pragma once

#include "Application.hpp"
#include "Extension.hpp"
#include "Skirnir/Common.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() : mServiceCollection(MakeRef<ServiceCollection>())
        {
        }

        Ref<ServiceCollection> GetServiceCollection()
        {
            return mServiceCollection;
        }

        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& AddExtension()
        {
            return AddExtension<TExtension>([](Ref<TExtension>) {});
        }

        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& AddExtension(
            std::function<void(Ref<TExtension>)> configureExtensionFunc)
        {
            auto extension = GetOrCreateExtension<TExtension>();

            configureExtensionFunc(extension);

            extension->Attach(*this);

            extension->ConfigureServices(mServiceCollection);

            return *this;
        }

        template <typename T>
            requires(std::is_base_of_v<IApplication, T>)
        Ref<T> Build()
        {
            mServiceCollection->AddSingleton<T>();

            auto serviceProvider = mServiceCollection->CreateServiceProvider();

            for (auto& [_, extension] : mExtensions)
            {
                extension->UseServices(serviceProvider);
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
                it != mExtensions.end())
            {
                return skr::RefCast<TExtension>(it->second);
            }

            auto extension                            = MakeRef<TExtension>();
            mExtensions[GetExtensionId<TExtension>()] = extension;

            return extension;
        }

        Ref<ServiceCollection>                 mServiceCollection;
        std::map<ExtensionId, Ref<IExtension>> mExtensions;
    };

} // namespace SKIRNIR_NAMESPACE
