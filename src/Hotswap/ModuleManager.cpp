#include "Skirnir/Hotswap/ModuleManager.hpp"
#include "Skirnir/Hotswap/Util.hpp"
#include "Skirnir/Hotswap/module/ModuleInterface.hpp"

namespace skr::hotswap
{
    ModuleManager::ModuleManager()
    {
        Skr_GetModuleInterface()->SetIsSwapping(&m_bSwapping);
        Skr_GetModuleInterface()->SetTrackersByKey(&m_TrackersByKey);
        Skr_GetModuleInterface()->SetConstructorsByKey(&m_ConstructorsByKey);

        m_ConstructorsByKey = Skr_GetModuleInterface()->GetModuleConstructorsByKey();
        WarnDuplicateKeys(Skr_GetModuleInterface());
    }

    void ModuleManager::SetAllocator(IAllocator* pAllocator)
    {
        m_pAllocator = pAllocator;
        Skr_GetModuleInterface()->SetAllocator(pAllocator);
    }

    void ModuleManager::SetGlobalUserData(void* pGlobalUserData)
    {
        m_pGlobalUserData = pGlobalUserData;
        Skr_GetModuleInterface()->SetGlobalUserData(m_pGlobalUserData);
    }

    std::vector<ITracker*> ModuleManager::GetTrackers(const std::string& key) const
    {
        auto it = m_TrackersByKey.find(key);
        if (it == m_TrackersByKey.end())
        {
            return {};
        }
        return it->second;
    }

    void* ModuleManager::ResolveTracked(const std::string& key) const
    {
        auto it = m_TrackersByKey.find(key);
        if (it == m_TrackersByKey.end() || it->second.empty())
        {
            return nullptr;
        }
        return it->second.back();
    }

    bool ModuleManager::PerformRuntimeSwap(const fs::path& modulePath)
    {
        void* pModule = platform::LoadModule(modulePath);
        if (pModule == nullptr)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to load module "
                         << modulePath << ". " << log::LastOsError() << log::End();
            return false;
        }

        auto GetModuleInterface =
            platform::GetModuleFunction<ModuleInterface*()>(pModule,
                                                             "Skr_GetModuleInterface");
        if (GetModuleInterface == nullptr)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to load Skr_GetModuleInterface procedure. "
                         << log::LastOsError() << log::End();
            return false;
        }

        ModuleInterface* pModuleInterface = GetModuleInterface();
        if (pModuleInterface == nullptr)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to get pointer to module interface." << log::End();
            return false;
        }

        pModuleInterface->SetIsSwapping(&m_bSwapping);
        pModuleInterface->SetTrackersByKey(&m_TrackersByKey);
        pModuleInterface->SetConstructorsByKey(&m_ConstructorsByKey);
        pModuleInterface->SetAllocator(m_pAllocator);
        pModuleInterface->SetGlobalUserData(m_pGlobalUserData);
        pModuleInterface->PerformRuntimeSwap();

        WarnDuplicateKeys(pModuleInterface);

        log::Build() << SKR_HOTSWAP_LOG_PREFIX << "Successfully performed runtime swap."
                     << log::End();
        return true;
    }

    void ModuleManager::WarnDuplicateKeys(ModuleInterface* pModuleInterface)
    {
        auto duplicateKeys = pModuleInterface->GetDuplicateKeys();
        for (const auto& duplicate : duplicateKeys)
        {
            log::Warning() << SKR_HOTSWAP_LOG_PREFIX
                           << "Duplicate SKR_TRACK key detected (key="
                           << duplicate.key << ", type=" << duplicate.type
                           << log::End(").");
        }
    }
} // namespace skr::hotswap
