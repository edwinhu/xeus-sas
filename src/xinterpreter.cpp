#include "xeus-sas/xinterpreter.hpp"
#include "xeus-sas/sas_session.hpp"
#include "xeus-sas/sas_parser.hpp"
#include "xeus-sas/completion.hpp"
#include "xeus-sas/inspection.hpp"
#include "xeus-sas/xeus_sas_config.hpp"

#include "xeus/xinterpreter.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

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
                // Determine what to display
                if (should_show_listing(result))
                {
                    // Show listing output
                    if (!result.listing.empty())
                    {
                        publish_stream("stdout", result.listing);
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
