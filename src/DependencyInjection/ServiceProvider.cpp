#include "Skirnir/DependencyInjection/ServiceProvider.hpp"
#include "Skirnir/DependencyInjection/ServiceScope.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace SKIRNIR_NAMESPACE
{
    namespace
    {
        std::optional<LifeTime> LifetimeForId(const ServiceDefinitionMap& map,
                                              ServiceId                    id)
        {
            auto range = map.equal_range(id);
            if (range.first == range.second)
                return std::nullopt;
            return range.first->second.lifetime;
        }

        std::string LifetimeName(LifeTime lt)
        {
            switch (lt)
            {
                case LifeTime::Transient:
                    return "Transient";
                case LifeTime::Scoped:
                    return "Scoped";
                case LifeTime::Singleton:
                    return "Singleton";
            }
            return "?";
        }
    } // namespace

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

        // Captive-dependency detection: a Singleton whose transitive
        // constructor dependencies include a Scoped service. Such a Scoped
        // instance ends up being captured by the long-lived Singleton,
        // breaking the lifetime contract and almost always indicating a
        // bug.
        std::set<ServiceId> captiveChecked;
        for (auto& [id, def] : *mServiceDefinitionMap)
        {
            if (def.lifetime != LifeTime::Singleton)
                continue;
            if (!captiveChecked.insert(id).second)
                continue;

            for (ServiceId depId : def.ctorDeps)
            {
                if (depId == id)
                    continue;

                auto depLt = LifetimeForId(*mServiceDefinitionMap, depId);
                if (depLt && *depLt == LifeTime::Scoped)
                {
                    std::ostringstream oss;
                    oss << "Captive dependency: Singleton service#"
                        << id
                        << " depends on Scoped service#"
                        << depId
                        << " — Scoped services must not be captured by a "
                           "Singleton.";
                    errors.push_back(oss.str());
                }
            }
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

            os << "  [" << LifetimeName(firstDef.lifetime) << "] service#"
               << id;
            if (count > 1)
                os << " (" << count << " registrations)";
            os << "\n";

            std::size_t idx = 0;
            for (auto it = range.first; it != range.second; ++it, ++idx)
            {
                os << "      registration #" << idx;
                if (!it->second.key.empty())
                    os << " key=\"" << it->second.key << "\"";
                os << ", ctor deps: [";
                for (std::size_t i = 0; i < it->second.ctorDeps.size(); ++i)
                {
                    if (i)
                        os << ", ";
                    os << "service#" << it->second.ctorDeps[i];
                }
                os << "]\n";
            }
        }
    }
} // namespace SKIRNIR_NAMESPACE
