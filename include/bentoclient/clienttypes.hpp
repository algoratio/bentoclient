#pragma once
#include <chrono>
#include <string>
#include <ostream>

namespace bentoclient
{
    /// @brief time points for market data
    typedef std::chrono::time_point<std::chrono::system_clock,
                            std::chrono::duration<uint64_t, std::nano>>
                            Timestamp;
    /// @brief duration for length of timeseries
    typedef std::chrono::duration<uint64_t, std::micro> TimeRange;
    /// @brief yyyy-mm-dd HH:MM:SS.NanosZ string representation of timestamp
    std::string serializeTimestamp(const Timestamp& timestamp);
}

std::ostream& operator << (std::ostream& ostr, const bentoclient::Timestamp& timestamp);

