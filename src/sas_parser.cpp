#include "xeus-sas/sas_parser.hpp"

#include <algorithm>
#include <regex>
#include <sstream>
#include <random>

namespace xeus_sas
{
    execution_result parse_execution_output(const std::string& raw_output)
    {
        execution_result result;

        // Split output into log and listing sections
        size_t log_pos = raw_output.find("=== LOG ===");
        size_t listing_pos = raw_output.find("=== LISTING ===");

        if (log_pos != std::string::npos && listing_pos != std::string::npos)
        {
            // Extract log
            size_t log_start = log_pos + 12; // length of "=== LOG ===" + newline
            size_t log_end = listing_pos;
            result.log = raw_output.substr(log_start, log_end - log_start);

            // Extract listing
            size_t listing_start = listing_pos + 16; // length of "=== LISTING ===" + newline
            result.listing = raw_output.substr(listing_start);
        }
        else
        {
            // Fallback: treat everything as log
            result.log = raw_output;
        }

        // Check for errors
        int error_code = 0;
        result.is_error = contains_error(result.log, error_code);
        result.error_code = error_code;

        if (result.is_error)
        {
            // Extract error message
            std::regex error_regex(R"(ERROR:?\s*(.+))");
            std::smatch match;
            if (std::regex_search(result.log, match, error_regex))
            {
                result.error_message = match[1].str();
            }
        }

        // Extract graph files
        result.graph_files = extract_graph_files(result.log);

        return result;
    }

    bool contains_error(const std::string& log, int& error_code)
    {
        // Search for ERROR: lines
        std::regex error_regex(R"(ERROR(?:\s+(\d+))?:\s*)", std::regex::icase);
        std::smatch match;

        if (std::regex_search(log, match, error_regex))
        {
            if (match.size() > 1 && match[1].matched)
            {
                error_code = std::stoi(match[1].str());
            }
            else
            {
                error_code = 1; // Generic error code
            }
            return true;
        }

        return false;
    }

    std::vector<std::string> extract_warnings(const std::string& log)
    {
        std::vector<std::string> warnings;

        // Search for WARNING: lines
        std::regex warning_regex(R"(WARNING:\s*(.+))");
        std::sregex_iterator iter(log.begin(), log.end(), warning_regex);
        std::sregex_iterator end;

        while (iter != end)
        {
            warnings.push_back((*iter)[1].str());
            ++iter;
        }

        return warnings;
    }

    std::vector<std::string> extract_graph_files(const std::string& log)
    {
        std::vector<std::string> graph_files;

        // Look for ODS graphics output
        // Example: NOTE: Writing HTML Body file: /path/to/graph.png
        std::regex graph_regex(R"(NOTE:.*?(?:file|File):\s*([^\s]+\.(?:png|svg|jpg|jpeg|gif)))",
                               std::regex::icase);
        std::sregex_iterator iter(log.begin(), log.end(), graph_regex);
        std::sregex_iterator end;

        while (iter != end)
        {
            graph_files.push_back((*iter)[1].str());
            ++iter;
        }

        return graph_files;
    }

    std::string colorize_log(const std::string& log)
    {
        std::stringstream result;
        std::istringstream iss(log);
        std::string line;

        // ANSI color codes
        const std::string RED = "\033[31m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string RESET = "\033[0m";

        while (std::getline(iss, line))
        {
            if (line.find("ERROR") != std::string::npos)
            {
                result << RED << line << RESET << std::endl;
            }
            else if (line.find("WARNING") != std::string::npos)
            {
                result << YELLOW << line << RESET << std::endl;
            }
            else if (line.find("NOTE") != std::string::npos)
            {
                result << BLUE << line << RESET << std::endl;
            }
            else
            {
                result << line << std::endl;
            }
        }

        return result.str();
    }

    std::string strip_ansi_codes(const std::string& text)
    {
        // Remove ANSI escape sequences
        // Match ESC [ <numbers and semicolons> m
        std::regex ansi_regex("\x1B\\[[0-9;]*m");
        return std::regex_replace(text, ansi_regex, "");
    }

    std::string generate_execution_marker()
    {
        // Generate a unique marker for execution tracking
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(100000, 999999);

        return "XEUS_SAS_MARKER_" + std::to_string(dis(gen));
    }

    bool should_show_listing(const execution_result& result)
    {
        // Decision logic for what to display:
        // 1. If errors: show log
        // 2. If listing is empty: show log
        // 3. Otherwise: show listing

        if (result.is_error)
        {
            return false; // Show log
        }

        // Check if listing has meaningful content
        std::string trimmed_listing = result.listing;
        trimmed_listing.erase(
            std::remove_if(trimmed_listing.begin(), trimmed_listing.end(), ::isspace),
            trimmed_listing.end()
        );

        if (trimmed_listing.empty())
        {
            return false; // Show log
        }

        return true; // Show listing
    }

} // namespace xeus_sas
