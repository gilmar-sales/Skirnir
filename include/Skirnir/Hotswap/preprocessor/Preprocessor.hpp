#pragma once

#include <unordered_set>

#include "Skirnir/Hotswap/preprocessor/IPreprocessor.hpp"
#include "Skirnir/Hotswap/preprocessor/DependencyGraph.hpp"
#include "Skirnir/Hotswap/preprocessor/VarStore.hpp"
#include "Skirnir/Hotswap/preprocessor/Token.hpp"
#include "Skirnir/Hotswap/preprocessor/Lexer.hpp"
#include "Skirnir/Hotswap/preprocessor/Parser.hpp"
#include "Skirnir/Hotswap/preprocessor/Interpreter.hpp"
#include "Skirnir/Hotswap/preprocessor/HscppRequire.hpp"
#include "Skirnir/Hotswap/preprocessor/Variant.hpp"
#include "Skirnir/Hotswap/FsPathHasher.hpp"

namespace skr::hotswap
{
    class Preprocessor : public IPreprocessor
    {
      public:
        bool Preprocess(const std::vector<fs::path>& canonicalFilePaths,
                        Output& output) override;

        void SetVar(const std::string& name, const Variant& value) override;
        bool RemoveVar(const std::string& name) override;

        void ClearDependencyGraph() override;
        void UpdateDependencyGraph(
            const std::vector<fs::path>& canonicalModifiedFilePaths,
            const std::vector<fs::path>& canonicalRemovedFilePaths,
            const std::vector<fs::path>& includeDirectoryPaths) override;

      private:
        std::vector<Token> m_Tokens;

        Lexer m_Lexer;
        Parser m_Parser;
        Interpreter m_Interpreter;

        DependencyGraph m_DependencyGraph;
        VarStore m_VarStore;

        std::unordered_set<fs::path, FsPathHasher> m_SourceFilePaths;
        std::unordered_set<fs::path, FsPathHasher> m_IncludeDirectoryPaths;
        std::unordered_set<fs::path, FsPathHasher> m_LibraryPaths;
        std::unordered_set<fs::path, FsPathHasher> m_LibraryDirectoryPaths;
        std::unordered_set<std::string> m_PreprocessorDefinitions;

        void Reset(Output& output);
        void CreateOutput(Output& output);

        void AddDependentFilePaths(std::unordered_set<fs::path, FsPathHasher>& filePaths);

        bool Preprocess(const std::unordered_set<fs::path, FsPathHasher>& filePaths);
        bool Process(const fs::path& filePath, Interpreter::Result& result);

        bool AddSkrRequire(const fs::path& sourceFilePath, const SkrRequire& skrRequire);
    };
} // namespace skr::hotswap
