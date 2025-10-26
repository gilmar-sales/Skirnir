#pragma once

#include "ServiceCollection.hpp"

namespace SKIRNIR_NAMESPACE
{
    using ExtensionId = unsigned long;

    inline ExtensionId ExtensionCount = 0;

    template <typename T>
    constexpr auto GetExtensionId() -> ExtensionId
    {
        static auto id = ExtensionCount++;

        return id;
    }

    class IExtension
    {
      public:
        virtual ~IExtension() = default;

      private:
        friend class ApplicationBuilder;

        virtual void Attach(ApplicationBuilder& applicationBuilder) {};
        virtual void ConfigureServices(ServiceCollection& services) {};
        virtual void UseServices(ServiceProvider& serviceProvider) {};
    };

} // namespace SKIRNIR_NAMESPACE