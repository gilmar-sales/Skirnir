#pragma once

#include <functional>
#include <ranges>
#include <type_traits>

#include "Skirnir/Async/AsyncApplication.hpp"
#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/Common/ConstructorArgumentTraits.hpp"
#include "Skirnir/Common/Keyed.hpp"
#include "Skirnir/Common/LifeTime.hpp"
#include "Skirnir/Configuration.hpp"
#include "Skirnir/DependencyInjection/Application.hpp"
#include "Skirnir/DependencyInjection/Extension.hpp"

namespace SKIRNIR_NAMESPACE
{

    class ApplicationBuilder
    {
      public:
        ApplicationBuilder() :
            mServiceCollection(MakeArc<ServiceCollection>()),
            mConfigurationBuilder(MakeArc<ConfigurationBuilder>())
        {
        }

        /**
         * @brief Gets the underlying ServiceCollection.
         *
         * @return The service collection used by this builder
         */
        Arc<ServiceCollection> GetServiceCollection()
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

            configureExtensionFunc(*ArcCast<TExtension>(extension));

            return *this;
        }

        ApplicationBuilder& WithConfiguration(
            std::function<void(ConfigurationBuilder&)> configureFunc)
        {
            configureFunc(*mConfigurationBuilder);

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
        Arc<T> Build()
        {
            mServiceCollection->AddSingleton(mConfigurationBuilder->Build());

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

        /**
         * @brief Builds and returns an async application instance.
         *
         * Performs the same service-registration sequence as
         * @c Build<T>() and then resolves the application as an
         * @c IAsyncApplication. The returned application is *not*
         * automatically driven — use @c AsyncApplicationHost::Run or
         * invoke @c IAsyncApplication::RunAsync manually.
         *
         * @tparam T The application type, must derive from
         *           @c IAsyncApplication
         * @return   The resolved async application instance
         */
        template <typename T>
            requires(std::is_base_of_v<IAsyncApplication, T>)
        Arc<T> BuildAsync()
        {
            mServiceCollection->AddSingleton(mConfigurationBuilder->Build());

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
        Arc<IExtension> GetOrCreateExtension()
        {
            if (auto it = mExtensions.find(GetExtensionId<TExtension>());
                it != mExtensions.end())
            {
                return ArcCast<TExtension>(it->second);
            }

            auto extension                            = MakeArc<TExtension>();
            mExtensions[GetExtensionId<TExtension>()] = extension;

            return extension;
        }

        Arc<ServiceCollection>                 mServiceCollection;
        Arc<ConfigurationBuilder>              mConfigurationBuilder;
        std::map<ExtensionId, Arc<IExtension>> mExtensions;
    };

} // namespace SKIRNIR_NAMESPACE
