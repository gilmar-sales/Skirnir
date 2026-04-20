# Dependency Injection

Skirnir automatically resolves constructor dependencies using `Ref<T>`:

```cpp
class MyService : public IMyService
{
public:
    MyService(Ref<ITransient> transient) : mTransient(transient) {}

private:
    Ref<ITransient> mTransient;
};
```

Simply declare your dependencies as constructor parameters with `Ref<T>` type, and Skirnir will handle the rest.