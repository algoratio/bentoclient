#include "bentoclient/marketenvironmentextended.hpp"
#include <boost/log/trivial.hpp>
#include <fmt/core.h>
#include <filesystem>
#include <chrono>
#include <fstream>

using namespace bentoclient;

MarketEnvironmentExtended::MarketEnvironmentExtended(double fDefaultRiskFreeRate,
    const DateUtils::ExchangeClose& exchangeClose,
    const std::string& yieldCurveCsv) :
    MarketEnvironment(fDefaultRiskFreeRate, exchangeClose),
    m_yieldCurveMap(readYieldCurveCsv(yieldCurveCsv))
{
    
}

MarketEnvironmentExtended::~MarketEnvironmentExtended()
{
}

double MarketEnvironmentExtended::getRiskFreeRate(Timestamp valuationTime, 
    Timestamp expiryTime) const
{
    /* No synchronization in this method, since all class data is const */
    if (m_yieldCurveMap.empty())
        return this->m_fRiskFreeRate;
    // Simply take the difference between expiryTime and valuationTime as tenor
    if (expiryTime <= valuationTime) {
        throw std::invalid_argument(fmt::format("Expiry time {} not after valuation time {}",
            serializeTimestamp(expiryTime), serializeTimestamp(valuationTime)));
    }
    // First, get the closest yield curve to valuation time
    auto pair = getNextInTimeRange(valuationTime, m_yieldCurveMap, std::chrono::seconds(1));
    if (pair.first == Timestamp{})
    {
        BOOST_LOG_TRIVIAL(error) << "Found no valid entry in yield curve list for valuation time "
            << serializeTimestamp(valuationTime);
        return this->m_fRiskFreeRate;
    }
    const YieldCurveMap::mapped_type& timeStampedCurve = m_yieldCurveMap.at(pair.first);
    // The timestamps in the timeStampedCurve have the tenor as difference between them and pair.first.
    // Therefore, the expiry time passed into here isn't the ideal lookup, because pair.first
    // may differ significantly from valuationTime, depending on whether the yield curve data
    // covers the valuation time range. And if yield curves files aren't updated...
    // So: use the incoming tenor as a shift from curve base time as a lookup key
    TimeRange tenor = std::chrono::duration_cast<TimeRange>(expiryTime - valuationTime);
    Timestamp lookupTime = pair.first + tenor;
    auto yieldPair = getNextInTimeRange(lookupTime, timeStampedCurve, std::chrono::seconds(1));
    if (yieldPair.first == Timestamp{})
    {
        BOOST_LOG_TRIVIAL(error) << "Found no valid entry in best yield curve for lookup time "
            << serializeTimestamp(lookupTime) << " shifted from expiry time " 
            << serializeTimestamp(expiryTime);
        return this->m_fRiskFreeRate;
    }
    // TODO: if options of longer durations are important, interpolate between points 
    // in the par yield curve. However, for options up to 35 DTE, the algo will always pick the
    // first point in the curve anyway.
    double tsyRate = timeStampedCurve.at(yieldPair.first);
    // Convert semiannual par rate (percent) to continuously compounded rate
    // and as to the constant below, C++ 20 is using years = duration<_GLIBCXX_CHRONO_INT64_T, ratio<31556952>>;
    // so it must be right :)
    double tenor_years = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<31556952>>>(tenor).count();
    double r_sa = tsyRate / 100.0;
    double r_cc = 2.0 * std::log(1.0 + r_sa / 2.0);
    return r_cc;
}

MarketEnvironmentExtended::YieldCurveMap MarketEnvironmentExtended::readYieldCurveCsv(const std::string& filename)
{
    /* Yield curve format from the Treasury:
Date,"1 Mo","2 Mo","3 Mo","4 Mo","6 Mo","1 Yr","2 Yr","3 Yr","5 Yr","7 Yr","10 Yr","20 Yr","30 Yr"
11/29/2024,4.76,4.69,4.58,4.52,4.42,4.30,4.13,4.10,4.05,4.10,4.18,4.45,4.36
11/27/2024,4.76,4.70,4.60,4.54,4.43,4.34,4.19,4.17,4.11,4.17,4.25,4.52,4.44
11/26/2024,4.74,4.67,4.61,4.52,4.45,4.37,4.21,4.21,4.17,4.24,4.30,4.56,4.48

    Note the change now having a column for 1.5 Month!
Date,"1 Mo","1.5 Month","2 Mo","3 Mo","4 Mo","6 Mo","1 Yr","2 Yr","3 Yr","5 Yr","7 Yr","10 Yr","20 Yr","30 Yr"
06/06/2025,4.28,4.31,4.35,4.43,4.38,4.31,4.14,4.04,4.02,4.13,4.31,4.51,4.99,4.97

    Sample request lines
    my $tsymonthly = "https://home.treasury.gov/resource-center/data-chart-center"
        "/interest-rates/daily-treasury-rates.csv/all/202411?"
        "type=daily_treasury_yield_curve&field_tdr_date_value_month=202411&page&_format=csv";

    my $tsyanually = https://home.treasury.gov/resource-center/data-chart-center/"
    "interest-rates/daily-treasury-rates.csv/2025/all?"
    "field_tdr_date_value=2025&type=daily_treasury_yield_curve&page&_format=csv"

    See ./scripts/tsy2csv.pl

    */

    YieldCurveMap yieldCurveMap;

    std::filesystem::path path(filename);
    if (!std::filesystem::exists(path)) {
        BOOST_LOG_TRIVIAL(error) << "Yield curve file does not exist: " << filename;
        return yieldCurveMap;
    }

    std::ifstream ifs(filename);
    if (!ifs) {
        BOOST_LOG_TRIVIAL(error) << "Failed to open yield curve file: " << filename;
        return yieldCurveMap;
    }

    std::string headerLine;
    if (!std::getline(ifs, headerLine)) {
        BOOST_LOG_TRIVIAL(error) << "Yield curve file is empty: " << filename;
        return yieldCurveMap;
    }

    // Parse header columns (skip first column "Date")
    std::vector<TimeRange> tenors;
    std::istringstream headerStream(headerLine);
    std::string col;
    std::getline(headerStream, col, ','); // skip "Date"
    while (std::getline(headerStream, col, ',')) {
        // Remove quotes if present
        if (!col.empty() && col.front() == '"') col.erase(0, 1);
        if (!col.empty() && col.back() == '"') col.pop_back();

        // Parse tenor
        TimeRange tenor;
        if (col.find("Mo") != std::string::npos) {
            double months = std::stod(col);
            // Use duration_cast to TimeRange from fractional months
            auto seconds = static_cast<std::uint64_t>(months * 2629746); // average seconds in a month
            tenor = std::chrono::seconds(seconds);
        } else if (col.find("Yr") != std::string::npos) {
            double years = std::stod(col);
            auto seconds = static_cast<std::uint64_t>(years * 31556952); // average seconds in a year
            tenor = std::chrono::seconds(seconds);
        } else {
            BOOST_LOG_TRIVIAL(warning) << "Unknown tenor column: " << col;
            tenor = TimeRange::zero();
        }
        tenors.push_back(tenor);
    }

    // Parse data rows
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream lineStream(line);
        std::string dateStr;
        std::getline(lineStream, dateStr, ',');

        // Parse date (assume format MM/DD/YYYY)
        std::tm tm = {};
        std::istringstream dateStream(dateStr);
        dateStream >> std::get_time(&tm, "%m/%d/%Y");
        if (dateStream.fail()) {
            BOOST_LOG_TRIVIAL(warning) << "Failed to parse date: " << dateStr;
            continue;
        }
        Timestamp dateTs = DateUtils::makeTimestamp(
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 0, 0, 0, DateUtils::Timezone::m_UTC);

        // Parse rates
        YieldCurveMap::mapped_type curve;
        for (const auto& tenor : tenors) {
            std::string rateStr;
            if (!std::getline(lineStream, rateStr, ',')) break;
            try {
                double rate = std::stod(rateStr);
                // Normally, yield curves have tenors. However, to avoid repeated
                // later conversions, turn tenors into maturity dates here.
                curve[dateTs + tenor] = rate;
            } catch (const std::exception& e) {
                // Due to the recent change in TSY yield curves, some
                // dates have the 1.5 month rate empty! That's fine, just
                // skip the column
                BOOST_LOG_TRIVIAL(debug) << "Failed to parse rate: '" << rateStr
                    << "', underlying error: " << e.what();
            }
        }
        yieldCurveMap[dateTs] = curve;
    }
    return yieldCurveMap;
}


