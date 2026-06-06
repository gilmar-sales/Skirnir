# Dependency Injection

Skirnir automatically resolves constructor dependencies using `Arc<T>`:

```cpp
class MyService : public IMyService
{
public:
    MyService(Arc<ITransient> transient) : mTransient(transient) {}

private:
    Arc<ITransient> mTransient;
};
```

Simply declare your dependencies as constructor parameters with `Arc<T>` type, and Skirnir will handle the rest.