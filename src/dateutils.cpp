#include "bentoclient/dateutils.hpp"
//#include <date/tz.h> linking or compativility issues because bento partly uses hinnant stuff
#include <boost/date_time/local_time_adjustor.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
//#include "boost/date_time/c_local_time_adjustor.hpp"
#include <regex>
#include <fmt/chrono.h>

using namespace bentoclient;

const std::string DateUtils::Timezone::m_UTC("UTC");
const std::string DateUtils::Timezone::m_GMT("GMT");
const std::string DateUtils::Timezone::m_EST("EST");
const std::string DateUtils::Timezone::m_CST("CST");
const std::string DateUtils::Timezone::m_MST("MST");
const std::string DateUtils::Timezone::m_PST("PST");
const std::string DateUtils::Timezone::m_NYC("America/New_York");

const DateUtils::ExchangeClose DateUtils::m_nasdaqClose(16,0, DateUtils::Timezone::m_NYC);

class DateUtils::Impl{
public:
    typedef std::function<boost::posix_time::ptime(const boost::posix_time::ptime &t)> TimeConverter;

    static TimeConverter m_utcToUsEastern;
    static TimeConverter m_usEasternToUtc;
    static TimeConverter m_utcToUsPacific;
    static TimeConverter m_usPacificToUtc;
    static TimeConverter m_utcToUsCentral;
    static TimeConverter m_usCentralToUtc;
    static TimeConverter m_utcToUsMountain;
    static TimeConverter m_usMountainToUtc;
    static Timestamp convertTimestamp(const Timestamp& timestamp, 
        const TimeConverter& converter)
    {
        std::time_t time_t_value = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(timestamp));
        boost::posix_time::ptime nyc(boost::posix_time::from_time_t(time_t_value));
        boost::posix_time::ptime utc = converter(nyc);
        auto utcTime = std::chrono::time_point_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::from_time_t(boost::posix_time::to_time_t(utc)));
        return Timestamp(std::chrono::nanoseconds(utcTime.time_since_epoch()));
    }
};

// static member initialization uses lambda functions to force template instantiation
DateUtils::Impl::TimeConverter DateUtils::Impl::m_utcToUsEastern([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -5, 
        boost::posix_time::us_dst> us_eastern;
    return us_eastern::utc_to_local(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_usEasternToUtc([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -5, 
        boost::posix_time::us_dst> us_eastern;
    return us_eastern::local_to_utc(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_utcToUsPacific([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -8, 
        boost::posix_time::us_dst> us_pacific;
    return us_pacific::utc_to_local(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_usPacificToUtc([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -8, 
        boost::posix_time::us_dst> us_pacific;
    return us_pacific::local_to_utc(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_utcToUsCentral([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -6, 
        boost::posix_time::us_dst> us_central;
    return us_central::utc_to_local(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_usCentralToUtc([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -6, 
        boost::posix_time::us_dst> us_central;
    return us_central::local_to_utc(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_utcToUsMountain([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -7, 
        boost::posix_time::us_dst> us_mountain;
    return us_mountain::utc_to_local(t);
});
DateUtils::Impl::TimeConverter DateUtils::Impl::m_usMountainToUtc([](const boost::posix_time::ptime &t) {
    typedef boost::date_time::local_adjustor<boost::posix_time::ptime, -7, 
        boost::posix_time::us_dst> us_mountain;
    return us_mountain::local_to_utc(t);
});

Timestamp DateUtils::makeTimestampZulu(int year, int month, int day, int hour,
    int minute, int second)
{
    std::tm timeinfo = {};
    timeinfo.tm_year = year - 1900; // Year since 1900
    timeinfo.tm_mon = month - 1;    // Month (0-based, so 3 = April)
    timeinfo.tm_mday = day;         // Day of the month
    timeinfo.tm_hour = hour;        // Hour (24-hour format)
    timeinfo.tm_min = minute;       // Minutes
    timeinfo.tm_sec = second;       // Seconds

    // Convert std::tm to std::time_t
    std::time_t time_t_value = timegm(&timeinfo);

    // Convert std::time_t to std::chrono::time_point
    auto timestamp = std::chrono::system_clock::from_time_t(time_t_value);

    // Output the timestamp as nanoseconds since epoch
    Timestamp result(std::chrono::nanoseconds(timestamp.time_since_epoch()));
    return result;
}

Timestamp DateUtils::makeTimestamp(int year, int month, int day, int hour,
        int minute, int second, const std::string& tz)
{
    Timestamp zulu = makeTimestampZulu(year, month, day, hour, minute, second);
    return convertTimestampToZulu(zulu, tz);
}

/// @brief output a nanoseconds timestring without fractional seconds
std::string DateUtils::timestampToStringIntSeconds(const Timestamp& timestamp)
{
    auto timePointSeconds = std::chrono::time_point_cast<std::chrono::seconds>(timestamp);
    return fmt::format("{:%Y-%m-%d %H:%M:%S}", timePointSeconds);
}


namespace
{
    struct _Date
    {
        _Date(int year, int month, int day) :
        m_year(year), m_month(month), m_day(day) {}
        int m_year;
        int m_month;
        int m_day;
        static _Date fromYyyyMmDd(const std::string& yyyyMmDd)
        {
            static const std::regex dateRegex(R"((\d{4})-(\d{2})-(\d{2}))");
            std::smatch match;
            if (std::regex_match(yyyyMmDd, match, dateRegex))
            {
                int year = std::stoi(match[1].str());
                int month = std::stoi(match[2].str());
                int day = std::stoi(match[3].str());
                return _Date(year, month, day);  
            }
            throw std::invalid_argument("Cannot convert '" + yyyyMmDd + "' as yyyy-mm-dd date string");
        }
    };
    struct _Time
    {
        _Time(int hour, int minute, int second) :
        m_hour(hour), m_minute(minute), m_second(second) {}
        int m_hour;
        int m_minute;
        int m_second;
        static _Time fromHhMmSs(const std::string& hhMmSs)
        {
            static const std::regex timeRegex(R"((\d{2}):(\d{2}):(\d{2}))");
            std::smatch match;
            if (std::regex_match(hhMmSs, match, timeRegex))
            {
                int hour = std::stoi(match[1].str());
                int minute = std::stoi(match[2].str());
                int second = std::stoi(match[3].str());
                return _Time(hour, minute, second);  
            } else {
                std::smatch match2;
                static const std::regex shortTimeRegex(R"((\d{2}):(\d{2}))");
                if (std::regex_match(hhMmSs, match2, shortTimeRegex))
                {
                    int hour = std::stoi(match2[1].str());
                    int minute = std::stoi(match2[2].str());
                    return _Time(hour, minute, 0);
                }
            }
            throw std::invalid_argument("Cannot convert '" + hhMmSs + "' as HH:MM:SS time string");
        }
    };
}

Timestamp DateUtils::makeTimestamp(const std::string& yyyyMmDd,
    const std::string& hhMmSs, const std::string& tz)
{
    _Date date = _Date::fromYyyyMmDd(yyyyMmDd);
    _Time time = _Time::fromHhMmSs(hhMmSs);
    return makeTimestamp(date.m_year, date.m_month, date.m_day,
        time.m_hour, time.m_minute, time.m_second, tz);
}

Timestamp DateUtils::makeTimestampZulu(const std::string& yyyyMmDd)
{
    _Date date = _Date::fromYyyyMmDd(yyyyMmDd);
    return makeTimestampZulu(date.m_year, date.m_month, date.m_day, 0, 0, 0);
}

Timestamp DateUtils::convertTimestampToZulu(const Timestamp& timestamp, 
    const std::string& tz)
{
    if (tz == Timezone::m_UTC || tz == Timezone::m_GMT) {
        return timestamp;
    } else if (tz == Timezone::m_EST || tz == Timezone::m_NYC) {
        return Impl::convertTimestamp(timestamp, Impl::m_usEasternToUtc);
    } else if (tz == Timezone::m_PST) {
        return Impl::convertTimestamp(timestamp, Impl::m_usPacificToUtc);
    } else if (tz == Timezone::m_CST) {
        return Impl::convertTimestamp(timestamp, Impl::m_usCentralToUtc);
    } else if (tz == Timezone::m_MST) {
        return Impl::convertTimestamp(timestamp, Impl::m_usMountainToUtc);
    } else {
        throw std::invalid_argument("Unsupported timezone: " + tz);
    }
    // if not for linkage errors due to the incomplete hinnant date integration
    // this might work, see #include "date/tz.h" above.
    // auto zoned = date::zoned_time(
    //     date::locate_zone(tz), zulu);
    // return Timestamp(std::chrono::nanoseconds(zoned.get_sys_time().time_since_epoch()));
}

Timestamp DateUtils::convertZuluToTimestamp(const Timestamp& timestamp,
    const std::string& tz)
{
    if (tz == Timezone::m_UTC || tz == Timezone::m_GMT) {
        return timestamp;
    } else if (tz == Timezone::m_EST || tz == Timezone::m_NYC) {
        return Impl::convertTimestamp(timestamp, Impl::m_utcToUsEastern);
    } else if (tz == Timezone::m_PST) {
        return Impl::convertTimestamp(timestamp, Impl::m_utcToUsPacific);
    } else if (tz == Timezone::m_CST) {
        return Impl::convertTimestamp(timestamp, Impl::m_utcToUsCentral);
    } else if (tz == Timezone::m_MST) {
        return Impl::convertTimestamp(timestamp, Impl::m_utcToUsMountain);
    } else {
        throw std::invalid_argument("Unsupported timezone: " + tz);
    }
}


