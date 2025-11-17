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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
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

    execution_result sas_session::impl::execute(const std::string& code)
    {
        // For Phase 1, use batch mode execution
        // Later phases will implement interactive session
        std::string output = run_sas_batch(code);

        // Parse output
        return parse_execution_output(output);
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
        // For batch mode, nothing to clean up
        // Future: kill interactive session if running
    }

    void sas_session::impl::interrupt()
    {
        // For batch mode, not applicable
        // Future: send SIGINT to SAS process
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
