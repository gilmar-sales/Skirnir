#pragma once

#include <string>

#include "Skirnir/Hotswap/preprocessor/Token.hpp"
#include "Skirnir/Hotswap/preprocessor/LangError.hpp"

namespace skr::hotswap
{
    class Variant
    {
      public:
        enum class Type
        {
            Unknown,
            String,
            Number,
            Bool,
        };

        Variant() = default;
        explicit Variant(const std::string& val);
        explicit Variant(const char* pVal);
        explicit Variant(double val);
        explicit Variant(bool val);

        [[nodiscard]] Type GetType() const;
        [[nodiscard]] std::string GetTypeName() const;

        [[nodiscard]] bool IsString() const;
        [[nodiscard]] bool IsNumber() const;
        [[nodiscard]] bool IsBool() const;

        [[nodiscard]] bool IsTruthy() const;

        [[nodiscard]] std::string StringVal() const;
        [[nodiscard]] double NumberVal() const;
        [[nodiscard]] bool BoolVal() const;

        [[nodiscard]] std::string ToString() const;

      private:
        Type m_Type = Type::Unknown;

        std::string m_StringVal;
        double m_NumberVal = 0;
        bool m_BoolVal = false;
    };

    bool UnaryOp(const Token& op, const Variant& rhs, Variant& result, LangError& error);
    bool BinaryOp(const Token& op,
                  const Variant& lhs,
                  const Variant& rhs,
                  Variant& result,
                  LangError& error);
} // namespace skr::hotswap
