#pragma once

#include "Common.hpp"
#include "ServiceProvider.hpp"

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

        Ref<ServiceProvider> GetRootServiceProvider() const
        {
            return mRootServiceProvider;
        }

        virtual void Run() = 0;

      protected:
        Ref<ServiceProvider> mRootServiceProvider;
    };
} // namespace SKIRNIR_NAMESPACE
