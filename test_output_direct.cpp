#include "xeus-sas/sas_session.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing xeus-sas output cleaning..." << std::endl;
    std::cout << "====================================" << std::endl;

    try {
        xeus_sas::sas_session session;

        // Test 1: Simple %put
        std::cout << "\nTest 1: %put hello world;" << std::endl;
        std::cout << "Expected: Only 'hello world' in output" << std::endl;
        std::cout << "Unwanted: 'XEUS_SAS_END_*' or numbered input" << std::endl;
        std::cout << "------------------------------------" << std::endl;

        auto result1 = session.execute("%put hello world;");

        std::cout << "Log output:" << std::endl;
        std::cout << result1.log << std::endl;
        std::cout << "------------------------------------" << std::endl;

        // Check for markers
        if (result1.log.find("XEUS_SAS_END") != std::string::npos) {
            std::cout << "\n✗ FAILURE: Execution marker found in output!" << std::endl;
            return 1;
        }

        // Check for input echo (numbered lines like "5    %put hello world;")
        if (result1.log.find("    %put hello world;") != std::string::npos) {
            std::cout << "\n✗ FAILURE: Input echo found in output!" << std::endl;
            return 1;
        }

        // Check that we see the expected output
        if (result1.log.find("hello world") != std::string::npos) {
            std::cout << "\n✓ Test 1 PASSED: Output is clean!" << std::endl;
        } else {
            std::cout << "\n✗ FAILURE: Expected output 'hello world' not found!" << std::endl;
            return 1;
        }

        // Test 2: Multiple %puts
        std::cout << "\nTest 2: Multiple executions" << std::endl;
        std::cout << "------------------------------------" << std::endl;

        auto result2 = session.execute("%put second test;");
        std::cout << "Log output:" << std::endl;
        std::cout << result2.log << std::endl;
        std::cout << "------------------------------------" << std::endl;

        // Check for markers
        if (result2.log.find("XEUS_SAS_END") != std::string::npos) {
            std::cout << "\n✗ FAILURE: Execution marker found in second test!" << std::endl;
            return 1;
        }

        std::cout << "\n✓ Test 2 PASSED: Second execution is also clean!" << std::endl;

        std::cout << "\n====================================" << std::endl;
        std::cout << "✓ ALL TESTS PASSED!" << std::endl;
        std::cout << "  - No execution markers" << std::endl;
        std::cout << "  - No input echo" << std::endl;
        std::cout << "  - Only actual SAS output visible" << std::endl;

        session.shutdown();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
