#pragma once
#include <string>
namespace bentoclient
{
    namespace logging
    {
        /// @brief Initializes logging
        /// @param  logThreadId Enables thread IDs prepended to log lines
        /// @param logLevel Sets the log level as a string (trace|debug|info|warning|error|fatal)
        void init_logging(bool logThreadId, const std::string& logLevel);
        /// @brief Indicator for enabled trace logging
        /// @return 
        bool trace_logging_enabled();
        /// @brief Returns known log levels == (trace|debug|info|warning|error|fatal)
        std::string get_log_levels();
    }

}