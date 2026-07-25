#include <algorithm>

#include "Skirnir/Hotswap/preprocessor/DependencyGraph.hpp"
#include "Skirnir/Hotswap/Util.hpp"

namespace skr::hotswap
{
    std::vector<fs::path> DependencyGraph::ResolveGraph(const fs::path& filePath)
    {
        std::unordered_set<int> dependents;
        std::unordered_set<int> dependencies;
        int fileHandle = GetHandle(filePath);
        if (IsModule(fileHandle))
        {
            CollectDependents(fileHandle, dependents);
        }
        CollectDependencies(fileHandle, dependencies);
        for (int dep : dependents)
        {
            CollectDependencies(dep, dependencies);
        }
        std::unordered_set<int> all;
        all.insert(dependents.begin(), dependents.end());
        all.insert(dependencies.begin(), dependencies.end());
        std::vector<fs::path> out;
        for (int h : all)
        {
            fs::path p = GetFilepath(h);
            if (util::IsSourceFile(p)) out.push_back(p);
        }
        return out;
    }

    void DependencyGraph::SetLinkedModules(const fs::path& filePath,
                                           const std::vector<std::string>& modules)
    {
        int handle = GetHandle(filePath);
        RemoveLinkedModule(handle);
        if (!modules.empty())
        {
            for (const auto& m : modules)
            {
                m_ModulesByHandle[handle].insert(m);
                m_HandlesByModule[m].insert(handle);
            }
        }
    }

    void DependencyGraph::SetFileDependencies(const fs::path& filePath,
                                              const std::vector<fs::path>& dependencies)
    {
        int fileHandle = GetHandle(filePath);
        Node* pNode = GetNode(fileHandle);
        if (pNode == nullptr) pNode = CreateNode(filePath);
        for (int dep : pNode->dependencyHandles)
        {
            Node* pDep = GetNode(dep);
            pDep->dependentHandles.erase(fileHandle);
        }
        pNode->dependencyHandles = AsHandleSet(dependencies);
        for (int dep : pNode->dependencyHandles)
        {
            Node* pDep = GetNode(dep);
            if (pDep == nullptr) pDep = CreateNode(dep);
            pDep->dependentHandles.insert(fileHandle);
        }
    }

    void DependencyGraph::RemoveFile(const fs::path& filePath)
    {
        int fileHandle = GetHandle(filePath);
        Node* pNode = GetNode(fileHandle);
        if (pNode == nullptr) return;
        for (int dep : pNode->dependencyHandles)
        {
            Node* pDep = GetNode(dep);
            pDep->dependentHandles.erase(fileHandle);
        }
        for (int dep : pNode->dependentHandles)
        {
            Node* pDep = GetNode(dep);
            pDep->dependencyHandles.erase(fileHandle);
        }
        m_NodeByHandle.erase(m_NodeByHandle.find(fileHandle));
        RemoveLinkedModule(fileHandle);
    }

    void DependencyGraph::Clear()
    {
        m_HandlesByModule.clear();
        m_ModulesByHandle.clear();
        m_FilePathByHandle.clear();
        m_HandleByFilePath.clear();
        m_NodeByHandle.clear();
    }

    void DependencyGraph::Collect(int handle, std::unordered_set<int>& collected,
                                  const std::function<void(Node*)>& cb)
    {
        if (collected.find(handle) != collected.end()) return;
        std::vector<int> linked;
        if (IsModule(handle)) linked = GetLinkedModuleHandles(handle);
        else linked = { handle };
        for (int h : linked)
        {
            if (collected.find(h) == collected.end())
            {
                collected.insert(h);
                Node* n = GetNode(h);
                if (n != nullptr) cb(n);
            }
        }
    }

    void DependencyGraph::CollectDependencies(int handle, std::unordered_set<int>& collected)
    {
        Collect(handle, collected, [&](Node* n) {
            for (int dep : n->dependencyHandles) CollectDependencies(dep, collected);
        });
    }

    void DependencyGraph::CollectDependents(int handle, std::unordered_set<int>& collected)
    {
        Collect(handle, collected, [&](Node* n) {
            for (int dep : n->dependentHandles) CollectDependents(dep, collected);
        });
    }

    bool DependencyGraph::IsModule(int handle)
    {
        return !m_ModulesByHandle.empty() && m_ModulesByHandle.find(handle) != m_ModulesByHandle.end();
    }

    std::vector<int> DependencyGraph::GetLinkedModuleHandles(int handle)
    {
        std::unordered_set<int> out;
        auto it = m_ModulesByHandle.find(handle);
        if (it != m_ModulesByHandle.end())
        {
            for (const std::string& module : it->second)
            {
                auto handlesIt = m_HandlesByModule.find(module);
                if (handlesIt != m_HandlesByModule.end())
                {
                    for (int h : handlesIt->second) out.insert(h);
                }
            }
        }
        return std::vector<int>(out.begin(), out.end());
    }

    void DependencyGraph::RemoveLinkedModule(int handle)
    {
        auto it = m_ModulesByHandle.find(handle);
        if (it == m_ModulesByHandle.end()) return;
        std::unordered_set<std::string> old = it->second;
        m_ModulesByHandle.erase(it);
        for (const auto& m : old) m_HandlesByModule[m].erase(handle);
    }

    int DependencyGraph::CreateHandle(const fs::path& filePath)
    {
        int handle = m_NextHandle++;
        m_HandleByFilePath[filePath] = handle;
        m_FilePathByHandle[handle] = filePath;
        return handle;
    }

    int DependencyGraph::GetHandle(const fs::path& filePath)
    {
        auto it = m_HandleByFilePath.find(filePath);
        if (it != m_HandleByFilePath.end()) return it->second;
        return CreateHandle(filePath);
    }

    DependencyGraph::Node* DependencyGraph::CreateNode(const fs::path& filePath)
    {
        return CreateNode(GetHandle(filePath));
    }

    DependencyGraph::Node* DependencyGraph::CreateNode(int handle)
    {
        m_NodeByHandle[handle] = std::unique_ptr<Node>(new Node());
        return m_NodeByHandle[handle].get();
    }

    DependencyGraph::Node* DependencyGraph::GetNode(int handle)
    {
        auto it = m_NodeByHandle.find(handle);
        if (it == m_NodeByHandle.end()) return nullptr;
        return it->second.get();
    }

    fs::path DependencyGraph::GetFilepath(int handle)
    {
        auto it = m_FilePathByHandle.find(handle);
        if (it == m_FilePathByHandle.end()) return fs::path();
        return it->second;
    }

    std::unordered_set<int> DependencyGraph::AsHandleSet(const std::vector<fs::path>& paths)
    {
        std::unordered_set<int> out;
        for (const auto& p : paths) out.insert(GetHandle(p));
        return out;
    }
} // namespace skr::hotswap
