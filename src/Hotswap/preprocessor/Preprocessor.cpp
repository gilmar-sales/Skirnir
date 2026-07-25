#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>

#include "Skirnir/Hotswap/preprocessor/Ast.hpp"
#include "Skirnir/Hotswap/preprocessor/Preprocessor.hpp"

namespace skr::hotswap
{
    bool Preprocessor::Preprocess(
        const std::vector<fs::path>& canonicalFilePaths,
        IPreprocessor::Output&       output)
    {
        Reset(output);
        std::unordered_set<fs::path, FsPathHasher> uniqueFilePaths(
            canonicalFilePaths.begin(),
            canonicalFilePaths.end());
        AddDependentFilePaths(uniqueFilePaths);
        m_SourceFilePaths = uniqueFilePaths;

        std::unordered_set<fs::path, FsPathHasher> processed;
        while (!uniqueFilePaths.empty())
        {
            if (!Preprocess(uniqueFilePaths))
                return false;
            processed.insert(uniqueFilePaths.begin(), uniqueFilePaths.end());
            uniqueFilePaths.clear();
            for (const auto& p : m_SourceFilePaths)
            {
                if (processed.find(p) == processed.end())
                    uniqueFilePaths.insert(p);
            }
        }
        CreateOutput(output);
        return true;
    }

    void Preprocessor::SetVar(const std::string& name, const Variant& value)
    {
        m_VarStore.SetVar(name, value);
    }
    bool Preprocessor::RemoveVar(const std::string& name)
    {
        return m_VarStore.RemoveVar(name);
    }
    void Preprocessor::ClearDependencyGraph()
    {
        m_DependencyGraph.Clear();
    }

    void Preprocessor::UpdateDependencyGraph(
        const std::vector<fs::path>& canonicalModifiedFilePaths,
        const std::vector<fs::path>& canonicalRemovedFilePaths,
        const std::vector<fs::path>& includeDirectoryPaths)
    {
        for (const auto& p : canonicalRemovedFilePaths)
            m_DependencyGraph.RemoveFile(p);
        for (const auto& p : canonicalModifiedFilePaths)
        {
            Interpreter::Result result;
            if (Process(p, result))
            {
                std::vector<fs::path> canonicalIncludes;
                for (const auto& inc : result.includePaths)
                {
                    fs::path incPath = fs::u8path(inc);
                    for (const auto& dir : includeDirectoryPaths)
                    {
                        fs::path fullPath = dir / incPath;
                        if (fs::exists(fullPath))
                        {
                            std::error_code error;
                            fs::path canonical = fs::canonical(fullPath, error);
                            if (error.value() == SKR_HOTSWAP_ERROR_SUCCESS)
                            {
                                canonicalIncludes.push_back(canonical);
                            }
                        }
                    }
                }
                m_DependencyGraph.SetLinkedModules(p, result.skrModules);
                m_DependencyGraph.SetFileDependencies(p, canonicalIncludes);
            }
        }
    }

    void Preprocessor::Reset(Output& output)
    {
        output = Output();
        m_SourceFilePaths.clear();
        m_IncludeDirectoryPaths.clear();
        m_LibraryPaths.clear();
        m_LibraryDirectoryPaths.clear();
        m_PreprocessorDefinitions.clear();
    }

    void Preprocessor::CreateOutput(Output& output)
    {
        output.sourceFiles = std::vector<fs::path>(m_SourceFilePaths.begin(),
                                                   m_SourceFilePaths.end());
        output.includeDirectories =
            std::vector<fs::path>(m_IncludeDirectoryPaths.begin(),
                                  m_IncludeDirectoryPaths.end());
        output.libraries =
            std::vector<fs::path>(m_LibraryPaths.begin(), m_LibraryPaths.end());
        output.libraryDirectories =
            std::vector<fs::path>(m_LibraryDirectoryPaths.begin(),
                                  m_LibraryDirectoryPaths.end());
        output.preprocessorDefinitions =
            std::vector<std::string>(m_PreprocessorDefinitions.begin(),
                                     m_PreprocessorDefinitions.end());
    }

    void Preprocessor::AddDependentFilePaths(
        std::unordered_set<fs::path, FsPathHasher>& filePaths)
    {
        std::unordered_set<fs::path, FsPathHasher> dependents;
        for (const auto& p : filePaths)
        {
            auto more = m_DependencyGraph.ResolveGraph(p);
            dependents.insert(more.begin(), more.end());
        }
        filePaths.insert(dependents.begin(), dependents.end());
    }

    bool Preprocessor::Preprocess(
        const std::unordered_set<fs::path, FsPathHasher>& filePaths)
    {
        for (const auto& p : filePaths)
        {
            Interpreter::Result result;
            if (!Process(p, result))
            {
                log::Error() << SKR_HOTSWAP_LOG_PREFIX
                             << "Failed to process file " << p << log::End(".");
                return false;
            }
            for (const auto& r : result.skrRequires)
            {
                if (!AddSkrRequire(p, r))
                {
                    log::Error()
                        << SKR_HOTSWAP_LOG_PREFIX << "Failed to process "
                        << r.name << " in file " << p << " (Line: " << r.line
                        << ")" << log::End();
                    return false;
                }
            }
            for (const auto& m : result.skrMessages)
            {
                log::Build() << SKR_HOTSWAP_LOG_PREFIX << m << log::End();
            }
        }
        return true;
    }

    bool Preprocessor::Process(const fs::path&      filePath,
                               Interpreter::Result& result)
    {
        std::ifstream ifs(filePath.string());
        if (!ifs.is_open())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to open file "
                         << filePath << log::LastOsError() << log::End(".");
            return false;
        }
        std::stringstream ss;
        ss << ifs.rdbuf();

        if (!m_Lexer.Lex(ss.str(), m_Tokens))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to lex "
                         << filePath << log::End(".");
            log::Error() << m_Lexer.GetLastError().ToString() << log::End();
            return false;
        }
        std::unique_ptr<Stmt> pRoot;
        if (!m_Parser.Parse(m_Tokens, pRoot))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to parse "
                         << filePath << log::End(".");
            log::Error() << m_Parser.GetLastError().ToString() << log::End();
            return false;
        }
        if (!m_Interpreter.Evaluate(*pRoot, m_VarStore, result))
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX << "Failed to interpret "
                         << filePath << log::End(".");
            log::Error() << m_Interpreter.GetLastError().ToString()
                         << log::End();
            return false;
        }
        return true;
    }

    bool Preprocessor::AddSkrRequire(const fs::path&   sourceFilePath,
                                     const SkrRequire& skrRequire)
    {
        for (const auto& v : skrRequire.values)
        {
            switch (skrRequire.type)
            {
                case SkrRequire::Type::Source:
                case SkrRequire::Type::IncludeDir:
                case SkrRequire::Type::Library:
                case SkrRequire::Type::LibraryDir: {
                    fs::path path = fs::path(v);
                    fs::path fullPath =
                        path.is_relative() ? sourceFilePath.parent_path() / path
                                           : path;
                    std::error_code error;
                    fs::path        canonical = fs::canonical(fullPath, error);
                    if (error.value() != SKR_HOTSWAP_ERROR_SUCCESS)
                    {
                        log::Error()
                            << SKR_HOTSWAP_LOG_PREFIX
                            << "Unable to get canonical path of " << fullPath
                            << " within " << skrRequire.name << " (Line: "
                            << skrRequire.line << ")" << log::End();
                        return false;
                    }
                    switch (skrRequire.type)
                    {
                        case SkrRequire::Type::Source:
                            m_SourceFilePaths.insert(canonical);
                            break;
                        case SkrRequire::Type::IncludeDir:
                            m_IncludeDirectoryPaths.insert(canonical);
                            break;
                        case SkrRequire::Type::Library:
                            m_LibraryPaths.insert(canonical);
                            break;
                        case SkrRequire::Type::LibraryDir:
                            m_LibraryDirectoryPaths.insert(canonical);
                            break;
                        default:
                            assert(false);
                            return false;
                    }
                    break;
                }
                case SkrRequire::Type::PreprocessorDef:
                    m_PreprocessorDefinitions.insert(v);
                    break;
                default:
                    assert(false);
                    return false;
            }
        }
        return true;
    }
} // namespace skr::hotswap
