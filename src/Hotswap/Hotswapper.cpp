#include <unordered_set>
#include <fstream>
#include <thread>
#include <chrono>
#include <cassert>
#include <functional>

#include "Skirnir/Hotswap/Hotswapper.hpp"
#include "Skirnir/Hotswap/Util.hpp"
#include "Skirnir/Hotswap/preprocessor/Variant.hpp"
#include "Skirnir/Hotswap/preprocessor/Preprocessor.hpp"

#ifndef SKR_DISABLE_HOTSWAP
namespace skr::hotswap
{
    namespace
    {
        const std::string HOTSWAP_TEMP_DIRECTORY_NAME =
            "SKR_HOTSWAP_7c9279ff-25af-488c-a634-b6aa68f47a65";
    } // namespace

    Hotswapper::Hotswapper()
        : Hotswapper(std::unique_ptr<Config>(new Config()), nullptr, nullptr, nullptr)
    {
    }

    Hotswapper::Hotswapper(std::unique_ptr<Config> pConfig)
        : Hotswapper(std::move(pConfig), nullptr, nullptr, nullptr)
    {
    }

    Hotswapper::Hotswapper(std::unique_ptr<Config> pConfig,
                           std::unique_ptr<IFileWatcher> pFileWatcher,
                           std::unique_ptr<ICompiler> pCompiler,
                           std::unique_ptr<IPreprocessor> pPreprocessor)
        : m_pConfig(std::move(pConfig))
    {
        if (pFileWatcher != nullptr)
        {
            m_pFileWatcher = std::move(pFileWatcher);
        }
        else
        {
            m_pFileWatcher = platform::CreateFileWatcher(&m_pConfig->fileWatcher);
        }
        if (pCompiler != nullptr)
        {
            m_pCompiler = std::move(pCompiler);
        }
        else
        {
            m_pCompiler = platform::CreateCompiler(&m_pConfig->compiler);
        }
        if (pPreprocessor != nullptr)
        {
            m_pPreprocessor = std::move(pPreprocessor);
        }
        else
        {
            m_pPreprocessor = std::unique_ptr<IPreprocessor>(new Preprocessor());
        }

        if (!(m_pConfig->flags & Config::Flag::NoDefaultCompileOptions))
        {
            for (const auto& option : platform::GetDefaultCompileOptions())
            {
                Add(option, m_NextCompileOptionHandle, m_CompileOptionsByHandle);
            }
        }
        if (!(m_pConfig->flags & Config::Flag::NoDefaultPreprocessorDefinitions))
        {
            for (const auto& d : platform::GetDefaultPreprocessorDefinitions())
            {
                Add(d, m_NextPreprocessorDefinitionHandle, m_PreprocessorDefinitionsByHandle);
            }
        }
        if (!(m_pConfig->flags & Config::Flag::NoDefaultIncludeDirectories))
        {
            Add(util::GetHotswapIncludePath(), m_NextIncludeDirectoryHandle,
                m_IncludeDirectoryPathsByHandle);
        }
        if (!(m_pConfig->flags & Config::Flag::NoDefaultForceCompiledSourceFiles))
        {
            fs::path moduleFilePath =
                util::GetHotswapSourcePath() / "Hotswap" / "module" / "Module.cpp";
            Add(moduleFilePath, m_NextForceCompiledSourceFileHandle,
                m_ForceCompiledSourceFilePathsByHandle);
        }
    }

    AllocationResolver* Hotswapper::GetAllocationResolver() { return &m_AllocationResolver; }

    void Hotswapper::SetAllocator(IAllocator* pAllocator) { m_ModuleManager.SetAllocator(pAllocator); }
    void Hotswapper::SetGlobalUserData(void* p) { m_ModuleManager.SetGlobalUserData(p); }

    void Hotswapper::EnableFeature(Feature f) { m_FeatureManager.EnableFeature(f); }
    void Hotswapper::DisableFeature(Feature f) { m_FeatureManager.DisableFeature(f); }
    bool Hotswapper::IsFeatureEnabled(Feature f) { return m_FeatureManager.IsFeatureEnabled(f); }

    void Hotswapper::TriggerManualBuild()
    {
        if (!CreateBuildDirectory()) return;
        ICompiler::Input compilerInput;
        if (!CreateCompilerInput({}, compilerInput)) return;
        if (!StartCompile(compilerInput)) return;
        while (m_pCompiler->IsCompiling())
        {
            m_pCompiler->Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (m_pCompiler->HasCompiledModule()) PerformRuntimeSwap();
    }

    Hotswapper::UpdateResult Hotswapper::Update()
    {
        if (m_bDependencyGraphNeedsRefresh) RefreshDependencyGraph();
        m_pCompiler->Update();
        if (m_pCompiler->IsCompiling()) return UpdateResult::Compiling;
        if (m_pCompiler->HasCompiledModule())
        {
            return PerformRuntimeSwap() ? UpdateResult::PerformedSwap
                                        : UpdateResult::FailedSwap;
        }
        if (!IsFeatureEnabled(Feature::ManualCompilationOnly))
        {
            m_pFileWatcher->PollChanges(m_FileEvents);
        }
        if (!m_FileEvents.empty())
        {
            if (!CreateBuildDirectory()) return UpdateResult::Idle;
            std::vector<fs::path> modified, removed;
            util::SortFileEvents(m_FileEvents, modified, removed);
            UpdateDependencyGraph(modified, removed);
            if (!modified.empty())
            {
                ICompiler::Input compilerInput;
                if (CreateCompilerInput(modified, compilerInput))
                {
                    if (StartCompile(compilerInput)) return UpdateResult::StartedCompiling;
                }
            }
        }
        return UpdateResult::Idle;
    }

    bool Hotswapper::IsCompiling() { return m_pCompiler->IsCompiling(); }
    bool Hotswapper::IsCompilerInitialized() { return m_pCompiler->IsInitialized(); }

    void Hotswapper::SetCallbacks(const Callbacks& cb) { m_Callbacks = cb; }

    void Hotswapper::DoProtectedCall(const std::function<void()>& cb)
    {
#ifdef SKR_DISABLE_HOTSWAP
        cb();
#else
        ProtectedFunction::Result result = ProtectedFunction::Call(cb);
        while (result != ProtectedFunction::Result::Success)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed protected call. Make code changes and save to reattempt."
                         << log::End();
            while (Update() != UpdateResult::PerformedSwap)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            result = ProtectedFunction::Call(cb);
        }
#endif
    }

    int Hotswapper::AddIncludeDirectory(const fs::path& p)
    {
        m_bDependencyGraphNeedsRefresh = true;
        return Add(p, m_NextIncludeDirectoryHandle, m_IncludeDirectoryPathsByHandle);
    }
    bool Hotswapper::RemoveIncludeDirectory(int h)
    {
        bool r = Remove(h, m_IncludeDirectoryPathsByHandle);
        if (r) m_bDependencyGraphNeedsRefresh = true;
        return r;
    }
    void Hotswapper::EnumerateIncludeDirectories(
        const std::function<void(int, const fs::path&)>& cb)
    {
        Enumerate(cb, m_IncludeDirectoryPathsByHandle);
    }
    void Hotswapper::ClearIncludeDirectories() { m_IncludeDirectoryPathsByHandle.clear(); }

    int Hotswapper::AddSourceDirectory(const fs::path& p)
    {
        m_bDependencyGraphNeedsRefresh = true;
        m_pFileWatcher->AddWatch(p);
        return Add(p, m_NextSourceDirectoryHandle, m_SourceDirectoryPathsByHandle);
    }
    bool Hotswapper::RemoveSourceDirectory(int h)
    {
        auto it = m_SourceDirectoryPathsByHandle.find(h);
        if (it != m_SourceDirectoryPathsByHandle.end())
        {
            m_pFileWatcher->RemoveWatch(it->second);
        }
        bool r = Remove(h, m_SourceDirectoryPathsByHandle);
        if (r) m_bDependencyGraphNeedsRefresh = true;
        return r;
    }
    void Hotswapper::EnumerateSourceDirectories(
        const std::function<void(int, const fs::path&)>& cb)
    {
        Enumerate(cb, m_SourceDirectoryPathsByHandle);
    }
    void Hotswapper::ClearSourceDirectories() { m_SourceDirectoryPathsByHandle.clear(); }

    int Hotswapper::AddForceCompiledSourceFile(const fs::path& p)
    {
        return Add(p, m_NextForceCompiledSourceFileHandle,
                   m_ForceCompiledSourceFilePathsByHandle);
    }
    bool Hotswapper::RemoveForceCompiledSourceFile(int h)
    {
        return Remove(h, m_ForceCompiledSourceFilePathsByHandle);
    }
    void Hotswapper::EnumerateForceCompiledSourceFiles(
        const std::function<void(int, const fs::path&)>& cb)
    {
        Enumerate(cb, m_ForceCompiledSourceFilePathsByHandle);
    }
    void Hotswapper::ClearForceCompiledSourceFiles()
    {
        m_ForceCompiledSourceFilePathsByHandle.clear();
    }

    int Hotswapper::AddLibraryDirectory(const fs::path& p)
    {
        return Add(p, m_NextLibraryDirectoryHandle, m_LibraryDirectoryPathsByHandle);
    }
    bool Hotswapper::RemoveLibraryDirectory(int h)
    {
        return Remove(h, m_LibraryDirectoryPathsByHandle);
    }
    void Hotswapper::EnumerateLibraryDirectories(
        const std::function<void(int, const fs::path&)>& cb)
    {
        Enumerate(cb, m_LibraryDirectoryPathsByHandle);
    }
    void Hotswapper::ClearLibraryDirectories() { m_LibraryDirectoryPathsByHandle.clear(); }

    int Hotswapper::AddLibrary(const fs::path& p)
    {
        return Add(p, m_NextLibraryHandle, m_LibraryPathsByHandle);
    }
    int Hotswapper::LocateAndAddLibrary(const fs::path& root, const fs::path& name)
    {
        return Add(util::FindFile(root, name), m_NextLibraryHandle, m_LibraryPathsByHandle);
    }
    bool Hotswapper::RemoveLibrary(int h) { return Remove(h, m_LibraryPathsByHandle); }
    void Hotswapper::EnumerateLibraries(
        const std::function<void(int, const fs::path&)>& cb)
    {
        Enumerate(cb, m_LibraryPathsByHandle);
    }
    void Hotswapper::ClearLibraries() { m_LibraryPathsByHandle.clear(); }

    int Hotswapper::AddPreprocessorDefinition(const std::string& d)
    {
        return Add(d, m_NextPreprocessorDefinitionHandle, m_PreprocessorDefinitionsByHandle);
    }
    bool Hotswapper::RemovePreprocessorDefinition(int h)
    {
        return Remove(h, m_PreprocessorDefinitionsByHandle);
    }
    void Hotswapper::EnumeratePreprocessorDefinitions(
        const std::function<void(int, const std::string&)>& cb)
    {
        Enumerate(cb, m_PreprocessorDefinitionsByHandle);
    }
    void Hotswapper::ClearPreprocessorDefinitions()
    {
        m_PreprocessorDefinitionsByHandle.clear();
    }

    int Hotswapper::AddCompileOption(const std::string& o)
    {
        return Add(o, m_NextCompileOptionHandle, m_CompileOptionsByHandle);
    }
    bool Hotswapper::RemoveCompileOption(int h) { return Remove(h, m_CompileOptionsByHandle); }
    void Hotswapper::EnumerateCompileOptions(
        const std::function<void(int, const std::string&)>& cb)
    {
        Enumerate(cb, m_CompileOptionsByHandle);
    }
    void Hotswapper::ClearCompileOptions() { m_CompileOptionsByHandle.clear(); }

    int Hotswapper::AddLinkOption(const std::string& o)
    {
        return Add(o, m_NextLinkOptionHandle, m_LinkOptionsByHandle);
    }
    bool Hotswapper::RemoveLinkOption(int h) { return Remove(h, m_LinkOptionsByHandle); }
    void Hotswapper::EnumerateLinkOptions(
        const std::function<void(int, const std::string&)>& cb)
    {
        Enumerate(cb, m_LinkOptionsByHandle);
    }
    void Hotswapper::ClearLinkOptions() { m_LinkOptionsByHandle.clear(); }

    void Hotswapper::SetVar(const std::string& name, const std::string& val)
    {
        m_pPreprocessor->SetVar(name, Variant(val));
    }
    void Hotswapper::SetVar(const std::string& name, const char* pVal)
    {
        SetVar(name, std::string(pVal));
    }
    void Hotswapper::SetVar(const std::string& name, double val)
    {
        m_pPreprocessor->SetVar(name, Variant(val));
    }
    void Hotswapper::SetVar(const std::string& name, bool val)
    {
        m_pPreprocessor->SetVar(name, Variant(val));
    }
    bool Hotswapper::RemoveVar(const std::string& name)
    {
        return m_pPreprocessor->RemoveVar(name);
    }

    bool Hotswapper::StartCompile(ICompiler::Input& compilerInput)
    {
        if (m_Callbacks.BeforeCompile) m_Callbacks.BeforeCompile(compilerInput);
        if (compilerInput.sourceFilePaths.empty()) return false;
        return m_pCompiler->StartBuild(compilerInput);
    }

    bool Hotswapper::CreateCompilerInput(const std::vector<fs::path>& sourceFilePaths,
                                         ICompiler::Input& compilerInput)
    {
        compilerInput.buildDirectoryPath = m_BuildDirectoryPath;
        compilerInput.sourceFilePaths = sourceFilePaths;
        compilerInput.includeDirectoryPaths = AsVector(m_IncludeDirectoryPathsByHandle);
        compilerInput.libraryDirectoryPaths = AsVector(m_LibraryDirectoryPathsByHandle);
        compilerInput.libraryPaths = AsVector(m_LibraryPathsByHandle);
        compilerInput.preprocessorDefinitions = AsVector(m_PreprocessorDefinitionsByHandle);
        compilerInput.compileOptions = AsVector(m_CompileOptionsByHandle);
        compilerInput.linkOptions = AsVector(m_LinkOptionsByHandle);

        for (const auto& h__path : m_ForceCompiledSourceFilePathsByHandle)
        {
            compilerInput.sourceFilePaths.push_back(h__path.second);
        }

        Deduplicate(compilerInput);
        if (!Preprocess(compilerInput)) return false;

        compilerInput.sourceFilePaths.erase(
            std::remove_if(
                compilerInput.sourceFilePaths.begin(),
                compilerInput.sourceFilePaths.end(),
                [](const fs::path& p) { return !util::IsSourceFile(p); }),
            compilerInput.sourceFilePaths.end());
        Deduplicate(compilerInput);
        return true;
    }

    bool Hotswapper::Preprocess(ICompiler::Input& compilerInput)
    {
        if (!IsFeatureEnabled(Feature::Preprocessor)) return true;
        IPreprocessor::Output out;
        if (!m_pPreprocessor->Preprocess(compilerInput.sourceFilePaths, out))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Preprocessing failed. Compilation will be skipped."
                         << log::End();
            return false;
        }
        compilerInput.sourceFilePaths.insert(compilerInput.sourceFilePaths.end(),
                                            out.sourceFiles.begin(),
                                            out.sourceFiles.end());
        compilerInput.includeDirectoryPaths.insert(
            compilerInput.includeDirectoryPaths.end(),
            out.includeDirectories.begin(), out.includeDirectories.end());
        compilerInput.libraryPaths.insert(compilerInput.libraryPaths.end(),
                                          out.libraries.begin(), out.libraries.end());
        compilerInput.libraryDirectoryPaths.insert(
            compilerInput.libraryDirectoryPaths.end(),
            out.libraryDirectories.begin(), out.libraryDirectories.end());
        compilerInput.preprocessorDefinitions.insert(
            compilerInput.preprocessorDefinitions.end(),
            out.preprocessorDefinitions.begin(),
            out.preprocessorDefinitions.end());
        return true;
    }

    void Hotswapper::Deduplicate(ICompiler::Input& input)
    {
        util::Deduplicate<fs::path, FsPathHasher>(input.sourceFilePaths);
        util::Deduplicate<fs::path, FsPathHasher>(input.includeDirectoryPaths);
        util::Deduplicate<fs::path, FsPathHasher>(input.libraryPaths);
        util::Deduplicate(input.preprocessorDefinitions);
        util::Deduplicate(input.compileOptions);
        util::Deduplicate(input.linkOptions);
    }

    bool Hotswapper::PerformRuntimeSwap()
    {
        if (m_Callbacks.BeforeSwap) m_Callbacks.BeforeSwap();
        bool r = m_ModuleManager.PerformRuntimeSwap(m_pCompiler->PopModule());
        if (m_Callbacks.AfterSwap) m_Callbacks.AfterSwap();
        return r;
    }

    bool Hotswapper::CreateHotswapTempDirectory()
    {
        std::error_code error;
        fs::path temp = fs::temp_directory_path(error);
        if (error.value() != SKR_HOTSWAP_ERROR_SUCCESS)
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to find temp directory path. "
                         << log::OsError(error) << log::End();
            return false;
        }
        fs::path hotswapTemp = temp / HOTSWAP_TEMP_DIRECTORY_NAME;
        fs::remove_all(hotswapTemp, error);
        if (!fs::create_directory(hotswapTemp, error))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create directory " << hotswapTemp << ". "
                         << log::OsError(error) << log::End();
            return false;
        }
        m_HotswapTempDirectoryPath = hotswapTemp;
        return true;
    }

    bool Hotswapper::CreateBuildDirectory()
    {
        if (m_HotswapTempDirectoryPath.empty())
        {
            if (!CreateHotswapTempDirectory()) return false;
        }
        std::string guid = platform::CreateGuid();
        m_BuildDirectoryPath = m_HotswapTempDirectoryPath / guid;
        std::error_code error;
        if (!fs::create_directory(m_BuildDirectoryPath, error))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Failed to create directory " << m_BuildDirectoryPath
                         << ". " << log::OsError(error) << log::End();
            return false;
        }
        return true;
    }

    void Hotswapper::UpdateDependencyGraph(
        const std::vector<fs::path>& canonicalModifiedFilePaths,
        const std::vector<fs::path>& canonicalRemovedFilePaths)
    {
        if (IsFeatureEnabled(Feature::DependentCompilation))
        {
            m_pPreprocessor->UpdateDependencyGraph(
                canonicalModifiedFilePaths, canonicalRemovedFilePaths,
                AsVector(m_IncludeDirectoryPathsByHandle));
        }
    }

    void Hotswapper::RefreshDependencyGraph()
    {
        if (IsFeatureEnabled(Feature::DependentCompilation))
        {
            m_pPreprocessor->ClearDependencyGraph();
            std::unordered_set<fs::path, FsPathHasher> unique;
            AppendDirectoryFiles<fs::directory_iterator>(m_SourceDirectoryPathsByHandle,
                                                          unique);
            AppendDirectoryFiles<fs::recursive_directory_iterator>(
                m_IncludeDirectoryPathsByHandle, unique);
            m_pPreprocessor->UpdateDependencyGraph(
                std::vector<fs::path>(unique.begin(), unique.end()), {},
                AsVector(m_IncludeDirectoryPathsByHandle));
        }
        m_bDependencyGraphNeedsRefresh = false;
    }
} // namespace skr::hotswap
#else
// SKR_DISABLE_HOTSWAP: provide stub implementations so the class still
// compiles and the public API remains usable. Hotswapper becomes a no-op.
namespace skr::hotswap
{
    Hotswapper::Hotswapper() = default;
    Hotswapper::Hotswapper(std::unique_ptr<Config>) {}
    Hotswapper::Hotswapper(std::unique_ptr<Config>,
                           std::unique_ptr<IFileWatcher>,
                           std::unique_ptr<ICompiler>,
                           std::unique_ptr<IPreprocessor>) {}

    AllocationResolver* Hotswapper::GetAllocationResolver() { return &m_AllocationResolver; }
    void Hotswapper::SetAllocator(IAllocator* p) { m_ModuleManager.SetAllocator(p); }
    void Hotswapper::SetGlobalUserData(void* p) { m_ModuleManager.SetGlobalUserData(p); }
    void Hotswapper::EnableFeature(Feature) {}
    void Hotswapper::DisableFeature(Feature) {}
    bool Hotswapper::IsFeatureEnabled(Feature) { return false; }
    void Hotswapper::TriggerManualBuild() {}
    Hotswapper::UpdateResult Hotswapper::Update() { return UpdateResult::Idle; }
    bool Hotswapper::IsCompiling() { return false; }
    bool Hotswapper::IsCompilerInitialized() { return true; }
    void Hotswapper::SetCallbacks(const Callbacks&) {}
    void Hotswapper::DoProtectedCall(const std::function<void()>& cb) { cb(); }

    int Hotswapper::AddIncludeDirectory(const fs::path&) { return -1; }
    bool Hotswapper::RemoveIncludeDirectory(int) { return false; }
    void Hotswapper::EnumerateIncludeDirectories(
        const std::function<void(int, const fs::path&)>&) {}
    void Hotswapper::ClearIncludeDirectories() {}
    int Hotswapper::AddSourceDirectory(const fs::path&) { return -1; }
    bool Hotswapper::RemoveSourceDirectory(int) { return false; }
    void Hotswapper::EnumerateSourceDirectories(
        const std::function<void(int, const fs::path&)>&) {}
    void Hotswapper::ClearSourceDirectories() {}
    int Hotswapper::AddForceCompiledSourceFile(const fs::path&) { return -1; }
    bool Hotswapper::RemoveForceCompiledSourceFile(int) { return false; }
    void Hotswapper::EnumerateForceCompiledSourceFiles(
        const std::function<void(int, const fs::path&)>&) {}
    void Hotswapper::ClearForceCompiledSourceFiles() {}
    int Hotswapper::AddLibraryDirectory(const fs::path&) { return -1; }
    bool Hotswapper::RemoveLibraryDirectory(int) { return false; }
    void Hotswapper::EnumerateLibraryDirectories(
        const std::function<void(int, const fs::path&)>&) {}
    void Hotswapper::ClearLibraryDirectories() {}
    int Hotswapper::AddLibrary(const fs::path&) { return -1; }
    int Hotswapper::LocateAndAddLibrary(const fs::path&, const fs::path&) { return -1; }
    bool Hotswapper::RemoveLibrary(int) { return false; }
    void Hotswapper::EnumerateLibraries(
        const std::function<void(int, const fs::path&)>&) {}
    void Hotswapper::ClearLibraries() {}
    int Hotswapper::AddPreprocessorDefinition(const std::string&) { return -1; }
    bool Hotswapper::RemovePreprocessorDefinition(int) { return false; }
    void Hotswapper::EnumeratePreprocessorDefinitions(
        const std::function<void(int, const std::string&)>&) {}
    void Hotswapper::ClearPreprocessorDefinitions() {}
    int Hotswapper::AddCompileOption(const std::string&) { return -1; }
    bool Hotswapper::RemoveCompileOption(int) { return false; }
    void Hotswapper::EnumerateCompileOptions(
        const std::function<void(int, const std::string&)>&) {}
    void Hotswapper::ClearCompileOptions() {}
    int Hotswapper::AddLinkOption(const std::string&) { return -1; }
    bool Hotswapper::RemoveLinkOption(int) { return false; }
    void Hotswapper::EnumerateLinkOptions(
        const std::function<void(int, const std::string&)>&) {}
    void Hotswapper::ClearLinkOptions() {}
    void Hotswapper::SetVar(const std::string&, const std::string&) {}
    void Hotswapper::SetVar(const std::string&, const char*) {}
    bool Hotswapper::RemoveVar(const std::string&) { return false; }
} // namespace skr::hotswap
#endif
