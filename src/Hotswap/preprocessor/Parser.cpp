#include <iostream>
#include <cassert>
#include <limits>

#include "Skirnir/Hotswap/preprocessor/Parser.hpp"

namespace skr::hotswap
{
    bool Parser::Parse(const std::vector<Token>& tokens, std::unique_ptr<Stmt>& pRootStmt)
    {
        Reset(tokens);
        try
        {
            pRootStmt = ParseBlockStmt();
            return true;
        }
        catch (const std::runtime_error&)
        {
            return false;
        }
    }

    LangError Parser::GetLastError() { return m_Error; }

    void Parser::Reset(const std::vector<Token>& tokens)
    {
        m_pTokens = &tokens;
        m_iToken = 0;
        m_Scopes = std::stack<std::unique_ptr<BlockStmt>>();
        m_DefaultToken = Token();
        if (!tokens.empty()) m_DefaultToken.line = tokens.back().line;
    }

    std::unique_ptr<Expr> Parser::ParseExpr(int precedence)
    {
        auto pExpr = ParsePrefixExpr();
        while (precedence < GetInfixPrecedence())
        {
            pExpr = ParseInfixExpr(std::move(pExpr));
        }
        return pExpr;
    }

    std::unique_ptr<Expr> Parser::ParsePrefixExpr()
    {
        switch (Peek().type)
        {
            case Token::Type::String:    return ParseStringLiteralExpr();
            case Token::Type::Number:    return ParseNumberLiteralExpr();
            case Token::Type::Bool:      return ParseBoolLiteralExpr();
            case Token::Type::LeftParen: return ParseGroupExpr();
            case Token::Type::Identifier: return ParseNameExpr();
            case Token::Type::Minus:
            case Token::Type::Exclamation:
                return ParseUnaryExpr();
            default:
                ThrowError(LangError(LangError::Code::Parser_FailedToParsePrefixExpression,
                                     Peek().line, { Peek().value }));
                return nullptr;
        }
    }

    std::unique_ptr<Expr> Parser::ParseInfixExpr(std::unique_ptr<Expr> pLeftExpr)
    {
        switch (Peek().type)
        {
            case Token::Type::Equivalent:
            case Token::Type::Inequivalent:
            case Token::Type::LessThan:
            case Token::Type::LessThanOrEqual:
            case Token::Type::GreaterThan:
            case Token::Type::GreaterThanOrEqual:
            case Token::Type::LogicalAnd:
            case Token::Type::LogicalOr:
            case Token::Type::Plus:
            case Token::Type::Minus:
            case Token::Type::Slash:
            case Token::Type::Star:
                return ParseBinaryExpr(std::move(pLeftExpr));
            default:
                ThrowError(LangError(LangError::Code::Parser_FailedToParseInfixExpression,
                                     Peek().line, { Peek().value }));
                return nullptr;
        }
    }

    std::unique_ptr<Expr> Parser::ParseStringLiteralExpr()
    {
        auto e = std::unique_ptr<StringLiteralExpr>(new StringLiteralExpr());
        e->value = Peek().value;
        Consume();
        return e;
    }

    std::unique_ptr<Expr> Parser::ParseNumberLiteralExpr()
    {
        auto e = std::unique_ptr<NumberLiteralExpr>(new NumberLiteralExpr());
        try
        {
            e->value = std::stod(Peek().value);
            Consume();
        }
        catch (const std::invalid_argument&)
        {
            ThrowError(LangError(LangError::Code::Parser_FailedToParseNumber,
                                 Peek().line, { Peek().value }));
        }
        catch (const std::out_of_range&)
        {
            ThrowError(LangError(LangError::Code::Parser_NumberIsOutOfRange,
                                 Peek().line, { Peek().value }));
        }
        return e;
    }

    std::unique_ptr<Expr> Parser::ParseBoolLiteralExpr()
    {
        auto e = std::unique_ptr<BoolLiteralExpr>(new BoolLiteralExpr());
        if (Peek().value == "true") e->value = true;
        else if (Peek().value == "false") e->value = false;
        else { assert(false); ThrowError(LangError(LangError::Code::InternalError)); }
        Consume();
        return e;
    }

    std::unique_ptr<Expr> Parser::ParseGroupExpr()
    {
        Consume();
        auto pExpr = ParseExpr();
        Expect(Token::Type::RightParen,
               LangError(LangError::Code::Parser_GroupExpressionMissingClosingParen,
                        Prev().line, Prev().column, {}));
        Consume();
        return pExpr;
    }

    std::unique_ptr<Expr> Parser::ParseNameExpr()
    {
        auto e = std::unique_ptr<NameExpr>(new NameExpr());
        e->name = Peek();
        Consume();
        return e;
    }

    std::unique_ptr<Expr> Parser::ParseUnaryExpr()
    {
        auto e = std::unique_ptr<UnaryExpr>(new UnaryExpr());
        int prec = GetPrefixPrecedence();
        e->op = Peek();
        Consume();
        e->pRightExpr = ParseExpr(prec);
        return e;
    }

    std::unique_ptr<Expr> Parser::ParseBinaryExpr(std::unique_ptr<Expr> pLeftExpr)
    {
        auto e = std::unique_ptr<BinaryExpr>(new BinaryExpr());
        int prec = GetInfixPrecedence();
        e->op = Peek();
        Consume();
        e->pLeftExpr = std::move(pLeftExpr);
        e->pRightExpr = ParseExpr(prec);
        return e;
    }

    int Parser::GetPrefixPrecedence() { return 7; }

    int Parser::GetInfixPrecedence()
    {
        switch (Peek().type)
        {
            case Token::Type::LogicalOr:        return 1;
            case Token::Type::LogicalAnd:       return 2;
            case Token::Type::Equivalent:
            case Token::Type::Inequivalent:     return 3;
            case Token::Type::LessThan:
            case Token::Type::LessThanOrEqual:
            case Token::Type::GreaterThan:
            case Token::Type::GreaterThanOrEqual: return 4;
            case Token::Type::Minus:
            case Token::Type::Plus:             return 5;
            case Token::Type::Slash:
            case Token::Type::Star:             return 6;
            case Token::Type::Unknown:          return (std::numeric_limits<int>::max)();
            default:                            return -1;
        }
    }

    std::unique_ptr<BlockStmt> Parser::ParseBlockStmt()
    {
        auto block = std::unique_ptr<BlockStmt>(new BlockStmt());
        while (!IsAtEnd())
        {
            switch (Peek().type)
            {
                case Token::Type::Include:
                    block->statements.push_back(ParseIncludeStmt());
                    break;
                case Token::Type::HscppIf:
                    block->statements.push_back(ParseHscppIfStmt());
                    break;
                case Token::Type::HscppReturn:
                    block->statements.push_back(ParseHscppReturnStmt());
                    break;
                case Token::Type::HscppElif:
                case Token::Type::HscppElse:
                case Token::Type::HscppEnd:
                    return block;
                case Token::Type::HscppRequireSource:
                case Token::Type::HscppRequireIncludeDir:
                case Token::Type::HscppRequireLibrary:
                case Token::Type::HscppRequireLibraryDir:
                case Token::Type::HscppRequirePreprocessorDef:
                    block->statements.push_back(ParseHscppRequireStmt());
                    break;
                case Token::Type::HscppModule:
                case Token::Type::HscppTrack:
                    block->statements.push_back(ParseHscppModuleStmt());
                    break;
                case Token::Type::HscppMessage:
                    block->statements.push_back(ParseHscppMessageStmt());
                    break;
                default:
                    Consume();
                    break;
            }
        }
        return block;
    }

    std::unique_ptr<Stmt> Parser::ParseIncludeStmt()
    {
        auto s = std::unique_ptr<IncludeStmt>(new IncludeStmt());
        Consume();
        Expect(Token::Type::String,
               LangError(LangError::Code::Parser_IncludeMissingPath, Prev().line, {}));
        s->path = Peek().value;
        Consume();
        return s;
    }

    std::unique_ptr<Stmt> Parser::ParseHscppIfStmt()
    {
        auto pIf = std::unique_ptr<HscppIfStmt>(new HscppIfStmt());
        std::string ifName;
        while (!IsAtEnd())
        {
            ifName = Peek().value;
            if (Peek().type == Token::Type::HscppIf || Peek().type == Token::Type::HscppElif)
            {
                Consume();
                Expect(Token::Type::LeftParen,
                       LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                                Peek().line, { ifName }));
                Consume();
                pIf->conditions.push_back(ParseExpr());
                Expect(Token::Type::RightParen,
                       LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                                Peek().line, { ifName }));
                Consume();
                pIf->conditionalBlocks.push_back(ParseBlockStmt());
            }
            else if (Peek().type == Token::Type::HscppElse)
            {
                Consume();
                Expect(Token::Type::LeftParen,
                       LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                                Peek().line, { ifName }));
                Consume();
                Expect(Token::Type::RightParen,
                       LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                                Peek().line, { ifName }));
                Consume();
                pIf->pElseBlock = ParseBlockStmt();
            }
            else break;
        }
        Expect(Token::Type::HscppEnd,
               LangError(LangError::Code::Parser_HscppIfStmtMissingHscppEnd,
                        Peek().line, { ifName }));
        Consume();
        Expect(Token::Type::LeftParen,
               LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                        Peek().line, { "skr_end" }));
        Consume();
        Expect(Token::Type::RightParen,
               LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                        Peek().line, { "skr_end" }));
        Consume();
        return pIf;
    }

    std::unique_ptr<Stmt> Parser::ParseHscppReturnStmt()
    {
        auto s = std::unique_ptr<HscppReturnStmt>(new HscppReturnStmt());
        Consume();
        Expect(Token::Type::LeftParen,
               LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                        Peek().line, { "skr_return" }));
        Consume();
        Expect(Token::Type::RightParen,
               LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                        Peek().line, { "skr_return" }));
        Consume();
        return s;
    }

    std::unique_ptr<Stmt> Parser::ParseHscppRequireStmt()
    {
        auto s = std::unique_ptr<HscppRequireStmt>(new HscppRequireStmt());
        s->token = Peek();
        Consume();
        Expect(Token::Type::LeftParen,
               LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                        Peek().line, { s->token.value }));
        Consume();
        while (!IsAtEnd())
        {
            if (s->token.type == Token::Type::HscppRequirePreprocessorDef)
            {
                if (Peek().type != Token::Type::String && Peek().type != Token::Type::Identifier)
                {
                    ThrowError(LangError(
                        LangError::Code::Parser_HscppStmtExpectedStringLiteralOrIdentifierInArgumentList,
                        Peek().line, { s->token.value }));
                }
            }
            else
            {
                Expect(Token::Type::String,
                       LangError(LangError::Code::Parser_HscppStmtExpectedStringLiteralInArgumentList,
                                Peek().line, { s->token.value }));
            }
            s->parameters.push_back(Peek().value);
            Consume();
            if (Peek().type == Token::Type::RightParen) break;
            Expect(Token::Type::Comma,
                   LangError(LangError::Code::Parser_HscppStmtMissingCommaInArgumentList,
                            Peek().line, Peek().column, { s->token.value }));
            Consume();
        }
        Expect(Token::Type::RightParen,
               LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                        Peek().line, { s->token.value }));
        Consume();
        return s;
    }

    std::unique_ptr<Stmt> Parser::ParseHscppModuleStmt()
    {
        auto s = std::unique_ptr<HscppModuleStmt>(new HscppModuleStmt());
        if (Peek().type == Token::Type::HscppModule)
        {
            Consume();
            Expect(Token::Type::LeftParen,
                   LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                            Peek().line, { "skr_module" }));
            Consume();
            Expect(Token::Type::String,
                   LangError(LangError::Code::Parser_HscppStmtArgumentMustBeStringLiteral,
                            Peek().line, { "skr_module" }));
            s->module = Peek().value;
            Consume();
            Expect(Token::Type::RightParen,
                   LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                            Peek().line, { "skr_module" }));
            Consume();
        }
        else
        {
            Consume();
            Expect(Token::Type::LeftParen,
                   LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                            Peek().line, { "SKR_TRACK" }));
            Consume();
            Expect(Token::Type::Identifier,
                   LangError(LangError::Code::Parser_HscppTrackMissingIdentifier, Peek().line, {}));
            Consume();
            Expect(Token::Type::Comma,
                   LangError(LangError::Code::Parser_HscppStmtMissingCommaInArgumentList,
                            Peek().line, Peek().column, { "SKR_TRACK" }));
            Consume();
            Expect(Token::Type::String,
                   LangError(LangError::Code::Parser_HscppTrackMissingString, Peek().line, {}));
            s->module = "@" + Peek().value;
            Consume();
            Expect(Token::Type::RightParen,
                   LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                            Peek().line, { "SKR_TRACK" }));
            Consume();
        }
        return s;
    }

    std::unique_ptr<Stmt> Parser::ParseHscppMessageStmt()
    {
        auto s = std::unique_ptr<HscppMessageStmt>(new HscppMessageStmt());
        Consume();
        Expect(Token::Type::LeftParen,
               LangError(LangError::Code::Parser_HscppStmtMissingOpeningParen,
                        Peek().line, { "skr_message" }));
        Consume();
        Expect(Token::Type::String,
               LangError(LangError::Code::Parser_HscppStmtArgumentMustBeStringLiteral,
                        Peek().line, { "skr_message" }));
        s->message = Peek().value;
        Consume();
        Expect(Token::Type::RightParen,
               LangError(LangError::Code::Parser_HscppStmtMissingClosingParen,
                        Peek().line, { "skr_message" }));
        Consume();
        return s;
    }

    const Token& Parser::Peek()
    {
        if (m_iToken < m_pTokens->size()) return m_pTokens->at(m_iToken);
        return m_DefaultToken;
    }
    const Token& Parser::Prev()
    {
        if (m_iToken > 0 && m_iToken <= m_pTokens->size()) return m_pTokens->at(m_iToken - 1);
        return m_DefaultToken;
    }
    void Parser::Consume() { ++m_iToken; }
    bool Parser::IsAtEnd() { return m_iToken >= m_pTokens->size(); }
    void Parser::ThrowError(const LangError& error)
    {
        m_Error = error;
        throw std::runtime_error("");
    }
    void Parser::Expect(Token::Type tokenType, const LangError& error)
    {
        if (tokenType != Peek().type) ThrowError(error);
    }
} // namespace skr::hotswap
