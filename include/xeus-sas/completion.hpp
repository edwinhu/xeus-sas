#ifndef XEUS_SAS_COMPLETION_HPP
#define XEUS_SAS_COMPLETION_HPP

#include <string>
#include <vector>

namespace xeus_sas
{
    class sas_session;

    /**
     * @brief Provides intelligent code completion for SAS
     *
     * Completion types:
     * - SAS procedures (PROC MEANS, PROC REG, etc.)
     * - DATA step keywords (SET, MERGE, BY, etc.)
     * - Global statements (LIBNAME, FILENAME, OPTIONS, etc.)
     * - Macro language elements (%LET, %IF, %DO, etc.)
     * - SAS functions (MEAN, SUM, SUBSTR, etc.)
     * - Variable names (from active datasets)
     * - Dataset names (from libraries)
     */
    class completion_engine
    {
    public:
        /**
         * @brief Construct completion engine
         * @param session Pointer to SAS session for dynamic completions
         */
        explicit completion_engine(sas_session* session);

        /**
         * @brief Get completions for code at cursor position
         *
         * @param code Full code string
         * @param cursor_pos Cursor position in code
         * @param start_pos Output: start position of completion range
         * @return Vector of completion suggestions
         */
        std::vector<std::string> get_completions(
            const std::string& code,
            int cursor_pos,
            int& start_pos
        );

    private:
        sas_session* m_session;

        /**
         * @brief Get SAS procedure completions
         *
         * Matches against PROC keywords:
         * MEANS, FREQ, REG, LOGISTIC, SQL, PRINT, etc.
         *
         * @param prefix Partial procedure name
         * @return Vector of matching procedure names
         */
        std::vector<std::string> get_procedure_completions(const std::string& prefix);

        /**
         * @brief Get DATA step keyword completions
         *
         * Keywords: SET, MERGE, BY, IF, THEN, DO, etc.
         *
         * @param prefix Partial keyword
         * @return Vector of matching keywords
         */
        std::vector<std::string> get_data_step_completions(const std::string& prefix);

        /**
         * @brief Get global statement completions
         *
         * Statements: LIBNAME, FILENAME, OPTIONS, TITLE, etc.
         *
         * @param prefix Partial statement name
         * @return Vector of matching statements
         */
        std::vector<std::string> get_global_statement_completions(const std::string& prefix);

        /**
         * @brief Get macro language completions
         *
         * Elements: %LET, %PUT, %IF, %DO, %MACRO, %MEND, etc.
         *
         * @param prefix Partial macro keyword
         * @return Vector of matching macro keywords
         */
        std::vector<std::string> get_macro_completions(const std::string& prefix);

        /**
         * @brief Get SAS function completions
         *
         * Functions: MEAN, SUM, SUBSTR, SCAN, INPUT, PUT, etc.
         *
         * @param prefix Partial function name
         * @return Vector of matching function names
         */
        std::vector<std::string> get_function_completions(const std::string& prefix);

        /**
         * @brief Get variable name completions from active datasets
         *
         * Uses PROC CONTENTS to fetch variable names.
         *
         * @param prefix Partial variable name
         * @return Vector of matching variable names
         */
        std::vector<std::string> get_variable_completions(const std::string& prefix);

        /**
         * @brief Get dataset name completions from libraries
         *
         * @param prefix Partial dataset name
         * @return Vector of matching dataset names
         */
        std::vector<std::string> get_dataset_completions(const std::string& prefix);

        /**
         * @brief Extract the token at cursor position
         *
         * Identifies the partial word being typed for completion.
         *
         * @param code Full code string
         * @param cursor_pos Cursor position
         * @param start_pos Output: start position of token
         * @return Extracted token
         */
        std::string extract_token(
            const std::string& code,
            int cursor_pos,
            int& start_pos
        );

        /**
         * @brief Determine completion context
         *
         * Analyzes code to determine what type of completion to provide:
         * - After PROC: procedure names
         * - After DATA: dataset names
         * - Inside DATA step: variables, keywords
         * - After %: macro keywords
         *
         * @param code Code before cursor
         * @return Context identifier
         */
        std::string determine_context(const std::string& code);
    };

} // namespace xeus_sas

#endif // XEUS_SAS_COMPLETION_HPP
