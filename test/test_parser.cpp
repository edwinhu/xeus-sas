#include <gtest/gtest.h>
#include "xeus-sas/sas_parser.hpp"

using namespace xeus_sas;

TEST(ParserTest, ParseEmptyOutput)
{
    std::string empty_output = "";
    auto result = parse_execution_output(empty_output);

    EXPECT_FALSE(result.is_error);
    EXPECT_EQ(result.error_code, 0);
}

TEST(ParserTest, DetectError)
{
    std::string log_with_error = "ERROR: Invalid syntax on line 5.";
    int error_code = 0;

    bool has_error = contains_error(log_with_error, error_code);

    EXPECT_TRUE(has_error);
    EXPECT_GT(error_code, 0);
}

TEST(ParserTest, NoError)
{
    std::string log_no_error = "NOTE: The procedure completed successfully.";
    int error_code = 0;

    bool has_error = contains_error(log_no_error, error_code);

    EXPECT_FALSE(has_error);
}

TEST(ParserTest, ExtractWarnings)
{
    std::string log = "WARNING: Data may be incomplete.\nNOTE: Processing.\nWARNING: Check results.";
    auto warnings = extract_warnings(log);

    EXPECT_EQ(warnings.size(), 2);
}

TEST(ParserTest, ColorizeLog)
{
    std::string log = "ERROR: Test error\nWARNING: Test warning\nNOTE: Test note";
    std::string colorized = colorize_log(log);

    EXPECT_GT(colorized.length(), log.length()); // Should have ANSI codes
    EXPECT_NE(colorized.find("\033["), std::string::npos); // Contains ANSI codes
}

TEST(ParserTest, StripAnsiCodes)
{
    std::string text_with_ansi = "\033[31mRed text\033[0m normal";
    std::string stripped = strip_ansi_codes(text_with_ansi);

    EXPECT_EQ(stripped, "Red text normal");
}

TEST(ParserTest, GenerateUniqueMarkers)
{
    std::string marker1 = generate_execution_marker();
    std::string marker2 = generate_execution_marker();

    EXPECT_NE(marker1, marker2); // Should be unique
    EXPECT_GT(marker1.length(), 0);
}

TEST(ParserTest, ShouldShowListing)
{
    execution_result result;

    // Case 1: Error - should show log
    result.is_error = true;
    result.listing = "Some output";
    EXPECT_FALSE(should_show_listing(result));

    // Case 2: No error, has listing - should show listing
    result.is_error = false;
    result.listing = "Procedure output";
    EXPECT_TRUE(should_show_listing(result));

    // Case 3: No error, empty listing - should show log
    result.is_error = false;
    result.listing = "";
    EXPECT_FALSE(should_show_listing(result));
}
