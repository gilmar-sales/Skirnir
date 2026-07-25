#include "Skirnir/Hotswap/preprocessor/VarStore.hpp"
#include "Skirnir/Hotswap/Util.hpp"

namespace skr::hotswap
{
    void VarStore::SetVar(const std::string& name, const Variant& val)
    {
        m_Vars[util::Trim(name)] = val;
    }

    bool VarStore::GetVar(const std::string& name, Variant& val) const
    {
        auto it = m_Vars.find(util::Trim(name));
        if (it == m_Vars.end()) return false;
        val = it->second;
        return true;
    }

    bool VarStore::RemoveVar(const std::string& name)
    {
        auto it = m_Vars.find(util::Trim(name));
        if (it == m_Vars.end()) return false;
        m_Vars.erase(it);
        return true;
    }

    std::string VarStore::Interpolate(const std::string& str) const
    {
        std::string result = str;
        const std::string startPat = "${";
        const std::string endPat = "}";

        std::size_t iStart = result.find(startPat);
        while (iStart != std::string::npos)
        {
            std::size_t iEnd = result.find(endPat, iStart + 1);
            if (iEnd != std::string::npos)
            {
                std::size_t matchStart = iStart + startPat.size();
                std::size_t matchEnd = iEnd - endPat.size();
                std::string varName =
                    result.substr(matchStart, matchEnd - matchStart + 1);
                varName = util::Trim(varName);

                auto it = m_Vars.find(varName);
                if (it != m_Vars.end())
                {
                    result.replace(iStart, iEnd - iStart + 1,
                                    it->second.ToString());
                }
            }
            iStart = result.find(startPat, iStart + 1);
        }
        return result;
    }
} // namespace skr::hotswap
