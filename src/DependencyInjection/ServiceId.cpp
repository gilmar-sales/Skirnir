#include "Skirnir/DependencyInjection/ServiceId.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

namespace SKIRNIR_NAMESPACE
{
    ServiceId RegisterTypeName(std::string_view typeName)
    {
        static std::mutex                              mutex;
        static std::unordered_map<std::string, ServiceId> ids;
        static ServiceId                               nextId = 0;

        std::lock_guard lock(mutex);

        const auto [it, inserted] =
            ids.try_emplace(std::string(typeName), nextId);
        if (inserted)
        {
            ++nextId;
        }

        return it->second;
    }
} // namespace SKIRNIR_NAMESPACE
