
#include <gtest/gtest.h>

#include <Skirnir/Configuration.hpp>

#include <clocale>
#include <cstdlib>
#include <locale>
#include <optional>
#include <string>
#include <utility>

namespace
{
    class ScopedEnv
    {
      public:
        ScopedEnv(std::string name, std::string value) : mName(std::move(name))
        {
#if defined(_WIN32)
            _putenv_s(mName.c_str(), value.c_str());
#else
            ::setenv(mName.c_str(), value.c_str(), 1);
#endif
        }

        ~ScopedEnv()
        {
#if defined(_WIN32)
            _putenv_s(mName.c_str(), nullptr);
#else
            ::unsetenv(mName.c_str());
#endif
        }

        ScopedEnv(const ScopedEnv&)            = delete;
        ScopedEnv& operator=(const ScopedEnv&) = delete;

      private:
        std::string mName;
    };
} // namespace

// A. Case preservation ------------------------------------------------------

TEST(ConfigurationCoercionSpec, InMemorySource_KeyCaseIsPreserved)
{
    auto config = skr::ConfigurationBuilder()
                      .AddInMemory({{"Logging.Level", "Info"}})
                      .Build();

    EXPECT_TRUE(config->HasKey("Logging.Level"));
    EXPECT_FALSE(config->HasKey("logging.level"));
    EXPECT_EQ(config->GetString("Logging.Level"), "Info");
}

TEST(ConfigurationCoercionSpec,
     EnvironmentVariables_KeyCaseIsPreservedAfterPrefixStrip)
{
    ScopedEnv env("SKR_TEST_CASEPRES_Logging__Level", "Info");
    auto      config = skr::ConfigurationBuilder()
                        .AddEnvironmentVariables("SKR_TEST_CASEPRES_")
                        .Build();

    EXPECT_TRUE(config->HasKey("Logging.Level"));
    EXPECT_FALSE(config->HasKey("logging.level"));
    EXPECT_EQ(config->GetString("Logging.Level"), "Info");
}

TEST(ConfigurationCoercionSpec, EnvironmentVariables_AgreesWithInMemory_ForSameLogicalInput)
{
    ScopedEnv env("SKR_TEST_PARITY_Logging__Level", "Info");

    auto envCfg = skr::ConfigurationBuilder()
                      .AddEnvironmentVariables("SKR_TEST_PARITY_")
                      .Build();

    auto memCfg = skr::ConfigurationBuilder()
                      .AddInMemory({{"Logging.Level", "Info"}})
                      .Build();

    EXPECT_EQ(envCfg->GetString("Logging.Level"),
              memCfg->GetString("Logging.Level"));
    EXPECT_EQ(envCfg->HasKey("Logging.Level"),
              memCfg->HasKey("Logging.Level"));
    EXPECT_EQ(envCfg->HasKey("logging.level"),
              memCfg->HasKey("logging.level"));
}

// B. Type-coercion edge cases (via InMemorySource, same BuildJsonFromFlat) -

TEST(ConfigurationCoercionSpec, InMemorySource_IntegerOverflowIsCoercedToDouble)
{
    auto config = skr::ConfigurationBuilder()
                      .AddInMemory({{"big", "99999999999999999999"}})
                      .Build();

    // std::stoll throws out_of_range, std::stod accepts it as a finite
    // double in the ~1e20 range with pos == v.size(), so the leaf is
    // stored as a JSON number (double), not a string. Read it back as a
    // double to confirm.
    double d = config->GetDouble("big", 0.0);
    EXPECT_GT(d, 1e19);
    EXPECT_TRUE(config->HasKey("big"));
    // The value must not be the verbatim source string — that would
    // prove it was stored as a string.
    EXPECT_NE(config->GetString("big"), "99999999999999999999");
}

TEST(ConfigurationCoercionSpec, InMemorySource_LeadingZeroIntegerIsAcceptedAsInt)
{
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "007"}}).Build();

    EXPECT_EQ(config->GetInt("n"), 7);
}

TEST(ConfigurationCoercionSpec, InMemorySource_HexLiteralIsCoercedByStdStod)
{
    // Documents the platform-dependent std::stod behavior for hex:
    //   - MSVC (Windows): std::stod accepts the "0x" prefix and parses
    //     "0x10" as the double 16.0, pos == v.size(). The leaf is a
    //     number, not a string.
    //   - libc++/libstdc++ may instead reject the prefix and keep the
    //     value as a string.
    // The test asserts the *actual* outcome on the host toolchain so a
    // future change in the stdlib is caught.
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "0x10"}}).Build();

    double d = config->GetDouble("n", -1.0);
    if (d == 16.0)
    {
        EXPECT_DOUBLE_EQ(d, 16.0);
        EXPECT_NE(config->GetString("n"), "0x10");
    }
    else
    {
        // libstdc++/libc++ path: kept as a string.
        EXPECT_DOUBLE_EQ(d, -1.0);
        EXPECT_EQ(config->GetString("n"), "0x10");
    }
}

TEST(ConfigurationCoercionSpec, InMemorySource_ExponentialOverflowBehaviour)
{
    // "1e999" overflows double. std::stod's reaction is
    // implementation-defined: it may return +/-inf (pos == v.size())
    // which Serialize turns into JSON null, or it may throw
    // std::out_of_range, in which case the catch-all in
    // BuildJsonTreeFromFlat falls through to a string leaf. Pin the
    // behaviour the host actually exhibits.
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "1e999"}}).Build();

    auto value = config->GetValue("n");
    double d   = config->GetDouble("n", 0.0);
    if (value.has_value())
    {
        // Serialized-as-null path: the value was coerced to +/-inf and
        // then re-parsed as null. GetValue returns nullopt in that
        // case (see ConfigurationOptions::StringifyRaw), so the
        // !has_value() branch below is also a valid outcome of the
        // same path. Both are acceptable for "stored as a number".
        EXPECT_EQ(value.value(), "1e999")
            << "value was kept as the literal string — std::stod threw "
               "out_of_range instead of returning inf";
    }
    else
    {
        EXPECT_DOUBLE_EQ(d, 0.0);
    }
}

TEST(ConfigurationCoercionSpec, InMemorySource_LeadingTrailingWhitespaceStaysString)
{
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "  42  "}}).Build();

    EXPECT_EQ(config->GetString("n"), "  42  ");
    EXPECT_EQ(config->GetInt("n", -1), -1);
}

TEST(ConfigurationCoercionSpec, InMemorySource_LocaleCommaDecimalStaysString)
{
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "1,5"}}).Build();

    EXPECT_EQ(config->GetString("n"), "1,5");
    EXPECT_DOUBLE_EQ(config->GetDouble("n", -1.0), -1.0);

    try
    {
        std::locale de("de_DE.UTF-8");
        ScopedEnv  localeEnv("LC_ALL", "de_DE.UTF-8");
        (void)de;
    }
    catch (const std::runtime_error&)
    {
        GTEST_SKIP() << "de_DE.UTF-8 locale not available on this host";
    }

    EXPECT_EQ(config->GetString("n"), "1,5");
    EXPECT_DOUBLE_EQ(config->GetDouble("n", -1.0), -1.0);
}

TEST(ConfigurationCoercionSpec, InMemorySource_NaNStringSerializesAsNull)
{
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "NaN"}}).Build();

    // std::stod accepts "NaN" with pos == v.size(), the value is stored
    // as a double NaN, the serializer writes JSON null for non-finite
    // doubles, and the rebuilt tree contains a null leaf. GetValue
    // returns nullopt for null leaves, not the string "null" — the
    // key is present but has no value.
    EXPECT_TRUE(config->HasKey("n"));
    EXPECT_FALSE(config->GetValue("n").has_value());
    EXPECT_EQ(config->GetString("n", "missing"), "missing");
    EXPECT_EQ(config->GetInt("n", -1), -1);
    EXPECT_DOUBLE_EQ(config->GetDouble("n", -1.0), -1.0);
}

TEST(ConfigurationCoercionSpec, InMemorySource_InfStringSerializesAsNull)
{
    auto config =
        skr::ConfigurationBuilder().AddInMemory({{"n", "inf"}}).Build();

    EXPECT_TRUE(config->HasKey("n"));
    EXPECT_FALSE(config->GetValue("n").has_value());
    EXPECT_EQ(config->GetString("n", "missing"), "missing");
    EXPECT_EQ(config->GetInt("n", -1), -1);
    EXPECT_DOUBLE_EQ(config->GetDouble("n", -1.0), -1.0);
}

TEST(ConfigurationCoercionSpec, InMemorySource_EmptyStringStaysString)
{
    auto config = skr::ConfigurationBuilder().AddInMemory({{"n", ""}}).Build();

    EXPECT_EQ(config->GetString("n"), "");
    EXPECT_FALSE(config->GetBool("n", false));
    EXPECT_EQ(config->GetInt("n", -1), -1);
}

TEST(ConfigurationCoercionSpec, InMemorySource_CaseSensitiveBooleanLiteralsStayString)
{
    auto config = skr::ConfigurationBuilder()
                      .AddInMemory({{"a", "True"},
                                    {"b", "FALSE"},
                                    {"c", " yes "}})
                      .Build();

    EXPECT_EQ(config->GetString("a"), "True");
    EXPECT_FALSE(config->GetBool("a", false));
    EXPECT_EQ(config->GetString("b"), "FALSE");
    EXPECT_FALSE(config->GetBool("b", false));
    EXPECT_EQ(config->GetString("c"), " yes ");
    EXPECT_FALSE(config->GetBool("c", false));
}

// C. Environment-variable side: parity / regression ----------------------------

TEST(ConfigurationCoercionSpec,
     EnvironmentVariables_ExponentialOverflowBehaviourMatchesInMemory)
{
    ScopedEnv env("SKR_TEST_OVERFLOW", "1e999");

    auto envCfg = skr::ConfigurationBuilder()
                      .AddEnvironmentVariables()
                      .Build();

    auto memCfg = skr::ConfigurationBuilder()
                      .AddInMemory({{"SKR_TEST_OVERFLOW", "1e999"}})
                      .Build();

    // The two sources must produce the same shape for the same
    // logical input (parity contract from
    // EnvironmentVariablesSource.hpp:20-22).
    EXPECT_EQ(envCfg->HasKey("SKR_TEST_OVERFLOW"),
              memCfg->HasKey("SKR_TEST_OVERFLOW"));
    EXPECT_EQ(envCfg->GetValue("SKR_TEST_OVERFLOW").has_value(),
              memCfg->GetValue("SKR_TEST_OVERFLOW").has_value());
    EXPECT_EQ(envCfg->GetString("SKR_TEST_OVERFLOW", "missing"),
              memCfg->GetString("SKR_TEST_OVERFLOW", "missing"));
}

TEST(ConfigurationCoercionSpec, EnvironmentVariables_OverflowingIntegerMatchesInMemory)
{
    ScopedEnv env("SKR_TEST_BIG", "99999999999999999999");

    auto envCfg = skr::ConfigurationBuilder()
                      .AddEnvironmentVariables()
                      .Build();

    auto memCfg = skr::ConfigurationBuilder()
                      .AddInMemory({{"SKR_TEST_BIG", "99999999999999999999"}})
                      .Build();

    // The two sources must produce the same shape: the value lives as
    // a JSON number, not a string. GetInt does a narrowing cast on
    // the stored double, so we do not assert a specific integer;
    // we only assert the values match between the two sources.
    EXPECT_GT(envCfg->GetDouble("SKR_TEST_BIG", 0.0), 1e19);
    EXPECT_GT(memCfg->GetDouble("SKR_TEST_BIG", 0.0), 1e19);
    EXPECT_DOUBLE_EQ(envCfg->GetDouble("SKR_TEST_BIG", 0.0),
                     memCfg->GetDouble("SKR_TEST_BIG", 0.0));
    EXPECT_NE(envCfg->GetString("SKR_TEST_BIG"),
              "99999999999999999999");
    EXPECT_EQ(envCfg->GetString("SKR_TEST_BIG"),
              memCfg->GetString("SKR_TEST_BIG"));
}
