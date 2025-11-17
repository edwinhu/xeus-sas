#ifndef XEUS_SAS_SESSION_HPP
#define XEUS_SAS_SESSION_HPP

#include <string>
#include <memory>
#include <vector>

namespace xeus_sas
{
    /**
     * @brief Result of SAS code execution
     *
     * SAS produces two primary output streams:
     * - log: Contains SAS log messages (NOTEs, WARNINGs, ERRORs)
     * - listing: Contains procedure output (results, tables)
     */
    struct execution_result
    {
        std::string log;                      // SAS log output
        std::string listing;                  // LST/ODS output (plain text, deprecated)
        std::string html_output;              // HTML5 output from ODS
        bool has_html;                        // Flag to indicate HTML vs TEXT mode
        bool is_error;                        // Error flag
        int error_code;                       // SAS error code
        std::string error_message;            // Error details
        std::vector<std::string> graph_files; // Generated graphics (PNG/SVG)
    };

    /**
     * @brief Manages SAS process lifecycle and communication
     *
     * This class handles:
     * - Starting/stopping SAS in batch mode
     * - Sending code to SAS via stdin
     * - Receiving output from SAS via stdout/stderr
     * - Managing SAS session state
     */
    class sas_session
    {
    public:
        /**
         * @brief Construct a new SAS session
         * @param sas_path Path to SAS executable (empty = auto-detect)
         */
        explicit sas_session(const std::string& sas_path = "");

        /**
         * @brief Destructor - ensures clean shutdown
         */
        ~sas_session();

        // Delete copy constructor and assignment
        sas_session(const sas_session&) = delete;
        sas_session& operator=(const sas_session&) = delete;

        /**
         * @brief Execute SAS code and return result
         * @param code SAS code to execute
         * @return execution_result containing log, listing, and error info
         */
        execution_result execute(const std::string& code);

        /**
         * @brief Get SAS version string
         * @return SAS version (e.g., "9.4")
         */
        std::string get_version();

        /**
         * @brief Check if session is ready for execution
         * @return true if SAS process is running and responsive
         */
        bool is_ready() const;

        /**
         * @brief Shutdown the SAS session gracefully
         */
        void shutdown();

        /**
         * @brief Interrupt current execution (SIGINT)
         */
        void interrupt();

        /**
         * @brief Get value of a SAS macro variable
         * @param name Macro variable name (without %)
         * @return Macro variable value
         */
        std::string get_macro(const std::string& name);

        /**
         * @brief Set value of a SAS macro variable
         * @param name Macro variable name (without %)
         * @param value Macro variable value
         */
        void set_macro(const std::string& name, const std::string& value);

    private:
        // PIMPL idiom for platform-specific implementation
        class impl;
        std::unique_ptr<impl> m_impl;
    };

} // namespace xeus_sas

#endif // XEUS_SAS_SESSION_HPP
