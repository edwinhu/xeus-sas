#include <gtest/gtest.h>
#include "xeus-sas/sas_session.hpp"

using namespace xeus_sas;

// Note: These tests require a working SAS installation
// They are disabled by default and should be run manually
// when SAS is available

TEST(SessionTest, DISABLED_CreateSession)
{
    // This test requires SAS to be installed
    EXPECT_NO_THROW({
        sas_session session;
        EXPECT_TRUE(session.is_ready());
    });
}

TEST(SessionTest, DISABLED_ExecuteSimpleCode)
{
    sas_session session;

    std::string code = "DATA test; x = 1; RUN;";
    auto result = session.execute(code);

    EXPECT_FALSE(result.is_error);
}

TEST(SessionTest, DISABLED_ExecuteWithError)
{
    sas_session session;

    std::string code = "INVALID SAS CODE;";
    auto result = session.execute(code);

    EXPECT_TRUE(result.is_error);
    EXPECT_GT(result.error_code, 0);
}

TEST(SessionTest, DISABLED_GetVersion)
{
    sas_session session;

    std::string version = session.get_version();
    EXPECT_FALSE(version.empty());
}

TEST(SessionTest, DISABLED_MacroVariable)
{
    sas_session session;

    // Set macro variable
    session.set_macro("test_var", "test_value");

    // Get macro variable
    std::string value = session.get_macro("test_var");
    EXPECT_EQ(value, "test_value");
}

// Mock test that doesn't require SAS
TEST(SessionTest, SessionStructure)
{
    // Test that execution_result structure is properly defined
    execution_result result;
    result.log = "Test log";
    result.listing = "Test listing";
    result.is_error = false;
    result.error_code = 0;
    result.error_message = "";

    EXPECT_EQ(result.log, "Test log");
    EXPECT_EQ(result.listing, "Test listing");
    EXPECT_FALSE(result.is_error);
}
