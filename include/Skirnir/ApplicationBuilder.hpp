#pragma once

#include "Application.hpp"
#include "Extension.hpp"
#include "Skirnir/Common.hpp"

#include <ranges>

namespace SKIRNIR_NAMESPACE
{

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() : mServiceCollection(MakeRef<ServiceCollection>())
        {
        }

        /**
         * @brief Gets the underlying ServiceCollection.
         *
         * @return The service collection used by this builder
         */
        Ref<ServiceCollection> GetServiceCollection()
        {
            return mServiceCollection;
        }

        /**
         * @brief Adds and configures an extension of the specified type.
         *
         * @tparam TExtension The extension type, must derive from IExtension
         * @return            Reference to this builder for chaining
         */
        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& WithExtension()
        {
            return WithExtension<TExtension>([](TExtension&) {});
        }

        /**
         * @brief Adds and configures an extension with a configuration
         * function.
         *
         * @tparam TExtension The extension type, must derive from IExtension
         * @param configureExtensionFunc Function to configure the extension
         * @return                       Reference to this builder for chaining
         */
        template <typename TExtension>
            requires(std::is_base_of_v<IExtension, TExtension>)
        ApplicationBuilder& WithExtension(
            std::function<void(TExtension&)> configureExtensionFunc)
        {
            auto extension = GetOrCreateExtension<TExtension>();

            extension->Attach(*this);

            configureExtensionFunc(*skr::RefCast<TExtension>(extension));

            return *this;
        }

        /**
         * @brief Builds and returns an application instance.
         *
         * Registers the application type as a singleton, creates a
         * ServiceProvider, runs all extension UseServices hooks, and returns
         * the resolved application.
         *
         * @tparam T The application type, must derive from IApplication
         * @return   The resolved application instance
         */
        template <typename T>
            requires(std::is_base_of_v<IApplication, T>)
        Ref<T> Build()
        {
            for (const auto extension : mExtensions | std::views::values)
                extension->ConfigureServices(*mServiceCollection);

            mServiceCollection->AddSingleton<T>();

            const auto serviceProvider =
                mServiceCollection->CreateServiceProvider();

            for (const auto& extension : mExtensions | std::views::values)
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
