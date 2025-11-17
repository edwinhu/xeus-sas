#include "xeus-sas/inspection.hpp"
#include "xeus-sas/sas_session.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace xeus_sas
{
    inspection_engine::inspection_engine(sas_session* session)
        : m_session(session)
    {
    }

    std::string inspection_engine::get_inspection(
        const std::string& code,
        int cursor_pos,
        int detail_level
    )
    {
        // Extract identifier at cursor
        std::string identifier = extract_identifier(code, cursor_pos);

        if (identifier.empty())
        {
            return "";
        }

        // Classify identifier
        std::string type = classify_identifier(code, identifier);

        // Get appropriate help based on type
        if (type == "procedure")
        {
            return get_procedure_help(identifier, detail_level);
        }
        else if (type == "function")
        {
            return get_function_help(identifier, detail_level);
        }
        else if (type == "dataset")
        {
            return get_dataset_info(identifier);
        }
        else if (type == "macro")
        {
            return get_macro_value(identifier);
        }

        return "";
    }

    std::string inspection_engine::get_procedure_help(
        const std::string& procedure,
        int detail_level
    )
    {
        std::string upper_proc = procedure;
        std::transform(upper_proc.begin(), upper_proc.end(), upper_proc.begin(), ::toupper);

        std::stringstream help;

        // Basic syntax for common procedures
        if (upper_proc == "MEANS")
        {
            help << "# PROC MEANS\n\n";
            help << "Computes descriptive statistics for numeric variables.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC MEANS <options>;\n";
            help << "  VAR variables;\n";
            help << "  CLASS class-variables;\n";
            help << "  OUTPUT OUT=dataset <statistics>;\n";
            help << "RUN;\n";
            help << "```\n";

            if (detail_level > 0)
            {
                help << "\n**Common Options:**\n";
                help << "- DATA= : Input dataset\n";
                help << "- N : Number of observations\n";
                help << "- MEAN : Arithmetic mean\n";
                help << "- STD : Standard deviation\n";
                help << "- MIN, MAX : Minimum and maximum values\n";
            }
        }
        else if (upper_proc == "FREQ")
        {
            help << "# PROC FREQ\n\n";
            help << "Produces frequency and crosstabulation tables.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC FREQ <options>;\n";
            help << "  TABLES variables / options;\n";
            help << "RUN;\n";
            help << "```\n";

            if (detail_level > 0)
            {
                help << "\n**Common Options:**\n";
                help << "- TABLES var1*var2 : Crosstabulation\n";
                help << "- / CHISQ : Chi-square test\n";
                help << "- / NOCUM : Suppress cumulative statistics\n";
            }
        }
        else if (upper_proc == "PRINT")
        {
            help << "# PROC PRINT\n\n";
            help << "Prints observations from a dataset.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC PRINT DATA=dataset <options>;\n";
            help << "  VAR variables;\n";
            help << "  ID id-variables;\n";
            help << "RUN;\n";
            help << "```\n";
        }
        else if (upper_proc == "SQL")
        {
            help << "# PROC SQL\n\n";
            help << "Implements ANSI SQL for data queries and manipulation.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC SQL;\n";
            help << "  SELECT columns\n";
            help << "  FROM table\n";
            help << "  WHERE condition;\n";
            help << "QUIT;\n";
            help << "```\n";
        }
        else if (upper_proc == "SORT")
        {
            help << "# PROC SORT\n\n";
            help << "Sorts observations in a dataset.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC SORT DATA=input OUT=output;\n";
            help << "  BY <DESCENDING> variables;\n";
            help << "RUN;\n";
            help << "```\n";
        }
        else if (upper_proc == "REG")
        {
            help << "# PROC REG\n\n";
            help << "Performs linear regression analysis.\n\n";
            help << "**Syntax:**\n```sas\n";
            help << "PROC REG DATA=dataset;\n";
            help << "  MODEL dependent = independents / options;\n";
            help << "RUN;\n";
            help << "```\n";
        }
        else
        {
            help << "# PROC " << upper_proc << "\n\n";
            help << "No detailed help available for this procedure.\n";
            help << "See SAS documentation for more information.\n";
        }

        return help.str();
    }

    std::string inspection_engine::get_function_help(
        const std::string& function,
        int detail_level
    )
    {
        std::string upper_func = function;
        std::transform(upper_func.begin(), upper_func.end(), upper_func.begin(), ::toupper);

        std::stringstream help;

        // Help for common functions
        if (upper_func == "SUBSTR")
        {
            help << "# SUBSTR Function\n\n";
            help << "**Syntax:** `SUBSTR(string, position, <length>)`\n\n";
            help << "Extracts a substring from a character string.\n";

            if (detail_level > 0)
            {
                help << "\n**Arguments:**\n";
                help << "- string: Character variable or constant\n";
                help << "- position: Starting position (1-based)\n";
                help << "- length: Optional length of substring\n";
            }
        }
        else if (upper_func == "MEAN")
        {
            help << "# MEAN Function\n\n";
            help << "**Syntax:** `MEAN(variable1, variable2, ...)`\n\n";
            help << "Computes the arithmetic mean of non-missing values.\n";
        }
        else if (upper_func == "SUM")
        {
            help << "# SUM Function\n\n";
            help << "**Syntax:** `SUM(variable1, variable2, ...)`\n\n";
            help << "Computes the sum of non-missing values.\n";
        }
        else if (upper_func == "INPUT")
        {
            help << "# INPUT Function\n\n";
            help << "**Syntax:** `INPUT(source, informat)`\n\n";
            help << "Converts character data to numeric or converts one type to another.\n";
        }
        else if (upper_func == "PUT")
        {
            help << "# PUT Function\n\n";
            help << "**Syntax:** `PUT(source, format)`\n\n";
            help << "Converts numeric or character data to character using a format.\n";
        }
        else
        {
            help << "# " << upper_func << " Function\n\n";
            help << "No detailed help available for this function.\n";
        }

        return help.str();
    }

    std::string inspection_engine::get_dataset_info(const std::string& dataset)
    {
        // TODO: Run PROC CONTENTS to get dataset info
        // For Phase 1, return placeholder
        std::stringstream info;
        info << "# Dataset: " << dataset << "\n\n";
        info << "Use PROC CONTENTS to view dataset details:\n";
        info << "```sas\nPROC CONTENTS DATA=" << dataset << ";\nRUN;\n```\n";
        return info.str();
    }

    std::string inspection_engine::get_variable_info(const std::string& variable)
    {
        // TODO: Query variable metadata
        return "Variable information not available in Phase 1.";
    }

    std::string inspection_engine::get_macro_value(const std::string& macro_var)
    {
        // Get macro variable value
        if (m_session)
        {
            std::string value = m_session->get_macro(macro_var);
            if (!value.empty())
            {
                std::stringstream info;
                info << "# Macro Variable: " << macro_var << "\n\n";
                info << "Value: `" << value << "`\n";
                return info.str();
            }
        }

        return "Macro variable information not available.";
    }

    std::string inspection_engine::get_macro_definition(const std::string& macro_name)
    {
        // TODO: Retrieve macro definition from session
        return "Macro definitions not available in Phase 1.";
    }

    std::string inspection_engine::extract_identifier(
        const std::string& code,
        int cursor_pos
    )
    {
        if (cursor_pos > static_cast<int>(code.length()))
        {
            cursor_pos = code.length();
        }

        // Find start of identifier
        int start = cursor_pos;
        while (start > 0 && (std::isalnum(code[start - 1]) || code[start - 1] == '_'))
        {
            --start;
        }

        // Find end of identifier
        int end = cursor_pos;
        while (end < static_cast<int>(code.length()) &&
               (std::isalnum(code[end]) || code[end] == '_'))
        {
            ++end;
        }

        return code.substr(start, end - start);
    }

    std::string inspection_engine::classify_identifier(
        const std::string& code,
        const std::string& identifier
    )
    {
        std::string upper_code = code;
        std::transform(upper_code.begin(), upper_code.end(), upper_code.begin(), ::toupper);

        std::string upper_id = identifier;
        std::transform(upper_id.begin(), upper_id.end(), upper_id.begin(), ::toupper);

        // Check if it's after PROC
        if (upper_code.find("PROC " + upper_id) != std::string::npos)
        {
            return "procedure";
        }

        // Check if it's followed by parenthesis (function)
        size_t id_pos = code.find(identifier);
        if (id_pos != std::string::npos)
        {
            size_t next_char = id_pos + identifier.length();
            while (next_char < code.length() && std::isspace(code[next_char]))
            {
                ++next_char;
            }
            if (next_char < code.length() && code[next_char] == '(')
            {
                return "function";
            }
        }

        // Check if it's a dataset reference
        if (upper_code.find("DATA=" + upper_id) != std::string::npos ||
            upper_code.find("DATA " + upper_id) != std::string::npos ||
            upper_code.find("SET " + upper_id) != std::string::npos)
        {
            return "dataset";
        }

        // Check if it's a macro variable
        if (!identifier.empty() && identifier[0] == '%')
        {
            return "macro";
        }

        return "unknown";
    }

} // namespace xeus_sas
