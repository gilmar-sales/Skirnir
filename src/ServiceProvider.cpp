#include "Skirnir/ServiceProvider.hpp"
#include "Skirnir/ServiceScope.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace SKIRNIR_NAMESPACE
{
    Ref<ServiceScope> ServiceProvider::CreateServiceScope() const
    {
        return MakeRef<ServiceScope>(mServiceDefinitionMap, mSingletonsCache);
    };

    void ServiceProvider::ValidateOnBuild()
    {
        // Walk all unique service ids and attempt to construct each
        // singleton. Aggregated failures are thrown as a single error.
        std::vector<std::string> errors;
        std::set<ServiceId>      visited;
        std::set<ServiceId>      onStack;

        std::function<void(ServiceId)> validate;
        validate = [&](ServiceId id) {
            if (visited.contains(id))
                return;
            if (onStack.contains(id))
            {
                errors.push_back("circular dependency detected");
                return;
            }
            onStack.insert(id);

            auto range = mServiceDefinitionMap->equal_range(id);
            if (range.first == range.second)
            {
                errors.push_back("unregistered service");
                onStack.erase(id);
                return;
            }

            // Only validate the first registration (Singleton semantics).
            const auto& def = range.first->second;
            if (def.lifetime == LifeTime::Singleton)
            {
                // We rely on GetService's cycle detection to throw.
                // Catch and convert to a friendly error.
                try
                {
                    // We can't call the template GetService from here
                    // (non-template method). Instead, we attempt to
                    // construct by invoking the factory with an empty
                    // description set; missing transitive deps will throw.
                    std::set<ServiceDescription> desc;
                    def.factory(*this, desc);
                }
                catch (const std::exception& e)
                {
                    errors.push_back(e.what());
                }
            }

            onStack.erase(id);
            visited.insert(id);
        };

        for (auto& [id, def] : *mServiceDefinitionMap)
        {
            if (!visited.contains(id))
                validate(id);
        }

        if (!errors.empty())
        {
            std::string message =
                "Skirnir: ValidateOnBuild failed with the following errors:";
            for (const auto& e : errors)
            {
                message += "\n  - " + e;
            }
            throw std::runtime_error(message);
        }
    }

    void ServiceProvider::PrintDiagnostics(std::ostream& os) const
    {
        os << "Skirnir service registrations: " << mServiceDefinitionMap->size()
           << " entries across ";

        std::set<ServiceId> uniqueIds;
        for (auto& [id, def] : *mServiceDefinitionMap)
            uniqueIds.insert(id);
        os << uniqueIds.size() << " unique services\n";

        // Group registrations by id and print a tree.
        for (auto id : uniqueIds)
        {
            auto        range = mServiceDefinitionMap->equal_range(id);
            std::size_t count = static_cast<std::size_t>(
                std::distance(range.first, range.second));
            const auto& firstDef = range.first->second;

            const char* lifetime = "?";
            switch (firstDef.lifetime)
            {
                case LifeTime::Transient:
                    lifetime = "Transient";
                    break;
                case LifeTime::Scoped:
                    lifetime = "Scoped";
                    break;
                case LifeTime::Singleton:
                    lifetime = "Singleton";
                    break;
            }

            os << "  [" << lifetime << "] service#" << id;
            if (count > 1)
                os << " (" << count << " registrations)";
            os << "\n";
        }
    }
} // namespace SKIRNIR_NAMESPACE
