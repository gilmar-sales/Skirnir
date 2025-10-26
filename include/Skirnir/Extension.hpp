#pragma once

#include "ServiceCollection.hpp"

namespace SKIRNIR_NAMESPACE
{
    class ApplicationBuilder;

    using ExtensionId = unsigned long;

    inline ExtensionId DependencyCount = 0;

    template <typename T>
    constexpr auto GetExtensionId() -> ExtensionId
    {
        static auto id = DependencyCount++;

        return id;
    }

    class IExtension
    {
      public:
        virtual ~IExtension() = default;

        virtual void Attach(ApplicationBuilder& applicationBuilder) {};
        virtual void ConfigureServices(ServiceCollection& services) {};
        virtual void UseServices(ServiceProvider& serviceProvider) {};
    };

} // namespace SKIRNIR_NAMESPACE