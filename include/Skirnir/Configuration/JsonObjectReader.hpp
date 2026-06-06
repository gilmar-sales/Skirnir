#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Skirnir/Configuration/Aliases.hpp"

namespace SKIRNIR_NAMESPACE
{
    /**
     *
     * Looks up keys case-sensitively in the underlying JSON object and
     * converts the value to the requested type. Missing or type-mismatched
     * keys are reported as failures and leave the destination untouched.
     */
    class JsonObjectReader
    {
      public:
        explicit JsonObjectReader(const simdjson::dom::object& obj) : mObj(obj)
        {
        }

        /** @brief Returns true if @p key is present in the object. */
        bool Contains(std::string_view key) const
        {
            for (auto kv : mObj)
            {
                if (kv.key == key)
                    return true;
            }
            return false;
        }

        /**
         * @brief Tries to read @p key into @p out. Returns true on success.
         */
        template <typename T>
        bool TryGet(std::string_view key, T& out) const
        {
            for (auto kv : mObj)
            {
                if (kv.key == key)
                {
                    return Assign(out, kv.value);
                }
            }
            return false;
        }

      private:
        template <typename T>
        static bool Assign(T& field, simdjson::dom::element el);

        template <typename T>
        static constexpr bool is_vector_of_string_v = false;

        template <typename Alloc>
        static constexpr bool
            is_vector_of_string_v<std::vector<std::string, Alloc>> = true;

        const simdjson::dom::object& mObj;
    };

    template <typename T>
    bool JsonObjectReader::Assign(T& field, simdjson::dom::element el)
    {
        using U = std::remove_cvref_t<T>;

        if constexpr (std::is_same_v<U, bool>)
        {
            if (!el.is_bool())
                return false;
            bool v = false;
            if (el.get_bool().get(v) != simdjson::SUCCESS)
                return false;
            field = v;
            return true;
        }
        else if constexpr (std::is_integral_v<U>)
        {
            if (el.is_int64())
            {
                int64_t v = 0;
                if (el.get_int64().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            if (el.is_uint64())
            {
                uint64_t v = 0;
                if (el.get_uint64().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            if (el.is_double())
            {
                double v = 0.0;
                if (el.get_double().get(v) != simdjson::SUCCESS)
                    return false;
                field = static_cast<U>(v);
                return true;
            }
            return false;
        }
        else if constexpr (std::is_same_v<U, double>)
        {
            if (!el.is_number())
                return false;
            double v = 0.0;
            if (el.get_double().get(v) != simdjson::SUCCESS)
                return false;
            field = v;
            return true;
        }
        else if constexpr (std::is_same_v<U, std::string>)
        {
            if (el.is_string())
            {
                std::string_view sv;
                if (el.get_string().get(sv) != simdjson::SUCCESS)
                    return false;
                field = std::string(sv);
                return true;
            }
            return false;
        }
        else if constexpr (is_vector_of_string_v<U>)
        {
            if (!el.is_array())
                return false;
            for (auto item : el.get_array())
            {
                if (!item.is_string())
                    return false;
                std::string_view sv;
                if (item.get_string().get(sv) != simdjson::SUCCESS)
                    return false;
                field.emplace_back(sv);
            }
            return true;
        }
        else
        {
            // Nested reflected struct.
            if (!el.is_object())
                return false;
            U                nested {};
            JsonObjectReader nestedReader(el.get_object());

            template for (constexpr auto member : std::define_static_array(
                              std::meta::nonstatic_data_members_of(
                                  ^^U,
                                  std::meta::access_context::current())))
            {
                nestedReader.TryGet(std::meta::identifier_of(member),
                                    nested.[:member:]);
            }
            field = std::move(nested);
            return true;
        }
    }
} // namespace SKIRNIR_NAMESPACE
