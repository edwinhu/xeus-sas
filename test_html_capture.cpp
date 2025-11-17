#include <iostream>
#include <fstream>
#include "src/sas_session.cpp"
#include "src/sas_parser.cpp"

using namespace xeus_sas;

int main() {
    try {
        sas_session session;
        
        std::string code = R"(
PROC PRINT DATA=sashelp.class(obs=5);
RUN;
)";
        
        std::cout << "Executing SAS code..." << std::endl;
        auto result = session.execute(code);
        
        // Write raw HTML to file
        std::ofstream html_file("/tmp/xeus_sas_raw_html.html");
        html_file << result.html_output;
        html_file.close();
        
        std::cout << "\n=== RAW HTML OUTPUT (first 1000 chars) ===" << std::endl;
        std::cout << result.html_output.substr(0, 1000) << std::endl;
        std::cout << "\n... [middle section truncated] ...\n" << std::endl;
        
        if (result.html_output.length() > 1000) {
            std::cout << "=== RAW HTML OUTPUT (last 500 chars) ===" << std::endl;
            size_t start = result.html_output.length() > 500 ? result.html_output.length() - 500 : 0;
            std::cout << result.html_output.substr(start) << std::endl;
        }
        
        std::cout << "\n=== HTML INFO ===" << std::endl;
        std::cout << "Has HTML: " << result.has_html << std::endl;
        std::cout << "HTML Length: " << result.html_output.length() << std::endl;
        std::cout << "Contains <!DOCTYPE: " << (result.html_output.find("<!DOCTYPE") != std::string::npos) << std::endl;
        std::cout << "Contains </html>: " << (result.html_output.find("</html>") != std::string::npos) << std::endl;
        std::cout << "\nFull HTML saved to: /tmp/xeus_sas_raw_html.html" << std::endl;
        
        std::cout << "\n=== LOG OUTPUT ===" << std::endl;
        std::cout << result.log << std::endl;
        
        session.shutdown();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
