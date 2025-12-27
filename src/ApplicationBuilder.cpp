export module Skirnir:ApplicationBuilder;

import std;

import :Application;
export import :Extension;

export namespace skr
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
        ApplicationBuilder& AddExtension()
        {
            return AddExtension<TExtension>([](auto&) {});
        }

        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& AddExtension(
            std::function<void(TExtension&)> configureExtensionFunc)
        {
            auto extension = GetOrCreateExtension<TExtension>();

            configureExtensionFunc(
                *std::static_pointer_cast<TExtension>(extension));

            extension->Attach(*this);

            extension->ConfigureServices(*mServiceCollection);

            return *this;
        }

        template <typename T>
            requires(std::is_base_of_v<IApplication, T>)
        Ref<T> Build()
        {
            mServiceCollection->AddSingleton<T>();

            auto serviceProvider = mServiceCollection->CreateServiceProvider();

            for (const auto& [_, extension] : mExtensions)
            {
                extension->UseServices(*serviceProvider);
            }

            auto app = serviceProvider->GetService<T>();

            return app;
        }

      private:
        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        Ref<IExtension> GetOrCreateExtension()
        {
            if (auto it = mExtensions.find(GetExtensionId<TExtension>());
                it != mExtensions.end())
            {
                return it->second;
            }

            auto extension                            = MakeRef<TExtension>();
            mExtensions[GetExtensionId<TExtension>()] = extension;

            return extension;
        }

        Ref<ServiceCollection>                 mServiceCollection;
        std::map<ExtensionId, Ref<IExtension>> mExtensions;
    };

} // namespace skr
