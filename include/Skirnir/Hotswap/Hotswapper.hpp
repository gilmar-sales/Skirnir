#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <memory>
#include <functional>

#include "Skirnir/Hotswap/Platform.hpp"
#include "Skirnir/Hotswap/file-watcher/IFileWatcher.hpp"
#include "Skirnir/Hotswap/compiler/ICompiler.hpp"
#include "Skirnir/Hotswap/ModuleManager.hpp"
#include "Skirnir/Hotswap/module/AllocationResolver.hpp"
#include "Skirnir/Hotswap/Feature.hpp"
#include "Skirnir/Hotswap/ProtectedFunction.hpp"
#include "Skirnir/Hotswap/Callbacks.hpp"
#include "Skirnir/Hotswap/FeatureManager.hpp"
#include "Skirnir/Hotswap/FsPathHasher.hpp"
#include "Skirnir/Hotswap/Config.hpp"
#include "Skirnir/Hotswap/preprocessor/IPreprocessor.hpp"
#include "Skirnir/Hotswap/Log.hpp"
#include "Skirnir/Hotswap/Util.hpp"

namespace skr::hotswap
{
    class Hotswapper
    {
      public:
        enum class UpdateResult
        {
            Idle,
            Compiling,
            StartedCompiling,
            PerformedSwap,
            FailedSwap,
        };

        Hotswapper();
        explicit Hotswapper(std::unique_ptr<Config> pConfig);
        Hotswapper(std::unique_ptr<Config> pConfig,
                   std::unique_ptr<IFileWatcher> pFileWatcher,
                   std::unique_ptr<ICompiler> pCompiler,
                   std::unique_ptr<IPreprocessor> pPreprocessor);

        AllocationResolver* GetAllocationResolver();

        void SetAllocator(IAllocator* pAllocator);
        void SetGlobalUserData(void* pGlobalUserData);

        void EnableFeature(Feature feature);
        void DisableFeature(Feature feature);
        bool IsFeatureEnabled(Feature feature);

        void TriggerManualBuild();

        UpdateResult Update();
        bool IsCompiling();
        bool IsCompilerInitialized();

        void SetCallbacks(const Callbacks& callbacks);
        void DoProtectedCall(const std::function<void()>& cb);

        //============================================================================
        // Add & Remove Functions
        //============================================================================

        int AddIncludeDirectory(const fs::path& directoryPath);
        bool RemoveIncludeDirectory(int handle);
        void EnumerateIncludeDirectories(
            const std::function<void(int handle, const fs::path& directoryPath)>& cb);
        void ClearIncludeDirectories();

        int AddSourceDirectory(const fs::path& directoryPath);
        bool RemoveSourceDirectory(int handle);
        void EnumerateSourceDirectories(
            const std::function<void(int handle, const fs::path& directoryPath)>& cb);
        void ClearSourceDirectories();

        int AddForceCompiledSourceFile(const fs::path& filePath);
        bool RemoveForceCompiledSourceFile(int handle);
        void EnumerateForceCompiledSourceFiles(
            const std::function<void(int handle, const fs::path& filePath)>& cb);
        void ClearForceCompiledSourceFiles();

        int AddLibraryDirectory(const fs::path& directoryPath);
        bool RemoveLibraryDirectory(int handle);
        void EnumerateLibraryDirectories(
            const std::function<void(int handle, const fs::path& directoryPath)>& cb);
        void ClearLibraryDirectories();

        int AddLibrary(const fs::path& libraryPath);
        int LocateAndAddLibrary(const fs::path& rootPath, const fs::path& libraryName);
        bool RemoveLibrary(int handle);
        void EnumerateLibraries(
            const std::function<void(int handle, const fs::path& libraryPath)>& cb);
        void ClearLibraries();

        int AddPreprocessorDefinition(const std::string& definition);
        bool RemovePreprocessorDefinition(int handle);
        void EnumeratePreprocessorDefinitions(
            const std::function<void(int handle, const std::string& definition)>& cb);
        void ClearPreprocessorDefinitions();

        int AddCompileOption(const std::string& option);
        bool RemoveCompileOption(int handle);
        void EnumerateCompileOptions(
            const std::function<void(int handle, const std::string& option)>& cb);
        void ClearCompileOptions();

        int AddLinkOption(const std::string& option);
        bool RemoveLinkOption(int handle);
        void EnumerateLinkOptions(
            const std::function<void(int handle, const std::string& option)>& cb);
        void ClearLinkOptions();

        void SetVar(const std::string& name, const std::string& val);
        void SetVar(const std::string& name, const char* pVal);
        void SetVar(const std::string& name, double val);
        void SetVar(const std::string& name, bool val);
        bool RemoveVar(const std::string& name);

        // Returns the internal ModuleManager so that higher-level integrations
        // (e.g. skr::ServiceCollection::AddHotSwapper) can resolve tracked
        // instances by key.
        ModuleManager& GetModuleManager() { return m_ModuleManager; }

#ifndef SKR_DISABLE_HOTSWAP
      private:
#endif
        std::unique_ptr<Config> m_pConfig;

        fs::path m_HotswapTempDirectoryPath;

        int m_NextIncludeDirectoryHandle = 0;
        int m_NextSourceDirectoryHandle = 0;
        int m_NextForceCompiledSourceFileHandle = 0;
        int m_NextLibraryDirectoryHandle = 0;
        int m_NextLibraryHandle = 0;
        int m_NextPreprocessorDefinitionHandle = 0;
        int m_NextCompileOptionHandle = 0;
        int m_NextLinkOptionHandle = 0;

        fs::path m_BuildDirectoryPath;

        std::map<int, fs::path> m_IncludeDirectoryPathsByHandle;
        std::map<int, fs::path> m_SourceDirectoryPathsByHandle;
        std::map<int, fs::path> m_ForceCompiledSourceFilePathsByHandle;
        std::map<int, fs::path> m_LibraryDirectoryPathsByHandle;
        std::map<int, fs::path> m_LibraryPathsByHandle;
        std::map<int, std::string> m_PreprocessorDefinitionsByHandle;
        std::map<int, std::string> m_CompileOptionsByHandle;
        std::map<int, std::string> m_LinkOptionsByHandle;

        std::unique_ptr<IFileWatcher> m_pFileWatcher;
        std::vector<IFileWatcher::Event> m_FileEvents;

        std::unique_ptr<ICompiler> m_pCompiler;
        std::unique_ptr<IPreprocessor> m_pPreprocessor;

        ModuleManager m_ModuleManager;
        FeatureManager m_FeatureManager;

        bool m_bDependencyGraphNeedsRefresh = true;

        AllocationResolver m_AllocationResolver;
        Callbacks m_Callbacks;

#ifndef SKR_DISABLE_HOTSWAP
        bool StartCompile(ICompiler::Input& compilerInput);

        bool CreateCompilerInput(const std::vector<fs::path>& sourceFilePaths,
                                 ICompiler::Input& compilerInput);
        bool Preprocess(ICompiler::Input& compilerInput);
        void Deduplicate(ICompiler::Input& input);

        bool PerformRuntimeSwap();

        bool CreateHotswapTempDirectory();
        bool CreateBuildDirectory();

        void UpdateDependencyGraph(
            const std::vector<fs::path>& canonicalModifiedFilePaths,
            const std::vector<fs::path>& canonicalRemovedFilePaths);
        void RefreshDependencyGraph();
#endif

        template <typename TDirectoryIterator>
        void AppendDirectoryFiles(
            const std::map<int, fs::path>& directoryPathsByHandle,
            std::unordered_set<fs::path, FsPathHasher>& sourceFilePaths);

        template <typename T>
        int Add(const T& value, int& handle, std::map<int, T>& map);

        template <typename T>
        bool Remove(int handle, std::map<int, T>& map);

        template <typename T>
        void Enumerate(const std::function<void(int handle, const T& value)>& cb,
                       std::map<int, T>& map);

        template <typename T>
        std::vector<T> AsVector(std::map<int, T>& map);
    };

    template <typename TDirectoryIterator>
    void Hotswapper::AppendDirectoryFiles(
        const std::map<int, fs::path>& directoryPathsByHandle,
        std::unordered_set<fs::path, FsPathHasher>& sourceFilePaths)
    {
        for (const auto& handle__directoryPath : directoryPathsByHandle)
        {
            std::error_code error;
            auto directoryIterator =
                TDirectoryIterator(handle__directoryPath.second, error);

            if (error.value() != 0)
            {
                log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Unable to iterate directory "
                             << handle__directoryPath.second << log::End(".");
                return;
            }

            for (const auto& filePath : fs::directory_iterator(handle__directoryPath.second))
            {
                if (util::IsSourceFile(filePath) || util::IsHeaderFile(filePath))
                {
                    fs::path canonicalFilePath = fs::canonical(filePath, error);
                    if (error.value() == 0)
                    {
                        sourceFilePaths.insert(filePath);
                    }
                }
            }
        }
    }

    template <typename T>
    int Hotswapper::Add(const T& value, int& handle, std::map<int, T>& map)
    {
        int curHandle = handle++;
        map[curHandle] = value;
        return curHandle;
    }

    template <typename T>
    bool Hotswapper::Remove(int handle, std::map<int, T>& map)
    {
        auto it = map.find(handle);
        if (it == map.end())
        {
            return false;
        }
        map.erase(it);
        return true;
    }

    template <typename T>
    void Hotswapper::Enumerate(const std::function<void(int handle, const T& value)>& cb,
                               std::map<int, T>& map)
    {
        for (const auto& handle__val : map)
        {
            cb(handle__val.first, handle__val.second);
        }
    }

    template <typename T>
    std::vector<T> Hotswapper::AsVector(std::map<int, T>& map)
    {
        std::vector<T> vec;
        for (const auto& handle__val : map)
        {
            vec.push_back(handle__val.second);
        }
        return vec;
    }
} // namespace skr::hotswap
