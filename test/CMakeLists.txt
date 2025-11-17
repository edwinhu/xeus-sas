# Test configuration for xeus-sas

find_package(GTest)

if(NOT GTest_FOUND)
    message(WARNING "Google Test not found. Skipping tests.")
    return()
endif()

# Test source files
set(TEST_SOURCES
    test_parser.cpp
    test_session.cpp
    test_completion.cpp
)

# Create test executable
add_executable(test_xeus_sas ${TEST_SOURCES})

# Link libraries
target_link_libraries(test_xeus_sas
    PRIVATE
        GTest::GTest
        GTest::Main
        nlohmann_json::nlohmann_json
)

# Include directories
target_include_directories(test_xeus_sas
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Add test sources (need to link against implementation)
target_sources(test_xeus_sas
    PRIVATE
        ../src/sas_parser.cpp
        ../src/sas_session.cpp
        ../src/completion.cpp
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_xeus_sas)
