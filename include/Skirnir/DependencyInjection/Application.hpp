#pragma once

#include "Skirnir/Common/Arc.hpp"
#include "Skirnir/DependencyInjection/ServiceProvider.hpp"

namespace SKIRNIR_NAMESPACE
{
    class IApplication
    {
      public:
        IApplication(const Arc<ServiceProvider>& rootServiceProvider) :
            mRootServiceProvider(rootServiceProvider)
        {
        }

        virtual ~IApplication() = default;

        /**
         * @brief Gets the root ServiceProvider.
         *
         * @return The root service provider instance
         */
        Arc<ServiceProvider> GetRootServiceProvider() const
        {
            return mRootServiceProvider;
        }

        virtual void Run() = 0;

      protected:
        Arc<ServiceProvider> mRootServiceProvider;
    };
} // namespace SKIRNIR_NAMESPACE
