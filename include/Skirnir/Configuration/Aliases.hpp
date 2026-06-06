#pragma once


#define SIMDJSON_STATIC_REFLECTION 1
#include <simdjson.h>

namespace SKIRNIR_NAMESPACE
{
    using ConfigurationElement = simdjson::dom::element;
    using ConfigurationObject  = simdjson::dom::object;
    using ConfigurationArray   = simdjson::dom::array;
} // namespace SKIRNIR_NAMESPACE
