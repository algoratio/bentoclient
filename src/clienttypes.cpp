#include "bentoclient/clienttypes.hpp"
#include <iomanip>
#include <sstream>
#include <databento/datetime.hpp>

static_assert(std::is_same_v<databento::UnixNanos, bentoclient::Timestamp>,
    "databento::UnixNanos and bentoclient::Timestamp must be same");

std::string bentoclient::serializeTimestamp(const bentoclient::Timestamp& timestamp) {
    // Convert Timestamp to std::time_t (seconds since epoch)
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(timestamp);
    std::time_t timeT = std::chrono::system_clock::to_time_t(seconds);

    // Format the time as "YYYY-MM-DD HH:MM:SS"
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&timeT), "%Y-%m-%d %H:%M:%S");

    // Add fractional seconds (nanoseconds)
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count() % 1'000'000'000;
    oss << "." << std::setfill('0') << std::setw(9) << nanoseconds << "Z";

    return oss.str();
}

std::ostream& operator << (std::ostream& ostr, const bentoclient::Timestamp& timestamp)
{
    ostr << bentoclient::serializeTimestamp(timestamp);
    return ostr;
}
