export module Skirnir:Application;

import :Core;

export namespace skr
{

    class IApplication
    {
      public:
        explicit IApplication(const Ref<ServiceProvider>& rootServiceProvider) :
            mRootServiceProvider(rootServiceProvider)
        {
        }

        virtual ~IApplication() = default;

        [[nodiscard]] ServiceProvider& GetRootServiceProvider() const
        {
            return *mRootServiceProvider;
        }

        virtual void Run() = 0;

      protected:
        Ref<ServiceProvider> mRootServiceProvider;
    };

} // namespace skr