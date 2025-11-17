#include <gtest/gtest.h>
#include "xeus-sas/completion.hpp"
#include "xeus-sas/sas_session.hpp"

using namespace xeus_sas;

TEST(CompletionTest, ExtractToken)
{
    // Create a mock session (nullptr is okay for basic tests)
    completion_engine engine(nullptr);

    std::string code = "PROC MEAN";
    int cursor_pos = 9; // After "MEAN"
    int start_pos = 0;

    // Note: extract_token is private, so we test via get_completions
    // This test is a placeholder for the public API
}

TEST(CompletionTest, ProcedureCompletions)
{
    completion_engine engine(nullptr);

    std::string code = "PROC ME";
    int cursor_pos = 7;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Should include MEANS
    EXPECT_GT(completions.size(), 0);
    bool found_means = false;
    for (const auto& comp : completions)
    {
        if (comp.find("MEANS") != std::string::npos)
        {
            found_means = true;
            break;
        }
    }
    EXPECT_TRUE(found_means);
}

TEST(CompletionTest, DataStepCompletions)
{
    completion_engine engine(nullptr);

    std::string code = "DATA test; SE";
    int cursor_pos = 13;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Should include SET
    bool found_set = false;
    for (const auto& comp : completions)
    {
        if (comp.find("SET") != std::string::npos)
        {
            found_set = true;
            break;
        }
    }
    EXPECT_TRUE(found_set);
}

TEST(CompletionTest, MacroCompletions)
{
    completion_engine engine(nullptr);

    std::string code = "%LE";
    int cursor_pos = 3;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Should include %LET
    bool found_let = false;
    for (const auto& comp : completions)
    {
        if (comp.find("LET") != std::string::npos)
        {
            found_let = true;
            break;
        }
    }
    EXPECT_TRUE(found_let);
}

TEST(CompletionTest, FunctionCompletions)
{
    completion_engine engine(nullptr);

    std::string code = "x = SU";
    int cursor_pos = 6;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Should include SUM and SUBSTR
    int sum_count = 0;
    for (const auto& comp : completions)
    {
        if (comp.find("SU") == 0)
        {
            sum_count++;
        }
    }
    EXPECT_GT(sum_count, 0);
}

TEST(CompletionTest, EmptyCode)
{
    completion_engine engine(nullptr);

    std::string code = "";
    int cursor_pos = 0;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Should return empty or general completions
    // Either behavior is acceptable
}

TEST(CompletionTest, NoDuplicates)
{
    completion_engine engine(nullptr);

    std::string code = "P";
    int cursor_pos = 1;
    int start_pos = 0;

    auto completions = engine.get_completions(code, cursor_pos, start_pos);

    // Check for duplicates
    std::set<std::string> unique_completions(completions.begin(), completions.end());
    EXPECT_EQ(completions.size(), unique_completions.size());
}
