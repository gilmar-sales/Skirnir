#pragma once

#include "ServiceCollection.hpp"

namespace SKIRNIR_NAMESPACE
{
    class IExtension
    {
      public:
        virtual ~IExtension() = default;

        virtual void ConfigureServices(ServiceCollection& services) {};
        virtual void UseServices(ServiceProvider& serviceProvider) {};
    };

} // namespace SKIRNIR_NAMESPACE