#include "xeus-sas/sas_session.hpp"
#include "xeus-sas/sas_parser.hpp"
#include "xeus-sas/xeus_sas_config.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <array>
#include <cstdio>
#include <memory>
#include <regex>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#endif

namespace xeus_sas
{
    // PIMPL implementation
    class sas_session::impl
    {
    public:
        impl(const std::string& sas_path);
        ~impl();

        execution_result execute(const std::string& code);
        std::string get_version();
        bool is_ready() const;
        void shutdown();
        void interrupt();
        void restart();
        std::string get_macro(const std::string& name);
        void set_macro(const std::string& name, const std::string& value);

    private:
        std::string m_sas_path;
        bool m_initialized;
        pid_t m_sas_pid;
        FILE* m_sas_stdin;
        FILE* m_sas_stdout;
        FILE* m_sas_stderr;

        void initialize_session();
        std::string find_sas_executable(const std::string& path_hint);
        std::string run_sas_batch(const std::string& code);
    };

    sas_session::impl::impl(const std::string& sas_path)
        : m_sas_path(sas_path)
        , m_initialized(false)
        , m_sas_pid(-1)
        , m_sas_stdin(nullptr)
        , m_sas_stdout(nullptr)
        , m_sas_stderr(nullptr)
    {
        // Find SAS executable
        if (m_sas_path.empty())
        {
            m_sas_path = find_sas_executable("");
        }

        if (m_sas_path.empty())
        {
            throw std::runtime_error(
                "SAS executable not found. Please set SAS_PATH environment variable."
            );
        }

        std::cout << "Using SAS: " << m_sas_path << std::endl;
    }

    sas_session::impl::~impl()
    {
        shutdown();
    }

    std::string sas_session::impl::find_sas_executable(const std::string& path_hint)
    {
        // Check environment variable first
        const char* env_path = std::getenv("SAS_PATH");
        if (env_path)
        {
            return std::string(env_path);
        }

        // Check default path from build configuration
        if (std::string(xeus_sas::default_sas_path).length() > 0)
        {
            return std::string(xeus_sas::default_sas_path);
        }

        // Try common locations
        std::vector<std::string> search_paths = {
            "/usr/local/SASHome/SASFoundation/9.4/bin/sas",
            "/usr/local/SAS/SASFoundation/9.4/bin/sas",
            "/opt/SASHome/SASFoundation/9.4/bin/sas",
            "/Applications/SASHome/SASFoundation/9.4/sas.app/Contents/MacOS/sas",
            "/Applications/SASHome/SASFoundation/9.4/bin/sas"
        };

        for (const auto& path : search_paths)
        {
            if (access(path.c_str(), X_OK) == 0)
            {
                return path;
            }
        }

        return "";
    }

    void sas_session::impl::initialize_session()
    {
        if (m_initialized)
            return;

        std::cout << "Initializing persistent SAS session..." << std::endl;

#ifndef _WIN32
        // Create pipes for stdin, stdout, and stderr
        int stdin_pipe[2];
        int stdout_pipe[2];
        int stderr_pipe[2];

        if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0)
        {
            throw std::runtime_error("Failed to create pipes for SAS communication");
        }

        // Fork and exec SAS
        m_sas_pid = fork();

        if (m_sas_pid == 0)
        {
            // Child process - redirect stdin/stdout/stderr and exec SAS
            dup2(stdin_pipe[0], STDIN_FILENO);
            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stderr_pipe[1], STDERR_FILENO);

            close(stdin_pipe[0]);
            close(stdin_pipe[1]);
            close(stdout_pipe[0]);
            close(stdout_pipe[1]);
            close(stderr_pipe[0]);
            close(stderr_pipe[1]);

            // Start SAS in interactive mode
            // -nodms: no display manager
            // -rsasuser: reuse sasuser library (faster startup)
            // -noovp: no OVP processing
            // -nosyntaxcheck: no pre-execution syntax check (faster execution)
            // -nonews: suppress startup news
            // -noaltlog: no alternate log
            // -noaltprint: no alternate print
            // -stdio: use stdin/stdout for I/O
            execlp(m_sas_path.c_str(), m_sas_path.c_str(),
                   "-nodms", "-rsasuser", "-noovp", "-nosyntaxcheck",
                   "-nonews", "-noaltlog", "-noaltprint", "-stdio", nullptr);

            // If exec fails
            std::cerr << "Failed to execute SAS: " << m_sas_path << std::endl;
            exit(1);
        }
        else if (m_sas_pid > 0)
        {
            // Parent process - setup file streams
            close(stdin_pipe[0]);
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            m_sas_stdin = fdopen(stdin_pipe[1], "w");
            m_sas_stdout = fdopen(stdout_pipe[0], "r");
            m_sas_stderr = fdopen(stderr_pipe[0], "r");

            if (!m_sas_stdin || !m_sas_stdout || !m_sas_stderr)
            {
                throw std::runtime_error("Failed to create file streams for SAS communication");
            }

            // Set stdin to line buffering for immediate writes
            setvbuf(m_sas_stdin, nullptr, _IOLBF, 0);
            // CRITICAL: Set stdout and stderr to unbuffered since we use read() on their fds
            setvbuf(m_sas_stdout, nullptr, _IONBF, 0);
            setvbuf(m_sas_stderr, nullptr, _IONBF, 0);

            // Note: SAS outputs startup messages (copyright, version, etc.) when started
            // with -stdio. These will appear in the first execution's output.
            // This is acceptable behavior - the important part is that subsequent
            // executions don't restart SAS (which would show these messages again).

            m_initialized = true;
            std::cout << "Persistent SAS session initialized (PID: " << m_sas_pid << ")" << std::endl;
        }
        else
        {
            throw std::runtime_error("Failed to fork process for SAS");
        }
#else
        throw std::runtime_error("Windows not yet supported for persistent sessions");
#endif
    }

    execution_result sas_session::impl::execute(const std::string& code)
    {
        // Initialize persistent session if not already done
        if (!m_initialized)
        {
            initialize_session();
        }

#ifndef _WIN32
        // Generate unique marker for this execution
        static int exec_counter = 0;
        std::string marker = "XEUS_SAS_END_" + std::to_string(++exec_counter);

        // Wrap code with ODS HTML5 commands for rich output
        // Following sas_kernel's approach:
        // - Close default listing destination
        // - Open HTML5 destination to stdout
        // - Enable inline graphics as base64
        // - Execute user code
        // - Close HTML5 and restore listing
        std::cerr << "\n=== CODE RECEIVED ===\n" << code << "\n=====================\n" << std::endl;

        // Allow user to override ODS style via environment variable
        // Default to HTMLBlue if not set
        const char* user_style = std::getenv("XEUS_SAS_ODS_STYLE");
        std::string ods_style = user_style ? user_style : "HTMLBlue";

        std::stringstream wrapped_code;
        wrapped_code << "ods listing close;\n"
                     << "ods html5 (id=xeus_sas_internal) file=stdout style=" << ods_style << ";\n"
                     << "ods graphics on / outputfmt=png;\n"
                     << "\n"
                     << code << "\n"
                     << "\n"
                     << "ods html5 (id=xeus_sas_internal) close;\n"
                     << "ods listing;\n"
                     << "* Force flush of all output before marker;\n"
                     << "DATA _null_; run;\n";

        // Send wrapped code to SAS
        fprintf(m_sas_stdin, "%s\n", wrapped_code.str().c_str());

        // Send marker to stderr (log stream) to detect end of output
        // Note: We add a DATA _null_; RUN; after the marker to force SAS to flush output
        fprintf(m_sas_stdin, "%%put %s;\n", marker.c_str());
        fprintf(m_sas_stdin, "DATA _null_; run;\n");
        fflush(m_sas_stdin);

        // Read both stdout (HTML) and stderr (log) until we see the marker
        // We need to read both streams to avoid deadlock
        // IMPORTANT: We must wait for BOTH conditions:
        // 1. Marker found in stderr (log)
        // 2. Complete HTML document received (if HTML is present)
        std::string html_output;
        std::string log_output;
        char buffer[8192];  // Larger buffer for better performance
        bool found_marker = false;
        bool found_html_end = false;
        bool has_html_start = false;

        // Set stderr to non-blocking mode for polling
        int stderr_fd = fileno(m_sas_stderr);
        int stdout_fd = fileno(m_sas_stdout);
        int stderr_flags = fcntl(stderr_fd, F_GETFL, 0);
        int stdout_flags = fcntl(stdout_fd, F_GETFL, 0);
        fcntl(stderr_fd, F_SETFL, stderr_flags | O_NONBLOCK);
        fcntl(stdout_fd, F_SETFL, stdout_flags | O_NONBLOCK);

        // Poll both streams until we find the marker in stderr
        struct pollfd fds[2];
        fds[0].fd = stdout_fd;
        fds[0].events = POLLIN;
        fds[1].fd = stderr_fd;
        fds[1].events = POLLIN;

        // Continue reading until we have both marker AND complete HTML
        int timeout_count = 0;
        int consecutive_empty_reads = 0;
        const int max_timeouts = 30;  // Allow up to 30 seconds total
        const int max_empty_reads = 5;  // If we get 5 consecutive empty reads, we're probably done

        while (!found_marker || (has_html_start && !found_html_end))
        {
            int poll_result = poll(fds, 2, 1000); // 1 second timeout
            if (poll_result < 0)
            {
                std::cerr << "Poll error in SAS output reading" << std::endl;
                break; // Error
            }
            else if (poll_result == 0)
            {
                // No data available right now
                timeout_count++;
                consecutive_empty_reads++;

                // If we have the marker and either no HTML or found HTML end, we can stop
                if (found_marker && (!has_html_start || found_html_end))
                {
                    break;
                }

                // If we've had too many consecutive empty reads after finding marker, give up
                if (found_marker && consecutive_empty_reads >= max_empty_reads)
                {
                    std::cerr << "WARNING: No more data after " << consecutive_empty_reads << " empty reads" << std::endl;
                    std::cerr << "  Marker found: " << found_marker << std::endl;
                    std::cerr << "  HTML start found: " << has_html_start << std::endl;
                    std::cerr << "  HTML end found: " << found_html_end << std::endl;
                    std::cerr << "  HTML length so far: " << html_output.length() << std::endl;
                    break;
                }

                if (timeout_count >= max_timeouts)
                {
                    std::cerr << "WARNING: Timeout waiting for complete SAS output" << std::endl;
                    std::cerr << "  Marker found: " << found_marker << std::endl;
                    std::cerr << "  HTML start found: " << has_html_start << std::endl;
                    std::cerr << "  HTML end found: " << found_html_end << std::endl;
                    break;
                }
                continue; // Timeout, try again
            }

            // We got data, reset empty read counter
            consecutive_empty_reads = 0;

            // Read from stdout (HTML)
            if (fds[0].revents & POLLIN)
            {
                ssize_t bytes_read = read(stdout_fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0)
                {
                    buffer[bytes_read] = '\0';  // Null terminate
                    html_output.append(buffer, bytes_read);

                    // Check for HTML document markers
                    if (!has_html_start &&
                        (html_output.find("<!DOCTYPE html>") != std::string::npos ||
                         html_output.find("<html") != std::string::npos))
                    {
                        has_html_start = true;
                    }

                    // Check for HTML end - use rfind to get the LAST occurrence
                    if (has_html_start && !found_html_end)
                    {
                        if (html_output.find("</html>") != std::string::npos)
                        {
                            found_html_end = true;
                        }
                    }
                }
            }

            // Read from stderr (log)
            if (fds[1].revents & POLLIN)
            {
                ssize_t bytes_read = read(stderr_fd, buffer, sizeof(buffer) - 1);
                if (bytes_read > 0)
                {
                    buffer[bytes_read] = '\0';  // Null terminate
                    std::string chunk(buffer, bytes_read);

                    // Check if this chunk contains our marker
                    if (chunk.find(marker) != std::string::npos)
                    {
                        found_marker = true;
                        // Don't add the marker line to log - split and remove it
                        size_t marker_pos = chunk.find(marker);
                        size_t line_start = chunk.rfind('\n', marker_pos);
                        if (line_start != std::string::npos)
                        {
                            log_output += chunk.substr(0, line_start + 1);
                        }
                        // Skip the marker line and add anything after
                        size_t line_end = chunk.find('\n', marker_pos);
                        if (line_end != std::string::npos && line_end + 1 < chunk.length())
                        {
                            log_output += chunk.substr(line_end + 1);
                        }
                    }
                    else
                    {
                        log_output += chunk;
                    }
                }
            }

            // Exit if we have marker and either no HTML or complete HTML
            if (found_marker && (!has_html_start || found_html_end))
            {
                break;
            }
        }

        // Restore blocking mode
        fcntl(stderr_fd, F_SETFL, stderr_flags);
        fcntl(stdout_fd, F_SETFL, stdout_flags);

        // Extract clean HTML if present
        std::string clean_html;
        bool has_html = false;

        if (has_html_start)
        {
            // === DEBUG: Log raw HTML output before extraction ===
            std::cerr << "\n=== SAS_SESSION: RAW HTML OUTPUT DEBUG ===" << std::endl;
            std::cerr << "Raw HTML output length: " << html_output.length() << std::endl;
            std::cerr << "First 400 chars: " << html_output.substr(0, std::min<size_t>(400, html_output.length())) << std::endl;
            if (html_output.length() > 400)
            {
                size_t start = html_output.length() > 400 ? html_output.length() - 400 : 0;
                std::cerr << "Last 400 chars: " << html_output.substr(start) << std::endl;
            }
            std::cerr << "==========================================\n" << std::endl;

            // Find HTML document boundaries
            size_t html_start = html_output.find("<!DOCTYPE html>");
            if (html_start == std::string::npos)
            {
                html_start = html_output.find("<html");
            }

            size_t html_end = html_output.rfind("</html>");  // Use rfind for LAST occurrence

            std::cerr << "=== SAS_SESSION: HTML EXTRACTION DEBUG ===" << std::endl;
            std::cerr << "html_start position: " << html_start << std::endl;
            std::cerr << "html_end position: " << html_end << std::endl;

            if (html_start != std::string::npos && html_end != std::string::npos)
            {
                html_end += 7;  // Include "</html>"
                clean_html = html_output.substr(html_start, html_end - html_start);
                has_html = true;

                // Post-process HTML to fix euporie rendering issues
                // Merge multiple colgroups into one (SAS splits rowheader from data columns)
                size_t colgroup_pos = 0;
                int colgroup_count = 0;
                int total_cols = 0;
                std::vector<size_t> colgroup_positions;

                // Find all colgroups and count columns
                while ((colgroup_pos = clean_html.find("<colgroup>", colgroup_pos)) != std::string::npos)
                {
                    colgroup_positions.push_back(colgroup_pos);
                    colgroup_count++;

                    // Count cols in this colgroup
                    size_t colgroup_end = clean_html.find("</colgroup>", colgroup_pos);
                    size_t search_pos = colgroup_pos;
                    while ((search_pos = clean_html.find("<col/>", search_pos)) != std::string::npos && search_pos < colgroup_end)
                    {
                        total_cols++;
                        search_pos += 6;
                    }

                    colgroup_pos = colgroup_end;
                }

                // If we have multiple colgroups, merge them into one
                if (colgroup_count > 1 && colgroup_positions.size() >= 2)
                {
                    std::cerr << "Merging " << colgroup_count << " colgroups with " << total_cols << " total columns" << std::endl;

                    // Build new single colgroup
                    std::string new_colgroup = "<colgroup>";
                    for (int i = 0; i < total_cols; i++)
                    {
                        new_colgroup += "<col/>";
                    }
                    new_colgroup += "</colgroup>";

                    // Find the range to replace (first colgroup start to last colgroup end)
                    size_t first_colgroup = colgroup_positions[0];
                    size_t last_colgroup_end = clean_html.find("</colgroup>", colgroup_positions.back()) + 11; // +11 for "</colgroup>"

                    // Replace
                    clean_html = clean_html.substr(0, first_colgroup) + new_colgroup + clean_html.substr(last_colgroup_end);
                }

                // Additional simplification: remove inline styles that might confuse terminal renderers
                // Remove style attributes from table and other elements
                size_t style_pos = 0;
                while ((style_pos = clean_html.find(" style=", style_pos)) != std::string::npos)
                {
                    size_t quote_start = clean_html.find("\"", style_pos);
                    if (quote_start != std::string::npos)
                    {
                        size_t quote_end = clean_html.find("\"", quote_start + 1);
                        if (quote_end != std::string::npos)
                        {
                            clean_html.erase(style_pos, quote_end - style_pos + 1);
                        }
                        else
                        {
                            style_pos++;
                        }
                    }
                    else
                    {
                        style_pos++;
                    }
                }

                // Remove aria-label attributes (accessibility but not needed for terminal)
                style_pos = 0;
                while ((style_pos = clean_html.find(" aria-label=", style_pos)) != std::string::npos)
                {
                    size_t quote_start = clean_html.find("\"", style_pos);
                    if (quote_start != std::string::npos)
                    {
                        size_t quote_end = clean_html.find("\"", quote_start + 1);
                        if (quote_end != std::string::npos)
                        {
                            clean_html.erase(style_pos, quote_end - style_pos + 1);
                        }
                        else
                        {
                            style_pos++;
                        }
                    }
                    else
                    {
                        style_pos++;
                    }
                }

                // CRITICAL FIX: Move thead content to be first row of tbody
                // Terminal renderers like euporie may treat thead as floating/fixed
                // This causes misalignment with the table body
                size_t thead_start = clean_html.find("<thead>");
                size_t tbody_start = clean_html.find("<tbody>");

                if (thead_start != std::string::npos && tbody_start != std::string::npos)
                {
                    size_t thead_end = clean_html.find("</thead>", thead_start);
                    if (thead_end != std::string::npos)
                    {
                        // Extract the header row content (between <thead> and </thead>)
                        size_t content_start = thead_start + 7; // After <thead>
                        std::string header_content = clean_html.substr(content_start, thead_end - content_start);

                        // Remove the entire <thead>...</thead> section
                        clean_html.erase(thead_start, thead_end + 8 - thead_start);

                        // Find tbody again (position changed after erase)
                        tbody_start = clean_html.find("<tbody>");
                        if (tbody_start != std::string::npos)
                        {
                            // Insert header content right after <tbody>
                            clean_html.insert(tbody_start + 7, header_content);
                            std::cerr << "Moved thead content to first row of tbody" << std::endl;
                        }
                    }
                }

                // Remove empty caption elements that create extra spacing
                // But keep captions with actual text content
                size_t caption_pos = 0;
                while ((caption_pos = clean_html.find("<caption", caption_pos)) != std::string::npos)
                {
                    size_t caption_end = clean_html.find("</caption>", caption_pos);
                    if (caption_end != std::string::npos)
                    {
                        // Find where caption tag ends (could have attributes)
                        size_t tag_close = clean_html.find(">", caption_pos);
                        if (tag_close != std::string::npos && tag_close < caption_end)
                        {
                            // Extract text between <caption...> and </caption>
                            std::string caption_text = clean_html.substr(tag_close + 1, caption_end - tag_close - 1);

                            // Check if caption is empty or whitespace-only
                            bool is_empty = true;
                            for (char c : caption_text)
                            {
                                if (!std::isspace(static_cast<unsigned char>(c)))
                                {
                                    is_empty = false;
                                    break;
                                }
                            }

                            if (is_empty)
                            {
                                // Remove empty caption
                                clean_html.erase(caption_pos, caption_end + 10 - caption_pos);
                                std::cerr << "Removed empty caption element" << std::endl;
                            }
                            else
                            {
                                // Keep caption with content
                                std::cerr << "Kept caption with text: " << caption_text.substr(0, 30) << "..." << std::endl;
                                caption_pos = caption_end + 10;
                            }
                        }
                        else
                        {
                            caption_pos++;
                        }
                    }
                    else
                    {
                        caption_pos++;
                    }
                }

                // CRITICAL FIX: Flatten rowspan/colspan attributes for PROC TABULATE
                // Terminal renderers like euporie cannot handle complex table spans properly
                // We need to duplicate cells that span multiple rows/columns
                std::cerr << "Flattening rowspan/colspan attributes..." << std::endl;

                // First, parse the table structure to understand row/column layout
                size_t table_start = clean_html.find("<table");
                if (table_start != std::string::npos)
                {
                    size_t table_end = clean_html.find("</table>", table_start);
                    if (table_end != std::string::npos)
                    {
                        // Extract just the table content for processing
                        std::string before_table = clean_html.substr(0, table_start);
                        std::string after_table = clean_html.substr(table_end);
                        std::string table_html = clean_html.substr(table_start, table_end - table_start);

                        // Build a grid representation of the table
                        std::vector<std::vector<std::string>> grid;
                        std::vector<std::vector<bool>> cell_occupied;

                        // Parse rows (both thead and tbody)
                        size_t row_pos = 0;
                        int current_row = 0;

                        while ((row_pos = table_html.find("<tr", row_pos)) != std::string::npos)
                        {
                            size_t row_end = table_html.find("</tr>", row_pos);
                            if (row_end == std::string::npos) break;

                            std::string row_content = table_html.substr(row_pos, row_end - row_pos);

                            // Ensure we have enough rows in our grid
                            if (current_row >= grid.size())
                            {
                                grid.resize(current_row + 1);
                                cell_occupied.resize(current_row + 1);
                            }

                            // Parse cells in this row
                            size_t cell_pos = 0;
                            int current_col = 0;

                            // Find the next available column (skip occupied cells from rowspan)
                            auto find_next_col = [&]() {
                                while (current_col < cell_occupied[current_row].size() &&
                                       cell_occupied[current_row][current_col])
                                {
                                    current_col++;
                                }
                            };

                            while (true)
                            {
                                // Find next cell (th or td)
                                size_t th_pos = row_content.find("<th", cell_pos);
                                size_t td_pos = row_content.find("<td", cell_pos);
                                size_t next_cell = std::min(
                                    th_pos == std::string::npos ? std::string::npos : th_pos,
                                    td_pos == std::string::npos ? std::string::npos : td_pos
                                );

                                if (next_cell == std::string::npos) break;

                                bool is_th = (next_cell == th_pos);
                                std::string cell_tag = is_th ? "th" : "td";
                                size_t cell_end = row_content.find("</" + cell_tag + ">", next_cell);
                                if (cell_end == std::string::npos) break;

                                std::string cell_content = row_content.substr(next_cell, cell_end - next_cell + cell_tag.length() + 3);

                                // Extract rowspan and colspan attributes
                                int rowspan = 1, colspan = 1;
                                size_t rowspan_pos = cell_content.find("rowspan=\"");
                                if (rowspan_pos != std::string::npos)
                                {
                                    size_t value_start = rowspan_pos + 9;
                                    size_t value_end = cell_content.find("\"", value_start);
                                    if (value_end != std::string::npos)
                                    {
                                        rowspan = std::stoi(cell_content.substr(value_start, value_end - value_start));
                                    }
                                }

                                size_t colspan_pos = cell_content.find("colspan=\"");
                                if (colspan_pos != std::string::npos)
                                {
                                    size_t value_start = colspan_pos + 9;
                                    size_t value_end = cell_content.find("\"", value_start);
                                    if (value_end != std::string::npos)
                                    {
                                        colspan = std::stoi(cell_content.substr(value_start, value_end - value_start));
                                    }
                                }

                                // Extract cell inner content (between tags)
                                size_t content_start = cell_content.find(">") + 1;
                                size_t content_end = cell_content.rfind("</");
                                std::string inner_content = cell_content.substr(content_start, content_end - content_start);

                                // Remove rowspan/colspan from the cell tag
                                std::string cleaned_cell = cell_content;
                                if (rowspan_pos != std::string::npos)
                                {
                                    size_t attr_start = rowspan_pos;
                                    size_t attr_end = cell_content.find("\"", rowspan_pos + 9) + 1;
                                    cleaned_cell.erase(attr_start, attr_end - attr_start);
                                    // Remove leading space if present
                                    if (cleaned_cell[attr_start] == ' ') cleaned_cell.erase(attr_start, 1);
                                }
                                if (colspan_pos != std::string::npos)
                                {
                                    // Recalculate position after potential rowspan removal
                                    colspan_pos = cleaned_cell.find("colspan=\"");
                                    if (colspan_pos != std::string::npos)
                                    {
                                        size_t attr_start = colspan_pos;
                                        size_t attr_end = cleaned_cell.find("\"", colspan_pos + 9) + 1;
                                        cleaned_cell.erase(attr_start, attr_end - attr_start);
                                        if (cleaned_cell[attr_start] == ' ') cleaned_cell.erase(attr_start, 1);
                                    }
                                }

                                // Find next available column for this cell
                                find_next_col();

                                // Mark grid positions as occupied for this cell and its span
                                // For rowspan cells, we'll place the content in the LAST row (bottom-aligned)
                                // and use empty cells for the earlier rows
                                for (int r = 0; r < rowspan; r++)
                                {
                                    int row_idx = current_row + r;
                                    if (row_idx >= grid.size())
                                    {
                                        grid.resize(row_idx + 1);
                                        cell_occupied.resize(row_idx + 1);
                                    }

                                    for (int c = 0; c < colspan; c++)
                                    {
                                        int col_idx = current_col + c;
                                        if (col_idx >= grid[row_idx].size())
                                        {
                                            grid[row_idx].resize(col_idx + 1);
                                            cell_occupied[row_idx].resize(col_idx + 1, false);
                                        }

                                        // For cells with rowspan, only store content in the last row
                                        // Use empty cells for earlier rows in the span
                                        if (rowspan > 1 && r < rowspan - 1)
                                        {
                                            // Empty cell for non-last rows in a rowspan
                                            grid[row_idx][col_idx] = "<td>&#160;</td>";
                                        }
                                        else
                                        {
                                            // Normal cell or last row of rowspan gets the content
                                            grid[row_idx][col_idx] = cleaned_cell;
                                        }
                                        cell_occupied[row_idx][col_idx] = true;
                                    }
                                }

                                current_col += colspan;
                                cell_pos = cell_end + cell_tag.length() + 3;
                            }

                            current_row++;
                            row_pos = row_end + 5;
                        }

                        // Now reconstruct the table from the grid
                        std::ostringstream new_table;
                        new_table << "<table class=\"table\">";
                        new_table << "<tbody>";

                        for (size_t r = 0; r < grid.size(); r++)
                        {
                            new_table << "<tr>";
                            for (size_t c = 0; c < grid[r].size(); c++)
                            {
                                if (!grid[r][c].empty())
                                {
                                    new_table << grid[r][c];
                                }
                            }
                            new_table << "</tr>";
                        }

                        new_table << "</tbody>";
                        new_table << "</table>";

                        // Reconstruct the full HTML
                        clean_html = before_table + new_table.str() + after_table;
                        std::cerr << "Flattened table structure (removed rowspan/colspan)" << std::endl;
                    }
                }

                std::cerr << "After simplification, HTML length: " << clean_html.length() << std::endl;
                std::cerr << "Extracted HTML first 200 chars: " << clean_html.substr(0, std::min<size_t>(200, clean_html.length())) << std::endl;
                std::cerr << "Extracted HTML last 200 chars: ";
                if (clean_html.length() > 200)
                {
                    size_t start = clean_html.length() > 200 ? clean_html.length() - 200 : 0;
                    std::cerr << clean_html.substr(start) << std::endl;
                }
                else
                {
                    std::cerr << "(too short)" << std::endl;
                }
                std::cerr << "Extracted HTML ends with </html>: " << (clean_html.length() >= 7 && clean_html.substr(clean_html.length()-7) == "</html>") << std::endl;

                // Write to debug file
                std::ofstream debug_file("/tmp/xeus_sas_extracted_html_debug.html");
                if (debug_file)
                {
                    debug_file << clean_html;
                    debug_file.close();
                    std::cerr << "DEBUG: Wrote extracted HTML to /tmp/xeus_sas_extracted_html_debug.html" << std::endl;
                }
            }
            else
            {
                std::cerr << "WARNING: Incomplete HTML detected" << std::endl;
                std::cerr << "  HTML start position: " << html_start << std::endl;
                std::cerr << "  HTML end position: " << html_end << std::endl;
                std::cerr << "  Total output length: " << html_output.length() << std::endl;

                // Log first and last 200 chars for debugging
                std::cerr << "  First 200 chars: " << html_output.substr(0, 200) << std::endl;
                if (html_output.length() > 200)
                {
                    size_t start = html_output.length() > 200 ? html_output.length() - 200 : 0;
                    std::cerr << "  Last 200 chars: " << html_output.substr(start) << std::endl;
                }
            }
            std::cerr << "==========================================\n" << std::endl;
        }

        // Create result with both HTML and log
        execution_result result;
        result.log = log_output;
        result.html_output = clean_html;  // Use extracted clean HTML
        result.has_html = has_html;

        // Check for errors in log
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

        // Extract graph files from log
        result.graph_files = extract_graph_files(result.log);

        return result;
#else
        // Fallback to batch mode on Windows
        std::string output = run_sas_batch(code);
        return parse_execution_output(output);
#endif
    }

    std::string sas_session::impl::run_sas_batch(const std::string& code)
    {
        // Create temporary file for code
        std::string temp_sas = "/tmp/xeus_sas_temp.sas";
        std::string temp_log = "/tmp/xeus_sas_temp.log";
        std::string temp_lst = "/tmp/xeus_sas_temp.lst";

        // Write code to file
        std::ofstream ofs(temp_sas);
        if (!ofs)
        {
            throw std::runtime_error("Failed to create temporary SAS file");
        }
        ofs << code << std::endl;
        ofs.close();

        // Build SAS command
        // -nodms: no display manager
        // -noterminal: batch mode
        // -sysin: input file
        // -log: log file
        // -print: listing file
        std::stringstream cmd;
        cmd << m_sas_path
            << " -nodms -noterminal"
            << " -sysin " << temp_sas
            << " -log " << temp_log
            << " -print " << temp_lst
            << " 2>&1";

        // Execute SAS
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen(cmd.str().c_str(), "r"),
            pclose
        );

        if (!pipe)
        {
            throw std::runtime_error("Failed to execute SAS");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }

        // Read log file
        std::string log_content;
        std::ifstream log_ifs(temp_log);
        if (log_ifs)
        {
            std::stringstream log_ss;
            log_ss << log_ifs.rdbuf();
            log_content = log_ss.str();
        }

        // Read listing file
        std::string lst_content;
        std::ifstream lst_ifs(temp_lst);
        if (lst_ifs)
        {
            std::stringstream lst_ss;
            lst_ss << lst_ifs.rdbuf();
            lst_content = lst_ss.str();
        }

        // Combine log and listing
        std::stringstream combined;
        combined << "=== LOG ===" << std::endl;
        combined << log_content << std::endl;
        combined << "=== LISTING ===" << std::endl;
        combined << lst_content << std::endl;

        // Clean up temporary files
        std::remove(temp_sas.c_str());
        std::remove(temp_log.c_str());
        std::remove(temp_lst.c_str());

        return combined.str();
    }

    std::string sas_session::impl::get_version()
    {
        // Execute a simple SAS program to get version
        std::string code = "%put &SYSVER;";
        auto result = execute(code);

        // Parse version from log
        // This is a placeholder - actual version extraction would be more sophisticated
        return "9.4";
    }

    bool sas_session::impl::is_ready() const
    {
        // For batch mode, always ready if SAS path is valid
        return !m_sas_path.empty();
    }

    void sas_session::impl::shutdown()
    {
        if (!m_initialized)
            return;

        std::cout << "Shutting down SAS session..." << std::endl;

#ifndef _WIN32
        // Send ENDSAS command to gracefully terminate SAS
        if (m_sas_stdin)
        {
            fprintf(m_sas_stdin, "endsas;\n");
            fflush(m_sas_stdin);
            fclose(m_sas_stdin);
            m_sas_stdin = nullptr;
        }

        // Close stdout pipe
        if (m_sas_stdout)
        {
            fclose(m_sas_stdout);
            m_sas_stdout = nullptr;
        }

        // Close stderr pipe
        if (m_sas_stderr)
        {
            fclose(m_sas_stderr);
            m_sas_stderr = nullptr;
        }

        // Wait for SAS process to terminate
        if (m_sas_pid > 0)
        {
            int status;
            waitpid(m_sas_pid, &status, 0);
            std::cout << "SAS process terminated (PID: " << m_sas_pid << ")" << std::endl;
            m_sas_pid = -1;
        }
#endif

        m_initialized = false;
    }

    void sas_session::impl::interrupt()
    {
        if (!m_initialized || m_sas_pid <= 0)
            return;

        std::cout << "Interrupting SAS session..." << std::endl;

#ifndef _WIN32
        // Send SIGINT to SAS process
        if (kill(m_sas_pid, SIGINT) == 0)
        {
            std::cout << "Interrupt signal sent to SAS (PID: " << m_sas_pid << ")" << std::endl;
        }
        else
        {
            std::cerr << "Failed to send interrupt signal to SAS" << std::endl;
        }
#endif
    }

    void sas_session::impl::restart()
    {
        std::cerr << "=== RESTARTING SAS SESSION ===" << std::endl;

        // Shutdown the current session
        shutdown();

        // Wait a moment for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Reinitialize the session
        initialize_session();

        std::cerr << "=== SAS SESSION RESTARTED ===" << std::endl;
        std::cerr << "WARNING: Session state lost (datasets, macro variables cleared)" << std::endl;
    }

    std::string sas_session::impl::get_macro(const std::string& name)
    {
        // Execute %PUT to get macro value
        std::string code = "%put &" + name + ";";
        auto result = execute(code);

        // Parse macro value from log
        // This is a placeholder
        return "";
    }

    void sas_session::impl::set_macro(const std::string& name, const std::string& value)
    {
        // Execute %LET to set macro
        std::string code = "%let " + name + " = " + value + ";";
        execute(code);
    }

    // Public API implementation

    sas_session::sas_session(const std::string& sas_path)
        : m_impl(std::make_unique<impl>(sas_path))
    {
    }

    sas_session::~sas_session()
    {
        // Cleanup handled by unique_ptr
    }

    execution_result sas_session::execute(const std::string& code)
    {
        return m_impl->execute(code);
    }

    std::string sas_session::get_version()
    {
        return m_impl->get_version();
    }

    bool sas_session::is_ready() const
    {
        return m_impl->is_ready();
    }

    void sas_session::shutdown()
    {
        m_impl->shutdown();
    }

    void sas_session::interrupt()
    {
        m_impl->interrupt();
    }

    void sas_session::restart()
    {
        m_impl->restart();
    }

    std::string sas_session::get_macro(const std::string& name)
    {
        return m_impl->get_macro(name);
    }

    void sas_session::set_macro(const std::string& name, const std::string& value)
    {
        m_impl->set_macro(name, value);
    }

} // namespace xeus_sas
