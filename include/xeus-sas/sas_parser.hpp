#ifndef XEUS_SAS_PARSER_HPP
#define XEUS_SAS_PARSER_HPP

#include <string>
#include <vector>
#include "sas_session.hpp"

namespace xeus_sas
{
    /**
     * @brief Parse raw SAS output into structured execution_result
     *
     * SAS output consists of:
     * - Log stream (contains NOTEs, WARNINGs, ERRORs, and execution details)
     * - Listing stream (contains PROC output, tables, results)
     *
     * This function separates the two streams and extracts metadata.
     *
     * @param raw_output Combined output from SAS
     * @return execution_result with separated log and listing
     */
    execution_result parse_execution_output(const std::string& raw_output);

    /**
     * @brief Extract error information from SAS log
     *
     * Searches for ERROR: lines in the log and extracts:
     * - Error code (if present)
     * - Error message
     *
     * @param log SAS log content
     * @param error_code Output parameter for error code
     * @return true if errors were found
     */
    bool contains_error(const std::string& log, int& error_code);

    /**
     * @brief Extract warning messages from SAS log
     *
     * @param log SAS log content
     * @return Vector of warning messages
     */
    std::vector<std::string> extract_warnings(const std::string& log);

    /**
     * @brief Extract graph file paths from SAS log
     *
     * ODS graphics generates PNG/SVG files and logs their paths.
     * This function extracts those paths for display in Jupyter.
     *
     * @param log SAS log content
     * @return Vector of graph file paths
     */
    std::vector<std::string> extract_graph_files(const std::string& log);

    /**
     * @brief Add ANSI color codes to SAS log for terminal display
     *
     * Colors:
     * - ERROR: red
     * - WARNING: yellow
     * - NOTE: blue
     *
     * @param log SAS log content
     * @return Colorized log
     */
    std::string colorize_log(const std::string& log);

    /**
     * @brief Strip ANSI escape sequences from text
     *
     * SAS may output ANSI codes on some platforms.
     * This function removes them for clean display.
     *
     * @param text Text with potential ANSI codes
     * @return Text without ANSI codes
     */
    std::string strip_ansi_codes(const std::string& text);

    /**
     * @brief Generate a unique execution marker
     *
     * Used to delimit SAS code blocks for parsing output.
     *
     * @return Unique marker string
     */
    std::string generate_execution_marker();

    /**
     * @brief Determine if listing should be shown instead of log
     *
     * Decision logic:
     * - If errors present: show log
     * - If only warnings: show listing (with warnings highlighted)
     * - If clean execution: show listing
     *
     * @param result Execution result
     * @return true if listing should be displayed
     */
    bool should_show_listing(const execution_result& result);

} // namespace xeus_sas

#endif // XEUS_SAS_PARSER_HPP
