#include <stdexcept>
#include <cassert>

#include "Skirnir/Hotswap/preprocessor/Interpreter.hpp"
#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap
{
    bool Interpreter::Evaluate(const Stmt& rootStmt, const VarStore& varStore, Result& result)
    {
        Reset(varStore, result);
        try
        {
            rootStmt.Accept(*this);
        }
        catch (ReturnFromInterpreter&)
        {
            return true;
        }
        catch (std::runtime_error&)
        {
            return false;
        }
        return true;
    }

    LangError Interpreter::GetLastError() { return m_Error; }

    void Interpreter::Reset(const VarStore& varStore, Result& result)
    {
        m_pVarStore = &varStore;
        result = Result();
        m_pResult = &result;
        m_Error = LangError(LangError::Code::Success);
    }

    void Interpreter::Visit(const BlockStmt& blockStmt)
    {
        for (const auto& s : blockStmt.statements) s->Accept(*this);
    }

    void Interpreter::Visit(const IncludeStmt& includeStmt)
    {
        m_pResult->includePaths.push_back(includeStmt.path);
    }

    void Interpreter::Visit(const HscppIfStmt& ifStmt)
    {
        bool matched = false;
        for (std::size_t i = 0; i < ifStmt.conditions.size(); ++i)
        {
            ifStmt.conditions.at(i)->Accept(*this);
            if (PopResult().IsTruthy())
            {
                ifStmt.conditionalBlocks.at(i)->Accept(*this);
                matched = true;
                break;
            }
        }
        if (!matched && ifStmt.pElseBlock != nullptr)
        {
            ifStmt.pElseBlock->Accept(*this);
        }
    }

    void Interpreter::Visit(const HscppReturnStmt& /*returnStmt*/)
    {
        throw ReturnFromInterpreter();
    }

    void Interpreter::Visit(const HscppRequireStmt& requireStmt)
    {
        SkrRequire req;
        req.name = requireStmt.token.value;
        req.line = requireStmt.token.line;
        switch (requireStmt.token.type)
        {
            case Token::Type::HscppRequireSource:           req.type = SkrRequire::Type::Source; break;
            case Token::Type::HscppRequireIncludeDir:       req.type = SkrRequire::Type::IncludeDir; break;
            case Token::Type::HscppRequireLibrary:          req.type = SkrRequire::Type::Library; break;
            case Token::Type::HscppRequireLibraryDir:       req.type = SkrRequire::Type::LibraryDir; break;
            case Token::Type::HscppRequirePreprocessorDef:  req.type = SkrRequire::Type::PreprocessorDef; break;
            default: assert(false); break;
        }
        for (const auto& p : requireStmt.parameters) req.values.push_back(Interpolate(p));
        m_pResult->skrRequires.push_back(req);
    }

    void Interpreter::Visit(const HscppModuleStmt& moduleStmt)
    {
        m_pResult->skrModules.push_back(Interpolate(moduleStmt.module));
    }

    void Interpreter::Visit(const HscppMessageStmt& messageStmt)
    {
        m_pResult->skrMessages.push_back(Interpolate(messageStmt.message));
    }

    void Interpreter::Visit(const UnaryExpr& unaryExpr)
    {
        unaryExpr.pRightExpr->Accept(*this);
        Variant result;
        LangError error(LangError::Code::Success);
        if (!UnaryOp(unaryExpr.op, PopResult(), result, error)) ThrowError(error);
        m_VariantStack.push(result);
    }

    void Interpreter::Visit(const BinaryExpr& binaryExpr)
    {
        binaryExpr.pRightExpr->Accept(*this);
        binaryExpr.pLeftExpr->Accept(*this);
        Variant left = PopResult();
        Variant right = PopResult();
        Variant result;
        LangError error(LangError::Code::Success);
        if (!BinaryOp(binaryExpr.op, left, right, result, error)) ThrowError(error);
        m_VariantStack.push(result);
    }

    void Interpreter::Visit(const NameExpr& nameExpr)
    {
        Variant val;
        if (!m_pVarStore->GetVar(nameExpr.name.value, val))
        {
            ThrowError(LangError(LangError::Code::Interpreter_UnableToResolveName,
                                 nameExpr.name.line, { nameExpr.name.value }));
        }
        m_VariantStack.push(val);
    }

    void Interpreter::Visit(const StringLiteralExpr& s)
    {
        m_VariantStack.push(Variant(Interpolate(s.value)));
    }

    void Interpreter::Visit(const NumberLiteralExpr& n)
    {
        m_VariantStack.push(Variant(n.value));
    }

    void Interpreter::Visit(const BoolLiteralExpr& b)
    {
        m_VariantStack.push(Variant(b.value));
    }

    Variant Interpreter::PopResult()
    {
        if (m_VariantStack.empty())
        {
            ThrowError(LangError(LangError::Code::InternalError));
            return Variant();
        }
        Variant v = m_VariantStack.top();
        m_VariantStack.pop();
        return v;
    }

    std::string Interpreter::Interpolate(const std::string& str)
    {
        return m_pVarStore->Interpolate(str);
    }

    void Interpreter::ThrowError(const LangError& error)
    {
        m_Error = error;
        throw std::runtime_error("");
    }
} // namespace skr::hotswap
