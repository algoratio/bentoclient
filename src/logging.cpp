#include "bentoclient/logging.hpp"
#include "bentoclient/apputils.hpp"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <fmt/core.h>
#include <atomic>

namespace {
class LoggingInitializer {
public:
    LoggingInitializer() {
        namespace logging = boost::log;
        // Suppress all output by default
        logging::core::get()->set_filter(
            logging::trivial::severity > logging::trivial::fatal
        );
    }
};

// default level no logging. The applicatin enables logging calling init_logging.
std::atomic<std::uint64_t> boost_log_level = boost::log::trivial::fatal;
// Static instance: constructed before main()
static LoggingInitializer loggingInitializerInstance;

std::map<std::string, boost::log::trivial::severity_level> getLevelMap()
{
    namespace logging = boost::log;
    std::map<std::string, logging::trivial::severity_level> levelMap =
    {
        {"trace", logging::trivial::trace},
        {"debug", logging::trivial::debug},
        {"info", logging::trivial::info},
        {"warning", logging::trivial::warning},
        {"error", logging::trivial::error},
        {"none", logging::trivial::fatal}
    };
    return levelMap;
}

}

void bentoclient::logging::init_logging(bool logThreadId, const std::string& logLevel) {
    namespace logging = boost::log;
    namespace expr = boost::log::expressions;
    namespace keywords = boost::log::keywords;

    std::string lowerLevel = bentoclient::AppUtils::toLower(logLevel);

    std::map<std::string, logging::trivial::severity_level> levelMap = getLevelMap();

    auto levelIt = levelMap.find(lowerLevel);
    if (levelIt == levelMap.end())
    {
        throw std::invalid_argument(
            fmt::format("Unknown logging level: {}, options are {}", 
            logLevel,
            get_log_levels()));
    }
    boost_log_level = levelIt->second;


    // Add common attributes like timestamp and thread id
    logging::add_common_attributes();

    boost::log::formatter fmt;
    if (logThreadId) {
        fmt = expr::stream
            << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] "
            << "[" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID") << "] "
            << "<" << logging::trivial::severity << "> "
            << expr::smessage;
    } else {
        fmt = expr::stream
            << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
            << "] "
            << "<" << logging::trivial::severity << "> "
            << expr::smessage;
    }

    auto sink = logging::add_console_log(
        std::clog,
        keywords::format = fmt
    );

    // Optional: set filter for minimum log level
    logging::core::get()->set_filter(
        logging::trivial::severity >= levelIt->second
    );
}

std::string bentoclient::logging::get_log_levels()
{
    return AppUtils::joinKeys(getLevelMap(), "|");
}


bool bentoclient::logging::trace_logging_enabled()
{
    return boost_log_level == boost::log::trivial::trace;
}


