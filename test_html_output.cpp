#include "xeus-sas/sas_session.hpp"
#include <iostream>

int main() {
    std::cout << "Testing xeus-sas HTML output..." << std::endl;

    try {
        xeus_sas::sas_session session;

        std::string code = R"(
PROC PRINT DATA=sashelp.class;
RUN;
)";

        std::cout << "\nExecuting code:\n" << code << std::endl;
        auto result = session.execute(code);

        std::cout << "\n=== EXECUTION RESULT ===\n" << std::endl;
        std::cout << "has_html: " << (result.has_html ? "YES" : "NO") << std::endl;
        std::cout << "is_error: " << (result.is_error ? "YES" : "NO") << std::endl;

        if (result.has_html) {
            std::cout << "\nHTML output length: " << result.html_output.length() << " chars" << std::endl;
            std::cout << "\nHTML preview (first 500 chars):\n" << result.html_output.substr(0, 500) << std::endl;

            // Check for key HTML elements
            if (result.html_output.find("<!DOCTYPE html>") != std::string::npos ||
                result.html_output.find("<html") != std::string::npos) {
                std::cout << "\n✅ SUCCESS: Valid HTML detected!" << std::endl;
            } else {
                std::cout << "\n⚠️  WARNING: HTML flag set but no HTML structure found" << std::endl;
            }
        } else {
            std::cout << "\n❌ FAILED: No HTML output detected" << std::endl;
            std::cout << "\nRaw output:\n" << result.html_output.substr(0, 500) << std::endl;
        }

        std::cout << "\n=== LOG OUTPUT ===\n" << std::endl;
        std::cout << result.log << std::endl;

        session.shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
