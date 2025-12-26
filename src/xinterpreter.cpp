#include "xeus-sas/xinterpreter.hpp"
#include "xeus-sas/sas_session.hpp"
#include "xeus-sas/sas_parser.hpp"
#include "xeus-sas/completion.hpp"
#include "xeus-sas/inspection.hpp"
#include "xeus-sas/xeus_sas_config.hpp"

#include "xeus/xinterpreter.hpp"

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <atomic>

namespace xeus_sas
{
    interpreter::interpreter()
        : m_session(nullptr)
        , m_completer(nullptr)
        , m_inspector(nullptr)
    {
        // Initialization will happen in configure_impl()
    }

    interpreter::~interpreter()
    {
        // Cleanup handled by unique_ptr
    }

    void interpreter::handle_interrupt()
    {
        std::cerr << "\n=== INTERRUPT HANDLER CALLED ===" << std::endl;

        if (m_session)
        {
            std::cerr << "Restarting SAS session due to interrupt..." << std::endl;

            // Restart the SAS session (shutdown + reinitialize)
            // This is necessary because SAS running in batch mode (-stdio)
            // does not support graceful interruption. SIGINT would kill the
            // child SAS process, breaking the kernel connection.
            m_session->restart();

            std::cerr << "=== SAS SESSION RESTARTED ===" << std::endl;
            std::cerr << "WARNING: Session state lost (datasets, macro variables cleared)" << std::endl;
            std::cerr << "=================================" << std::endl;

            // Publish warning to user via stderr stream
            publish_stream("stderr",
                "\n⚠️  Kernel interrupted - SAS session restarted\n"
                "    Session state has been lost (WORK datasets, macro variables)\n"
                "    You can continue using the kernel normally.\n"
            );
        }
        else
        {
            std::cerr << "WARNING: Interrupt received but no active session" << std::endl;
        }
    }

    void interpreter::configure_impl()
    {
        // Initialize SAS session
        m_session = std::make_unique<sas_session>();

        // Initialize completion and inspection engines
        m_completer = std::make_unique<completion_engine>(m_session.get());
        m_inspector = std::make_unique<inspection_engine>(m_session.get());
    }

    void interpreter::execute_request_impl(
        xeus::xinterpreter::send_reply_callback cb,
        int execution_counter,
        const std::string& code,
        xeus::execute_request_config config,
        nl::json user_expressions
    )
    {
        // Check if interrupt was requested before execution
        if (g_interrupt_requested.exchange(false, std::memory_order_acquire))
        {
            // Handle interrupt before executing new code
            handle_interrupt();
        }

        // Execute code in SAS session
        auto result = m_session->execute(code);

        // Prepare response
        nl::json response;

        if (result.is_error)
        {
            // Execution error
            response["status"] = "error";
            response["ename"] = "SAS Error";
            response["evalue"] = result.error_message;
            response["traceback"] = nl::json::array({result.log});

            if (!config.silent)
            {
                publish_stream("stderr", colorize_log(result.log));
            }
        }
        else
        {
            // Successful execution
            response["status"] = "ok";
            response["execution_count"] = execution_counter;

            if (!config.silent)
            {
                // Check if we have HTML output
                if (result.has_html && !result.html_output.empty())
                {
                    // === DEBUG LOGGING: HTML BEING SENT ===
                    std::cerr << "=== XINTERPRETER: HTML DEBUG INFO ===" << std::endl;
                    std::cerr << "HTML Length: " << result.html_output.length() << std::endl;
                    std::cerr << "First 300 chars: " << result.html_output.substr(0, 300) << std::endl;
                    if (result.html_output.length() > 300)
                    {
                        size_t start = result.html_output.length() > 300 ? result.html_output.length() - 300 : 0;
                        std::cerr << "Last 300 chars: " << result.html_output.substr(start) << std::endl;
                    }
                    std::cerr << "Contains <!DOCTYPE: " << (result.html_output.find("<!DOCTYPE") != std::string::npos) << std::endl;
                    std::cerr << "Contains </html>: " << (result.html_output.find("</html>") != std::string::npos) << std::endl;
                    std::cerr << "Starts with <!DOCTYPE: " << (result.html_output.substr(0, 15) == "<!DOCTYPE html>") << std::endl;
                    std::cerr << "Ends with </html>: " << (result.html_output.length() >= 7 && result.html_output.substr(result.html_output.length()-7) == "</html>") << std::endl;

                    // Count table rows
                    size_t tr_count = 0;
                    size_t pos = 0;
                    while ((pos = result.html_output.find("</tr>", pos)) != std::string::npos)
                    {
                        tr_count++;
                        pos += 5;
                    }
                    std::cerr << "Number of </tr> tags: " << tr_count << std::endl;

                    // Check for table structure
                    std::cerr << "Contains <table: " << (result.html_output.find("<table") != std::string::npos) << std::endl;
                    std::cerr << "Contains </table>: " << (result.html_output.find("</table>") != std::string::npos) << std::endl;
                    std::cerr << "Contains <colgroup>: " << (result.html_output.find("<colgroup>") != std::string::npos) << std::endl;
                    std::cerr << "Contains <thead>: " << (result.html_output.find("<thead>") != std::string::npos) << std::endl;
                    std::cerr << "Contains <tbody>: " << (result.html_output.find("<tbody>") != std::string::npos) << std::endl;
                    std::cerr << "======================================" << std::endl;

                    // Display rich HTML output using display_data
                    // Inject booktabs-style CSS for tables
                    std::string styled_output = "<style>\n"
                        ".sas-table, .sas-table table, table.table {\n"
                        "  border-collapse: collapse;\n"
                        "  border: none;\n"
                        "}\n"
                        ".sas-table td, .sas-table th,\n"
                        "table.table td, table.table th {\n"
                        "  border: none;\n"
                        "  padding: 4px 8px;\n"
                        "}\n"
                        "/* Toprule: first row with headers */\n"
                        ".sas-table tbody tr:first-child th,\n"
                        ".sas-table tbody tr:first-child td,\n"
                        "table.table tbody tr:first-child th,\n"
                        "table.table tbody tr:first-child td {\n"
                        "  border-top: 2px solid currentcolor;\n"
                        "}\n"
                        "/* Midrule: after header rows (rows with .header class) */\n"
                        ".sas-table tbody tr:has(.header) + tr:not(:has(.header)) td,\n"
                        ".sas-table tbody tr:has(.header) + tr:not(:has(.header)) th,\n"
                        "table.table tbody tr:has(.header) + tr:not(:has(.header)) td,\n"
                        "table.table tbody tr:has(.header) + tr:not(:has(.header)) th {\n"
                        "  border-top: 1px solid currentcolor;\n"
                        "}\n"
                        "/* Bottomrule: last row */\n"
                        ".sas-table tbody tr:last-child td,\n"
                        ".sas-table tbody tr:last-child th,\n"
                        "table.table tbody tr:last-child td,\n"
                        "table.table tbody tr:last-child th {\n"
                        "  border-bottom: 2px solid currentcolor;\n"
                        "}\n"
                        "</style>\n";
                    styled_output += result.html_output;

                    nl::json html_data;
                    html_data["text/html"] = styled_output;

                    // Also include plain text fallback (log)
                    if (!result.log.empty())
                    {
                        html_data["text/plain"] = result.log;
                    }

                    // Debug: Check what's in the JSON
                    std::cerr << "=== JSON DEBUG INFO ===" << std::endl;
                    std::cerr << "JSON has text/html key: " << html_data.contains("text/html") << std::endl;
                    if (html_data.contains("text/html"))
                    {
                        std::string json_html = html_data["text/html"].get<std::string>();
                        std::cerr << "JSON text/html length: " << json_html.length() << std::endl;
                        std::cerr << "JSON text/html matches result.html_output: " << (json_html == result.html_output) << std::endl;
                    }
                    std::cerr << "=======================" << std::endl;

                    display_data(html_data, nl::json::object(), nl::json::object());
                }
                else
                {
                    // Fallback to plain text output
                    // Determine what to display
                    if (should_show_listing(result))
                    {
                        // Show listing output with theme-adaptive styling
                        if (!result.listing.empty())
                        {
                            // Strip XEUS_SAS_END markers from listing
                            std::string clean_listing = result.listing;
                            std::regex marker_regex(R"(XEUS_SAS_END_\d+\s*)");
                            clean_listing = std::regex_replace(clean_listing, marker_regex, "");

                            // Trim trailing whitespace
                            clean_listing.erase(clean_listing.find_last_not_of(" \n\r\t") + 1);

                            if (!clean_listing.empty())
                            {
                                // Wrap in styled HTML for consistent appearance
                                std::string styled_html = "<style>\n"
                                    ".sas-listing {\n"
                                    "  font-family: ui-monospace, 'Cascadia Code', 'Source Code Pro', Menlo, 'DejaVu Sans Mono', Consolas, monospace;\n"
                                    "  font-size: 12px;\n"
                                    "  font-variant-ligatures: none;\n"
                                    "  color: inherit;\n"
                                    "  background-color: transparent;\n"
                                    "  padding: 10px;\n"
                                    "  border: 1px solid currentcolor;\n"
                                    "  border-radius: 3px;\n"
                                    "  opacity: 0.6;\n"
                                    "  overflow-x: auto;\n"
                                    "  margin: 0;\n"
                                    "  line-height: 1.4;\n"
                                    "  white-space: pre;\n"
                                    "}\n"
                                    "</style>\n"
                                    "<pre class=\"sas-listing\">";

                                // HTML escape the listing content
                                for (char c : clean_listing)
                                {
                                    switch (c)
                                    {
                                        case '<': styled_html += "&lt;"; break;
                                        case '>': styled_html += "&gt;"; break;
                                        case '&': styled_html += "&amp;"; break;
                                        default: styled_html += c;
                                    }
                                }
                                styled_html += "</pre>";

                                nl::json html_data;
                                html_data["text/html"] = styled_html;
                                html_data["text/plain"] = clean_listing;
                                publish_execution_result(execution_counter, std::move(html_data), nl::json::object());
                            }
                        }
                    }
                    else
                    {
                        // Show log (for debugging or when no listing)
                        if (!result.log.empty())
                        {
                            publish_stream("stdout", colorize_log(result.log));
                        }
                    }
                }

                // Display graphics if any
                if (!result.graph_files.empty())
                {
                    display_graphics(result.graph_files);
                }
            }
        }

        // Handle user expressions (if any)
        if (!user_expressions.is_null())
        {
            nl::json user_expr_results;
            for (auto it = user_expressions.begin(); it != user_expressions.end(); ++it)
            {
                // For now, just return empty results
                user_expr_results[it.key()] = nl::json::object();
            }
            response["user_expressions"] = user_expr_results;
        }

        // Send the response via callback
        cb(response);
    }

    nl::json interpreter::complete_request_impl(
        const std::string& code,
        int cursor_pos
    )
    {
        int start_pos = 0;
        auto matches = m_completer->get_completions(code, cursor_pos, start_pos);

        nl::json response;
        response["matches"] = matches;
        response["cursor_start"] = start_pos;
        response["cursor_end"] = cursor_pos;
        response["metadata"] = nl::json::object();
        response["status"] = "ok";

        return response;
    }

    nl::json interpreter::inspect_request_impl(
        const std::string& code,
        int cursor_pos,
        int detail_level
    )
    {
        auto inspection = m_inspector->get_inspection(code, cursor_pos, detail_level);

        nl::json response;
        response["found"] = !inspection.empty();
        response["status"] = "ok";

        if (!inspection.empty())
        {
            response["data"] = nl::json::object();
            response["data"]["text/plain"] = inspection;
            response["metadata"] = nl::json::object();
        }

        return response;
    }

    nl::json interpreter::is_complete_request_impl(const std::string& code)
    {
        nl::json response;

        // Simple heuristic: check for unclosed statements
        // SAS statements typically end with semicolon
        std::string trimmed = code;
        // Remove trailing whitespace
        trimmed.erase(trimmed.find_last_not_of(" \n\r\t") + 1);

        if (trimmed.empty())
        {
            response["status"] = "incomplete";
            response["indent"] = "";
        }
        else if (trimmed.back() == ';')
        {
            // Check for unclosed PROC or DATA blocks
            // Count DATA/PROC vs RUN/QUIT
            size_t data_count = 0, proc_count = 0;
            size_t run_count = 0, quit_count = 0;

            std::istringstream iss(code);
            std::string line;
            while (std::getline(iss, line))
            {
                if (line.find("DATA ") != std::string::npos ||
                    line.find("data ") != std::string::npos)
                    data_count++;
                if (line.find("PROC ") != std::string::npos ||
                    line.find("proc ") != std::string::npos)
                    proc_count++;
                if (line.find("RUN;") != std::string::npos ||
                    line.find("run;") != std::string::npos)
                    run_count++;
                if (line.find("QUIT;") != std::string::npos ||
                    line.find("quit;") != std::string::npos)
                    quit_count++;
            }

            size_t blocks = data_count + proc_count;
            size_t closures = run_count + quit_count;

            if (blocks > closures)
            {
                response["status"] = "incomplete";
                response["indent"] = "  ";
            }
            else
            {
                response["status"] = "complete";
            }
        }
        else
        {
            response["status"] = "incomplete";
            response["indent"] = "";
        }

        return response;
    }

    nl::json interpreter::kernel_info_request_impl()
    {
        nl::json response;

        response["implementation"] = "xeus-sas";
        response["implementation_version"] = xeus_sas::version;

        response["language_info"]["name"] = "sas";
        response["language_info"]["version"] = "";  // Will be filled by SAS version
        response["language_info"]["mimetype"] = "text/x-sas";
        response["language_info"]["file_extension"] = ".sas";
        response["language_info"]["pygments_lexer"] = "sas";
        response["language_info"]["codemirror_mode"] = "sas";

        response["protocol_version"] = "5.3";
        response["banner"] =
            "xeus-sas - SAS Jupyter Kernel\n"
            "Version: " + std::string(xeus_sas::version) + "\n"
            "Native C++ implementation using xeus framework";

        response["help_links"] = nl::json::array({
            nl::json::object({
                {"text", "SAS Documentation"},
                {"url", "https://documentation.sas.com/"}
            }),
            nl::json::object({
                {"text", "xeus-sas Repository"},
                {"url", "https://github.com/jupyter-xeus/xeus-sas"}
            })
        });

        response["status"] = "ok";

        return response;
    }

    void interpreter::shutdown_request_impl()
    {
        if (m_session)
        {
            m_session->shutdown();
        }
    }

    void interpreter::display_graphics(const std::vector<std::string>& graph_files)
    {
        for (const auto& file : graph_files)
        {
            // Read image file
            std::ifstream ifs(file, std::ios::binary);
            if (!ifs)
            {
                std::cerr << "Failed to open graph file: " << file << std::endl;
                continue;
            }

            // Read file contents
            std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)),
                                     std::istreambuf_iterator<char>());

            // Determine MIME type
            std::string mime_type = "image/png";
            if (file.find(".svg") != std::string::npos)
            {
                mime_type = "image/svg+xml";
            }

            // Publish display data
            nl::json data;
            if (mime_type == "image/svg+xml")
            {
                data[mime_type] = std::string(buffer.begin(), buffer.end());
            }
            else
            {
                // Base64 encode binary data
                // For now, just indicate the file path
                data["text/plain"] = "Graph: " + file;
            }

            display_data(data, nl::json::object(), nl::json::object());
        }
    }

} // namespace xeus_sas
