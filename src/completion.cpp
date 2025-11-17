#include "xeus-sas/completion.hpp"
#include "xeus-sas/sas_session.hpp"

#include <algorithm>
#include <cctype>

namespace xeus_sas
{
    completion_engine::completion_engine(sas_session* session)
        : m_session(session)
    {
    }

    std::vector<std::string> completion_engine::get_completions(
        const std::string& code,
        int cursor_pos,
        int& start_pos
    )
    {
        // Extract token at cursor
        std::string token = extract_token(code, cursor_pos, start_pos);

        if (token.empty())
        {
            return {};
        }

        // Determine context
        std::string context = determine_context(code.substr(0, cursor_pos));

        // Get appropriate completions based on context
        std::vector<std::string> completions;

        if (context == "proc")
        {
            completions = get_procedure_completions(token);
        }
        else if (context == "macro")
        {
            completions = get_macro_completions(token);
        }
        else if (context == "data_step")
        {
            auto data_completions = get_data_step_completions(token);
            auto var_completions = get_variable_completions(token);
            completions.insert(completions.end(), data_completions.begin(), data_completions.end());
            completions.insert(completions.end(), var_completions.begin(), var_completions.end());
        }
        else if (context == "function")
        {
            completions = get_function_completions(token);
        }
        else
        {
            // General context: try all
            auto proc_comp = get_procedure_completions(token);
            auto data_comp = get_data_step_completions(token);
            auto global_comp = get_global_statement_completions(token);
            auto macro_comp = get_macro_completions(token);

            completions.insert(completions.end(), proc_comp.begin(), proc_comp.end());
            completions.insert(completions.end(), data_comp.begin(), data_comp.end());
            completions.insert(completions.end(), global_comp.begin(), global_comp.end());
            completions.insert(completions.end(), macro_comp.begin(), macro_comp.end());
        }

        // Remove duplicates and sort
        std::sort(completions.begin(), completions.end());
        completions.erase(std::unique(completions.begin(), completions.end()), completions.end());

        return completions;
    }

    std::vector<std::string> completion_engine::get_procedure_completions(const std::string& prefix)
    {
        // Common SAS procedures
        static const std::vector<std::string> procedures = {
            "APPEND", "CALIS", "CANDISC", "CDISC", "COMPARE", "CONTENTS", "COPY",
            "CORR", "DATASETS", "DISPLAY", "EXPORT", "FCMP", "FORMAT", "FREQ",
            "GENMOD", "GLM", "GLMMOD", "GPLOT", "GREPLAY", "IMPORT", "IML",
            "LIFETEST", "LOGISTIC", "MEANS", "MIXED", "NLIN", "NLMIXED", "OPTEX",
            "PLOT", "POWER", "PRINT", "PRINCOMP", "PRINQUAL", "PROBIT", "RANK",
            "REG", "REPORT", "RSREG", "SCORE", "SGP ANEL", "SGPLOT", "SGRENDER",
            "SGSCATTER", "SORT", "SQL", "STANDARD", "STDIZE", "STEPDISC", "SUMMARY",
            "TABULATE", "TEMPLATE", "TIMEPLOT", "TIMESERIES", "TPSPLINE", "TRANSPOSE",
            "TTEST", "UNIVARIATE", "VARCLUS", "VARCOMP", "VARIOGRAM"
        };

        std::vector<std::string> matches;
        std::string upper_prefix = prefix;
        std::transform(upper_prefix.begin(), upper_prefix.end(), upper_prefix.begin(), ::toupper);

        for (const auto& proc : procedures)
        {
            if (proc.find(upper_prefix) == 0)
            {
                matches.push_back(proc);
            }
        }

        return matches;
    }

    std::vector<std::string> completion_engine::get_data_step_completions(const std::string& prefix)
    {
        // DATA step keywords
        static const std::vector<std::string> keywords = {
            "ABORT", "ARRAY", "ATTRIB", "BY", "CALL", "CARDS", "DATALINES",
            "DELETE", "DO", "DROP", "ELSE", "END", "ERROR", "FILE", "FORMAT",
            "GO", "GOTO", "IF", "INFILE", "INFORMAT", "INPUT", "KEEP", "LABEL",
            "LEAVE", "LENGTH", "LINK", "LIST", "MERGE", "MODIFY", "OUTPUT",
            "PUT", "PUTLOG", "REDIRECT", "REMOVE", "RENAME", "REPLACE", "RETAIN",
            "RETURN", "SELECT", "SET", "STOP", "SUM", "THEN", "UNTIL", "UPDATE",
            "WHEN", "WHERE", "WHILE", "WINDOW"
        };

        std::vector<std::string> matches;
        std::string upper_prefix = prefix;
        std::transform(upper_prefix.begin(), upper_prefix.end(), upper_prefix.begin(), ::toupper);

        for (const auto& keyword : keywords)
        {
            if (keyword.find(upper_prefix) == 0)
            {
                matches.push_back(keyword);
            }
        }

        return matches;
    }

    std::vector<std::string> completion_engine::get_global_statement_completions(const std::string& prefix)
    {
        // Global statements
        static const std::vector<std::string> statements = {
            "CATNAME", "FILENAME", "FOOTNOTE", "LIBNAME", "LOCK", "MISSING",
            "OPTIONS", "PAGE", "RESETLINE", "SKIP", "TITLE", "X"
        };

        std::vector<std::string> matches;
        std::string upper_prefix = prefix;
        std::transform(upper_prefix.begin(), upper_prefix.end(), upper_prefix.begin(), ::toupper);

        for (const auto& stmt : statements)
        {
            if (stmt.find(upper_prefix) == 0)
            {
                matches.push_back(stmt);
            }
        }

        return matches;
    }

    std::vector<std::string> completion_engine::get_macro_completions(const std::string& prefix)
    {
        // Macro language keywords
        static const std::vector<std::string> macros = {
            "%ABORT", "%BY", "%COPY", "%DISPLAY", "%DO", "%ELSE", "%END",
            "%EVAL", "%GLOBAL", "%GOTO", "%IF", "%INCLUDE", "%INPUT", "%LET",
            "%LIST", "%LOCAL", "%MACRO", "%MEND", "%PUT", "%RETURN", "%RUN",
            "%SYMDEL", "%SYSCALL", "%SYSEVALF", "%SYSEXEC", "%SYSLPUT", "%SYSMACDELETE",
            "%SYSRPUT", "%THEN", "%UNTIL", "%WHILE", "%WINDOW"
        };

        std::vector<std::string> matches;
        std::string upper_prefix = prefix;
        std::transform(upper_prefix.begin(), upper_prefix.end(), upper_prefix.begin(), ::toupper);

        // Handle % prefix
        if (!upper_prefix.empty() && upper_prefix[0] == '%')
        {
            for (const auto& macro : macros)
            {
                if (macro.find(upper_prefix) == 0)
                {
                    matches.push_back(macro);
                }
            }
        }
        else if (!upper_prefix.empty())
        {
            // User typed without %, match the part after %
            for (const auto& macro : macros)
            {
                if (macro.substr(1).find(upper_prefix) == 0)
                {
                    matches.push_back(macro);
                }
            }
        }

        return matches;
    }

    std::vector<std::string> completion_engine::get_function_completions(const std::string& prefix)
    {
        // Common SAS functions
        static const std::vector<std::string> functions = {
            "ABS", "CEIL", "FLOOR", "INT", "LOG", "LOG10", "MAX", "MEAN", "MIN",
            "MOD", "ROUND", "SQRT", "SUM", "COMPRESS", "INDEX", "LEFT", "LENGTH",
            "LOWCASE", "REVERSE", "RIGHT", "SCAN", "STRIP", "SUBSTR", "TRIM",
            "UPCASE", "CAT", "CATS", "CATT", "CATX", "INPUT", "PUT", "INPUTC",
            "INPUTN", "PUTC", "PUTN", "DATE", "DATETIME", "DAY", "MONTH", "YEAR",
            "TODAY", "TIME", "INTCK", "INTNX", "DATEPART", "TIMEPART"
        };

        std::vector<std::string> matches;
        std::string upper_prefix = prefix;
        std::transform(upper_prefix.begin(), upper_prefix.end(), upper_prefix.begin(), ::toupper);

        for (const auto& func : functions)
        {
            if (func.find(upper_prefix) == 0)
            {
                matches.push_back(func);
            }
        }

        return matches;
    }

    std::vector<std::string> completion_engine::get_variable_completions(const std::string& prefix)
    {
        // TODO: Implement by querying active datasets using PROC CONTENTS
        // For Phase 1, return empty
        return {};
    }

    std::vector<std::string> completion_engine::get_dataset_completions(const std::string& prefix)
    {
        // TODO: Implement by querying SASHELP.VTABLE or dictionary tables
        // For Phase 1, return empty
        return {};
    }

    std::string completion_engine::extract_token(
        const std::string& code,
        int cursor_pos,
        int& start_pos
    )
    {
        if (cursor_pos > static_cast<int>(code.length()))
        {
            cursor_pos = code.length();
        }

        // Find start of token (move backwards from cursor)
        start_pos = cursor_pos;
        while (start_pos > 0 && (std::isalnum(code[start_pos - 1]) ||
                                 code[start_pos - 1] == '_' ||
                                 code[start_pos - 1] == '%'))
        {
            --start_pos;
        }

        // Extract token
        std::string token = code.substr(start_pos, cursor_pos - start_pos);
        return token;
    }

    std::string completion_engine::determine_context(const std::string& code)
    {
        // Simple context determination
        std::string upper_code = code;
        std::transform(upper_code.begin(), upper_code.end(), upper_code.begin(), ::toupper);

        // Check if we're after PROC
        if (upper_code.find("PROC ") != std::string::npos)
        {
            size_t proc_pos = upper_code.rfind("PROC ");
            size_t semi_pos = upper_code.rfind(";");
            if (semi_pos == std::string::npos || semi_pos < proc_pos)
            {
                return "proc";
            }
        }

        // Check if we're in macro context
        if (!code.empty() && code.back() == '%')
        {
            return "macro";
        }

        // Check if we're in DATA step
        if (upper_code.find("DATA ") != std::string::npos)
        {
            size_t data_pos = upper_code.rfind("DATA ");
            size_t run_pos = upper_code.rfind("RUN;");
            if (run_pos == std::string::npos || run_pos < data_pos)
            {
                return "data_step";
            }
        }

        return "general";
    }

} // namespace xeus_sas
