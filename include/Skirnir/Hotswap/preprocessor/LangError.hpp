#pragma once

#include <vector>
#include <string>
#include <limits>

namespace skr::hotswap
{
    class LangError
    {
      public:
        static constexpr std::size_t NO_VALUE = (std::numeric_limits<std::size_t>::max)();

        enum class Code
        {
            Success,

            Lexer_UnterminatedString,

            Parser_FailedToParsePrefixExpression,
            Parser_FailedToParseInfixExpression,
            Parser_FailedToParseNumber,
            Parser_NumberIsOutOfRange,
            Parser_GroupExpressionMissingClosingParen,
            Parser_IncludeMissingPath,
            Parser_HscppIfStmtMissingHscppEnd,
            Parser_HscppStmtMissingOpeningParen,
            Parser_HscppStmtMissingClosingParen,
            Parser_HscppStmtArgumentMustBeStringLiteral,
            Parser_HscppStmtMissingCommaInArgumentList,
            Parser_HscppStmtExpectedStringLiteralInArgumentList,
            Parser_HscppStmtExpectedStringLiteralOrIdentifierInArgumentList,
            Parser_HscppTrackMissingIdentifier,
            Parser_HscppTrackMissingString,

            Interpreter_UnableToResolveName,

            Variant_OperandMustBeNumber,
            Variant_OperandsDifferInType,

            InternalError,
        };

        explicit LangError(Code errorCode);
        LangError(Code errorCode,
                  std::size_t line,
                  std::size_t column,
                  const std::vector<std::string>& args);
        LangError(Code errorCode, std::size_t line, const std::vector<std::string>& args);
        LangError(Code errorCode, const std::vector<std::string>& args);

        [[nodiscard]] Code ErrorCode() const;
        [[nodiscard]] std::size_t Line() const;

        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] std::size_t NumArgs() const;
        [[nodiscard]] std::string GetArg(std::size_t i) const;

      private:
        Code m_ErrorCode;
        std::size_t m_Line = NO_VALUE;
        std::size_t m_Column = NO_VALUE;

        std::vector<std::string> m_Args;
    };

    class LangErrorCodeHasher
    {
      public:
        std::size_t operator()(LangError::Code errorCode) const;
    };
} // namespace skr::hotswap
