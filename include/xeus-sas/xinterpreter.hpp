#ifndef XEUS_SAS_INTERPRETER_HPP
#define XEUS_SAS_INTERPRETER_HPP

#include <memory>
#include <string>
#include <atomic>

#include "xeus/xinterpreter.hpp"
#include "nlohmann/json.hpp"

namespace nl = nlohmann;

// Global flag to indicate interrupt was requested
// Defined in main.cpp, accessed by interpreter
extern std::atomic<bool> g_interrupt_requested;

namespace xeus_sas
{
    class sas_session;
    class completion_engine;
    class inspection_engine;

    /**
     * @brief Main interpreter class for xeus-sas kernel
     *
     * This class implements the Jupyter kernel protocol via xeus framework.
     * It manages:
     * - Code execution (execute_request)
     * - Code completion (complete_request)
     * - Code inspection (inspect_request)
     * - Kernel information (kernel_info_request)
     * - Session shutdown (shutdown_request)
     */
    class interpreter : public xeus::xinterpreter
    {
    public:
        /**
         * @brief Construct the SAS interpreter
         *
         * Initializes:
         * - SAS session
         * - Completion engine
         * - Inspection engine
         */
        interpreter();

        /**
         * @brief Destructor - ensures clean shutdown
         */
        virtual ~interpreter();

        /**
         * @brief Handle interrupt request
         *
         * Called when SIGINT is received. Restarts the SAS session to recover
         * from interrupt (since SAS batch mode doesn't support graceful interrupt).
         *
         * WARNING: This will lose all SAS session state (datasets, macro variables).
         */
        void handle_interrupt();

    private:
        /**
         * @brief Configure the interpreter
         *
         * Called by xeus framework during initialization.
         */
        void configure_impl() override;

        /**
         * @brief Execute SAS code
         *
         * @param cb Callback to send reply
         * @param execution_counter Execution number (for In[n]/Out[n])
         * @param code SAS code to execute
         * @param config Execution configuration (silent, store_history, allow_stdin)
         * @param user_expressions Additional expressions to evaluate
         */
        void execute_request_impl(
            xeus::xinterpreter::send_reply_callback cb,
            int execution_counter,
            const std::string& code,
            xeus::execute_request_config config,
            nl::json user_expressions
        ) override;

        /**
         * @brief Provide code completion suggestions
         *
         * @param code Code to complete
         * @param cursor_pos Cursor position in code
         * @return JSON response with completion matches
         */
        nl::json complete_request_impl(
            const std::string& code,
            int cursor_pos
        ) override;

        /**
         * @brief Provide code inspection/help
         *
         * @param code Code to inspect
         * @param cursor_pos Cursor position in code
         * @param detail_level 0 = brief, 1 = detailed
         * @return JSON response with inspection data
         */
        nl::json inspect_request_impl(
            const std::string& code,
            int cursor_pos,
            int detail_level
        ) override;

        /**
         * @brief Check if code is complete and ready to execute
         *
         * Determines if user needs to continue typing (multi-line input).
         *
         * @param code Code to check
         * @return JSON response with completion status
         */
        nl::json is_complete_request_impl(
            const std::string& code
        ) override;

        /**
         * @brief Provide kernel metadata
         *
         * Returns information about the kernel:
         * - Implementation name/version
         * - Language name/version
         * - Supported features
         *
         * @return JSON response with kernel info
         */
        nl::json kernel_info_request_impl() override;

        /**
         * @brief Handle shutdown request
         *
         * Cleans up resources and terminates SAS session.
         */
        void shutdown_request_impl() override;

    private:
        // Core components
        std::unique_ptr<sas_session> m_session;
        std::unique_ptr<completion_engine> m_completer;
        std::unique_ptr<inspection_engine> m_inspector;

        /**
         * @brief Display graphics in notebook
         *
         * Sends image data to frontend for display.
         *
         * @param graph_files Vector of graph file paths
         */
        void display_graphics(const std::vector<std::string>& graph_files);
    };

} // namespace xeus_sas

#endif // XEUS_SAS_INTERPRETER_HPP
