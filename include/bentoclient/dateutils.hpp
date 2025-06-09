#pragma once
#include "bentoclient/clienttypes.hpp"
#include <string>

namespace bentoclient
{
    /// @brief Date, time and conversions
    class DateUtils{
    private:
        class Impl;
    public:
        /// @brief Creates a UTC timestamp from input
        /// @param year four digit year
        /// @param month calendar month
        /// @param day calendar day
        /// @param hour hour 0 - 23
        /// @param minute minute
        /// @param second second
        /// @return UTC timestamp
        static Timestamp makeTimestampZulu(int year, int month, int day, int hour,
            int minute, int second);
        /// @brief Creates a UTC timestamp shifted from the input in the given timezone
        /// @param year four digit year
        /// @param month calendar month
        /// @param day calendar day
        /// @param hour hour 0 - 23
        /// @param minute minute
        /// @param second second
        /// @param tz the time zone for the above input date 
        /// @return UTC timestamp
        static Timestamp makeTimestamp(int year, int month, int day, int hour,
            int minute, int second, const std::string& tz = Timezone::m_NYC);
        /// @brief Creates a UTC timestamp from yyyy-mm-dd hh:mm:ss input
        /// @param yyyyMmDd yyyy-mm-dd date string
        /// @param hhMmSs hh:mm:ss or hh:mm time string
        /// @param tz The time zone the date-/time-strings are in
        /// @return UTC timestamp
        static Timestamp makeTimestamp(const std::string& yyyyMmDd,
            const std::string& hhMmSs, const std::string& tz = Timezone::m_NYC);
        static Timestamp makeTimestampZulu(const std::string& yyyyMmDd);
        /// @brief convert a timestamp in some local timezone to UTC
        /// @details effects a shift in the timestamp value
        static Timestamp convertTimestampToZulu(const Timestamp& timestamp, 
            const std::string& tz = Timezone::m_NYC);
        /// @brief convert a timestamp in UTC to some local timezone
        /// @details effects a shift in the timestamp value
        static Timestamp convertZuluToTimestamp(const Timestamp& timestamp, 
            const std::string& tz = Timezone::m_NYC);
        /// @brief output a nanoseconds timestring without fractional seconds
        static std::string timestampToStringIntSeconds(const Timestamp& timestamp);
        /// @brief Recognized time zones (not even all of the americas)
        struct Timezone
        {
            static const std::string m_UTC;
            static const std::string m_GMT;
            static const std::string m_EST;
            static const std::string m_CST;
            static const std::string m_MST;
            static const std::string m_PST;
            static const std::string m_NYC;
        };
        /// @brief a definition of when an exchange closes in its time zone 
        /// used for instance for discounting of strike prices
        struct ExchangeClose
        {
            ExchangeClose(int hour, int minute, const std::string& timeZone) :
            m_hour(hour), m_minute(minute), m_timeZone(timeZone) {}

            int m_hour;
            int m_minute;
            std::string m_timeZone;
        };
        /// @brief the only exchange that's currently tested ( :( )
        static const ExchangeClose m_nasdaqClose;
    
    };
}
