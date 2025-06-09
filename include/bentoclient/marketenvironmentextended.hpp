#pragma once

#include "bentoclient/marketenvironment.hpp"
#include <map>
#include <mutex>

namespace bentoclient
{
    /// @brief A market environment reading a yield curve from CSV
    /// @details Extended market environments must remain immutable as 
    /// they are shared across threads without synchronization.
    class MarketEnvironmentExtended : public MarketEnvironment
    {
    public:
        /// @brief Map with yield curve entries as value_type. Though yield curves
        /// are in tenors, the mapped_type has Timestamps of maturity for faster lookup
        /// @details Normally, a yield curve definition should have time ranges
        /// starting from a maturity date as in: 
        /// typedef std::map<Timestamp, std::map<TimeRange, double>> YieldCurveMap;
        typedef std::map<Timestamp, std::map<Timestamp, double>> YieldCurveMap;

        /// @brief Constructs market environment with yield curve
        /// @param fDefaultRiskFreeRate The default risk free rate, if yield curve can't be read
        /// @param exchangeClose Information on the exchange for the underlier
        /// @param yieldCurveCsv Pathname for a yield curve CSV file
        MarketEnvironmentExtended(double fDefaultRiskFreeRate,
            const DateUtils::ExchangeClose& exchangeClose,
            const std::string& yieldCurveCsv);
        ~MarketEnvironmentExtended() override;

        double getRiskFreeRate(Timestamp valuationTime, Timestamp expiryTime) const override;

        /// @brief Finds the key for the record next in time to requested time {at}
        /// @tparam T Mapped type
        /// @param at Requested time
        /// @param timeKeyedMap Map having timestamp as key
        /// @param timeRange Lookup timerange for the closest find to be valid
        /// @return Pair of closest key and if it's in the timeRange
        template<typename T>
        static std::pair<Timestamp, bool> getNextInTimeRange(Timestamp at,
            const std::map<Timestamp, T>& timeKeyedMap, TimeRange timeRange)
        {
            // first element not less than {at}, or end
            auto it = timeKeyedMap.lower_bound(at);
            if (it != timeKeyedMap.end())
            {
                if (it != timeKeyedMap.begin()) {
                    // {at} is in between two keys
                    auto prev = it;
                    --prev;
                    if (at - prev->first < it->first - at) {
                        return { prev->first, at - prev->first < timeRange };
                    } else {
                        return { it->first, it->first - at < timeRange };
                    }
                } else {
                    // only one key same as or above {at}
                    return { it->first, it->first - at < timeRange };
                }
            } 
            else 
            {
                // if map not empty, its last entry will be closest
                auto rit = timeKeyedMap.rbegin();
                if (rit != timeKeyedMap.rend()) {
                    return { rit->first, at - rit->first < timeRange };
                } else {
                    return { Timestamp{}, false };
                }
            }
        }
    private:
        static YieldCurveMap readYieldCurveCsv(const std::string& filename);
    private:
        const YieldCurveMap m_yieldCurveMap;
    };
}