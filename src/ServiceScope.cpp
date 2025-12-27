export module Skirnir:ServiceScope;

export import :Core;
export import :ServiceProvider;

export namespace skr
{

    class ServiceScope
    {
      public:
        ServiceScope(const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
                     const Ref<ServicesCache>&        singletonsCache);

        Ref<ServiceProvider> GetServiceProvider() { return mServiceProvider; };

      private:
        Ref<ServiceProvider>      mServiceProvider;
        Ref<ServiceDefinitionMap> mServiceDefinitionMap;
        Ref<ServicesCache>        mSingletonsCache;
        Ref<ServicesCache>        mScopeCache;
    };
} // namespace skr