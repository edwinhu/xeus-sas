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
            // -stdio: use stdin/stdout for I/O
            // -nonews: suppress startup news
            // -nosource: suppress source code echo in log
            execlp(m_sas_path.c_str(), m_sas_path.c_str(),
                   "-nodms", "-stdio", "-nonews", "-nosource", nullptr);

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
        std::stringstream wrapped_code;
        wrapped_code << "ods listing close;\n"
                     << "ods html5 (id=xeus_sas_internal) file=stdout options(bitmap_mode='inline') device=svg style=HTMLBlue;\n"
                     << "ods graphics on / outputfmt=png;\n"
                     << "\n"
                     << code << "\n"
                     << "\n"
                     << "ods html5 (id=xeus_sas_internal) close;\n"
                     << "ods listing;\n";

        // Send wrapped code to SAS
        fprintf(m_sas_stdin, "%s\n", wrapped_code.str().c_str());

        // Send marker to stderr (log stream) to detect end of output
        // Note: We add a DATA _null_; RUN; after the marker to force SAS to flush output
        fprintf(m_sas_stdin, "%%put %s;\n", marker.c_str());
        fprintf(m_sas_stdin, "DATA _null_; run;\n");
        fflush(m_sas_stdin);

        // Read both stdout (HTML) and stderr (log) until we see the marker
        // We need to read both streams to avoid deadlock
        std::string html_output;
        std::string log_output;
        char buffer[4096];
        bool found_marker = false;

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

        while (!found_marker)
        {
            int poll_result = poll(fds, 2, 1000); // 1 second timeout
            if (poll_result < 0)
            {
                break; // Error
            }
            else if (poll_result == 0)
            {
                continue; // Timeout, try again
            }

            // Read from stdout (HTML)
            if (fds[0].revents & POLLIN)
            {
                if (fgets(buffer, sizeof(buffer), m_sas_stdout))
                {
                    html_output += buffer;
                }
            }

            // Read from stderr (log)
            if (fds[1].revents & POLLIN)
            {
                if (fgets(buffer, sizeof(buffer), m_sas_stderr))
                {
                    std::string line(buffer);

                    // Check if this line contains our marker
                    if (line.find(marker) != std::string::npos)
                    {
                        found_marker = true;
                        break;  // Don't add marker line to log
                    }

                    log_output += line;
                }
            }
        }

        // Restore blocking mode
        fcntl(stderr_fd, F_SETFL, stderr_flags);
        fcntl(stdout_fd, F_SETFL, stdout_flags);

        // Create result with both HTML and log
        execution_result result;
        result.log = log_output;
        result.html_output = html_output;

        // Detect if we have HTML output
        result.has_html = (html_output.find("<!DOCTYPE html>") != std::string::npos ||
                          html_output.find("<html") != std::string::npos);

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

    std::string sas_session::get_macro(const std::string& name)
    {
        return m_impl->get_macro(name);
    }

    void sas_session::set_macro(const std::string& name, const std::string& value)
    {
        m_impl->set_macro(name, value);
    }

} // namespace xeus_sas
