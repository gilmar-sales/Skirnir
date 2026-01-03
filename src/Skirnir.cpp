module;

export module Skirnir;

export import :Core;
export import :Logger;
export import :Reflection;
export import :Application;
export import :ApplicationBuilder;
export import :ServiceCollection;
export import :ServiceProvider;
export import :ServiceScope;

export namespace skr
{

    ServiceScope::ServiceScope(
        const Ref<ServiceDefinitionMap>& serviceDefinitionMap,
        const Ref<ServicesCache>&        singletonsCache) :
        mServiceDefinitionMap(serviceDefinitionMap),
        mSingletonsCache(singletonsCache), mScopeCache(MakeRef<ServicesCache>())
    {
        mServiceProvider = MakeRef<ServiceProvider>(
            mServiceDefinitionMap,
            mSingletonsCache,
            mScopeCache,
            true);
    }

    Ref<ServiceScope> ServiceProvider::CreateServiceScope()
    {
        return MakeRef<ServiceScope>(mServiceDefinitionMap, mSingletonsCache);
    };

} // namespace skr