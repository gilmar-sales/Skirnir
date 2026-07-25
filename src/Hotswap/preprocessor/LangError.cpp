#include <unordered_map>

#include "Skirnir/Hotswap/preprocessor/LangError.hpp"

#include "Skirnir/Hotswap/Platform.hpp"

namespace skr::hotswap
{
    namespace
    {
        const std::unordered_map<LangError::Code, std::string, LangErrorCodeHasher>
            ERRORS = {
                { LangError::Code::Success, "" },
                { LangError::Code::Lexer_UnterminatedString,
                  "Unterminated string, expected a '$1'. String opens on line $2, beginning with '$3'." },
                { LangError::Code::Parser_FailedToParsePrefixExpression,
                  "'$1' is not a supported unary operator." },
                { LangError::Code::Parser_FailedToParseInfixExpression,
                  "'$1' is not a supported binary operator." },
                { LangError::Code::Parser_FailedToParseNumber,
                  "Failed to parse Number with value '$1'." },
                { LangError::Code::Parser_NumberIsOutOfRange,
                  "Number '$1' is out of range." },
                { LangError::Code::Parser_GroupExpressionMissingClosingParen,
                  "Subexpression missing closing ')'." },
                { LangError::Code::Parser_IncludeMissingPath,
                  "#include statement missing path." },
                { LangError::Code::Parser_HscppIfStmtMissingHscppEnd,
                  "$1 missing skr_end() at end of block." },
                { LangError::Code::Parser_HscppStmtMissingOpeningParen,
                  "$1 missing opening '('." },
                { LangError::Code::Parser_HscppStmtMissingClosingParen,
                  "$1 missing closing ')'." },
                { LangError::Code::Parser_HscppStmtArgumentMustBeStringLiteral,
                  "$1 argument must be a string literal." },
                { LangError::Code::Parser_HscppStmtMissingCommaInArgumentList,
                  "$1 missing comma in argument list." },
                { LangError::Code::Parser_HscppStmtExpectedStringLiteralInArgumentList,
                  "$1 expected string literal in argument list." },
                { LangError::Code::Parser_HscppStmtExpectedStringLiteralOrIdentifierInArgumentList,
                  "$1 expected string literal or identifier in argument list." },
                { LangError::Code::Parser_HscppTrackMissingIdentifier,
                  "SKR_TRACK expects first argument to be an identifier." },
                { LangError::Code::Parser_HscppTrackMissingString,
                  "SKR_TRACK expects second argument to be a unique string literal." },
                { LangError::Code::Interpreter_UnableToResolveName,
                  "Unable to resolve variable name '$1'." },
                { LangError::Code::Variant_OperandMustBeNumber,
                  "Operand of '$1' must be Number." },
                { LangError::Code::Variant_OperandsDifferInType,
                  "Operands of '$1' differ in type ('$2' and '$3')." },
                { LangError::Code::InternalError, "Internal error." },
            };
    } // namespace

    LangError::LangError(Code errorCode) : m_ErrorCode(errorCode)
    {
    }
    LangError::LangError(Code errorCode, std::size_t line, std::size_t column,
                         const std::vector<std::string>& args)
        : m_ErrorCode(errorCode), m_Line(line), m_Column(column), m_Args(args)
    {
    }
    LangError::LangError(Code errorCode, std::size_t line,
                         const std::vector<std::string>& args)
        : m_ErrorCode(errorCode), m_Line(line), m_Args(args)
    {
    }
    LangError::LangError(Code errorCode, const std::vector<std::string>& args)
        : m_ErrorCode(errorCode), m_Args(args)
    {
    }

    LangError::Code LangError::ErrorCode() const { return m_ErrorCode; }
    std::size_t LangError::Line() const { return m_Line; }
    std::size_t LangError::NumArgs() const { return m_Args.size(); }
    std::string LangError::GetArg(std::size_t i) const { return m_Args.at(i); }

    std::string LangError::ToString() const
    {
        auto it = ERRORS.find(m_ErrorCode);
        if (it == ERRORS.end())
        {
            log::Error() << SKR_HOTSWAP_LOG_PREFIX
                         << "Invalid error code '"
                         << static_cast<int>(m_ErrorCode) << log::End("'.");
            return "";
        }
        std::string out = it->second;
        for (std::size_t i = 0; i < m_Args.size(); ++i)
        {
            std::string needle = "$" + std::to_string(i + 1);
            std::size_t pos = out.find(needle);
            if (pos != std::string::npos)
            {
                out.replace(pos, needle.size(), m_Args.at(i));
            }
        }
        if (m_Line != NO_VALUE || m_Column != NO_VALUE)
        {
            out += " (";
            if (m_Line != NO_VALUE)
            {
                out += "Line: " + std::to_string(m_Line);
                if (m_Column != NO_VALUE) out += ", ";
            }
            if (m_Column != NO_VALUE)
            {
                out += "Column: " + std::to_string(m_Column);
            }
            out += ")";
        }
        return out;
    }

    std::size_t LangErrorCodeHasher::operator()(LangError::Code errorCode) const
    {
        return static_cast<std::size_t>(errorCode);
    }
} // namespace skr::hotswap
