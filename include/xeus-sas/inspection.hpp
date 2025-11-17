#ifndef XEUS_SAS_INSPECTION_HPP
#define XEUS_SAS_INSPECTION_HPP

#include <string>

namespace xeus_sas
{
    class sas_session;

    /**
     * @brief Provides inline help and code inspection for SAS
     *
     * Inspection capabilities:
     * - Procedure syntax and documentation
     * - Function signatures and descriptions
     * - Dataset information (PROC CONTENTS)
     * - Macro variable values
     * - Macro definitions
     */
    class inspection_engine
    {
    public:
        /**
         * @brief Construct inspection engine
         * @param session Pointer to SAS session for dynamic inspection
         */
        explicit inspection_engine(sas_session* session);

        /**
         * @brief Get inspection info for code at cursor position
         *
         * @param code Full code string
         * @param cursor_pos Cursor position
         * @param detail_level 0 = brief, 1 = full documentation
         * @return Formatted inspection text (markdown)
         */
        std::string get_inspection(
            const std::string& code,
            int cursor_pos,
            int detail_level
        );

    private:
        sas_session* m_session;

        /**
         * @brief Get help for a SAS procedure
         *
         * Returns syntax and description:
         * ```
         * PROC MEANS <options>;
         *   VAR variables;
         *   CLASS class-variables;
         *   OUTPUT OUT=dataset <statistics>;
         * RUN;
         * ```
         *
         * @param procedure Procedure name (e.g., "MEANS")
         * @param detail_level 0 = syntax only, 1 = full help
         * @return Markdown-formatted help text
         */
        std::string get_procedure_help(
            const std::string& procedure,
            int detail_level
        );

        /**
         * @brief Get help for a SAS function
         *
         * Returns function signature and description:
         * ```
         * SUBSTR(string, position, <length>)
         * Extracts a substring from a character string.
         * ```
         *
         * @param function Function name
         * @param detail_level 0 = signature only, 1 = full help
         * @return Markdown-formatted help text
         */
        std::string get_function_help(
            const std::string& function,
            int detail_level
        );

        /**
         * @brief Get information about a dataset
         *
         * Runs PROC CONTENTS and returns:
         * - Number of observations
         * - Number of variables
         * - Variable list with types
         *
         * @param dataset Dataset name (e.g., "WORK.MYDATA")
         * @return Formatted dataset info
         */
        std::string get_dataset_info(const std::string& dataset);

        /**
         * @brief Get information about a variable
         *
         * Returns:
         * - Variable type (numeric/character)
         * - Length
         * - Label (if present)
         * - Format (if present)
         *
         * @param variable Variable name
         * @return Formatted variable info
         */
        std::string get_variable_info(const std::string& variable);

        /**
         * @brief Get value of a macro variable
         *
         * @param macro_var Macro variable name (without %)
         * @return Macro variable value
         */
        std::string get_macro_value(const std::string& macro_var);

        /**
         * @brief Get definition of a macro
         *
         * Shows the macro source code.
         *
         * @param macro_name Macro name (without %)
         * @return Macro definition
         */
        std::string get_macro_definition(const std::string& macro_name);

        /**
         * @brief Extract the identifier at cursor position
         *
         * Identifies the word under cursor for inspection.
         *
         * @param code Full code string
         * @param cursor_pos Cursor position
         * @return Extracted identifier
         */
        std::string extract_identifier(
            const std::string& code,
            int cursor_pos
        );

        /**
         * @brief Determine what type of identifier this is
         *
         * Categories:
         * - procedure (after PROC)
         * - function (followed by parenthesis)
         * - dataset (in DATA or SET statement)
         * - variable
         * - macro
         *
         * @param code Code context
         * @param identifier Identifier to classify
         * @return Type string
         */
        std::string classify_identifier(
            const std::string& code,
            const std::string& identifier
        );
    };

} // namespace xeus_sas

#endif // XEUS_SAS_INSPECTION_HPP
