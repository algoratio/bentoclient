#include "bentoclient/optionchain.hpp"
#include "bentoclient/osioption.hpp"
#include "bentoclient/optionrecordgapfiller.hpp"
#include <boost/log/trivial.hpp>
#include <fmt/core.h>
#include <functional>

using namespace bentoclient;

const std::string OptionRecordGapFiller::m_pcpFitComment("pcp-fit");
const std::string OptionRecordGapFiller::m_spreadFitComment("spread-fit");
const std::string OptionRecordGapFiller::m_linInterpolComment("lin-interpol");
const std::string OptionRecordGapFiller::m_logExtrapolateComment("log-extrapolate");

class OptionRecordGapFiller::Algos
{
    // sets a lower limit for double values to take logs from to ensure validity 
    static constexpr double m_logLowerLimit = 1e-9;
public:
    /// @brief The result of a put-call-parity computation on matching strike options
    struct PCPResult
    {
        PCPResult():
        m_pcpConsistentRate(std::nan("0xbad")),
        m_putPrice(std::nan("0xbad")),
        m_callPrice(std::nan("0xbad")),
        m_isValid(false)
        {}
        PCPResult(double pcpConsistentRate,
            double putPrice, double callPrice): 
        m_pcpConsistentRate(pcpConsistentRate),
        m_putPrice(putPrice),
        m_callPrice(callPrice),
        m_isValid(true)
        {}
        /// @brief The put-call-parity price from a put call same strike options pair
        double m_pcpConsistentRate;
        /// @brief Price of the put option
        double m_putPrice;
        /// @brief Price of the call option
        double m_callPrice;
        /// @brief Indicates that a computation on a Record pair matched on strike could be performed
        bool m_isValid;
    };
    using Record = OptionChain::Record;
    using RecordMap = OptionChain::RecordMap;
    /// @brief maps strike key to put-call-parity computation
    typedef std::map<std::string, PCPResult> PCPMap;
    /// @brief Slope and intercept of a least squares fit line
    typedef std::pair<double, double> LSFitValue;
    // type for points to perform a LS fit on 
    typedef std::list<std::pair<double, double>> FitPoints;
    /// @brief Indicates whether a fit is extrapolating (start or end) or interpolating (gap)
    enum class FitType
    {
        Start, Gap, End
    };
    /// @brief A least squares fit in the scope of next unfitted valid records
    struct LSFit
    {
        LSFit() = delete;
        LSFit(const LSFitValue& fitValue, 
            FitType type, 
            const std::string& lowerKey,
            const std::string& upperKey):
        m_fit(fitValue),
        m_type(type),
        m_lowerKey(lowerKey),
        m_upperKey(upperKey)
        {}
        LSFit(const FitPoints& fitPoints, 
            FitType type, 
            const std::string& lowerKey,
            const std::string& upperKey):
        m_fit(OptionChain::Util::fitLeastSquaresLine(fitPoints)),
        m_type(type),
        m_lowerKey(lowerKey),
        m_upperKey(upperKey)
        {}
        /// @brief slope and intercept of fit line
        LSFitValue m_fit;
        /// @brief indicates start, gap, and end fits 
        FitType m_type;
        /// @brief record key having valid values on lower end of fit range
        std::string m_lowerKey;
        /// @brief record key having valid values on upper end of fit range
        std::string m_upperKey;
    };
    /// @brief maps strike keys to least squares fits
    typedef std::map<std::string, LSFit> LSFitMap;

    /// @brief Iterates strike key matched records of an option chain to compute put-call-parities
    /// @param optionChain Option chain holding put/call records
    /// @param discountFactor Discount factor for the strike as a zero coupon bond
    /// @return map of strike key to put-call-parity computation
    static PCPMap matchPutCall(const OptionChain& optionChain, double discountFactor)
    {
        OptionChain::Util::PutCallParityRate parityRateCalculator(discountFactor);
        auto pcpFunctor = std::function<PCPResult(const RecordMap::value_type&, const RecordMap::value_type&)>(
            [&parityRateCalculator](const RecordMap::value_type& put, const RecordMap::value_type& call)
            {
                if (put.second.bidAskValid() && call.second.bidAskValid())
                {
                    double pcpRate = parityRateCalculator(put, call);
                    return PCPResult{ pcpRate, put.second.getMidPrice(), call.second.getMidPrice() };
                } else {
                    return PCPResult{ };
                }
            }
        );
        PCPMap result = OptionChain::Util::onAllPutCallRecords(
            optionChain, pcpFunctor, false);
        return result;
    }

    /// @brief Clears records from map M1 that have no matching key in map M2
    /// @tparam M1 Map1 type
    /// @tparam M2 Map2 type
    /// @param clearFrom Map to clear records from
    /// @param keysIn Map that holds the keys that should remain in clearFrom
    /// @return A list of keys erased from clearFrom map
    template <typename M1, typename M2>
    static std::list<std::string> removeElementsNotInKeys(M1& clearFrom, const M2& keysIn)
    {
        std::list<std::string>  erasedKeys;
        for (auto m1It = clearFrom.begin(); m1It != clearFrom.end();)
        {
            if (keysIn.find(m1It->first) == keysIn.end())
            {
                auto toErase = m1It++;
                erasedKeys.emplace_back(std::move(toErase->first));
                clearFrom.erase(toErase);
            } else {
                ++m1It;
            }
        }
        return erasedKeys;
    }
    /// @brief Produces a map of strike keys to least squares fits for gaps
    /// @param pcpMap Map of strike key to put-call-parity computation
    /// @return Map of least squares fits
    static LSFitMap fitPCPRateForGaps(const PCPMap& pcpMap)
    {
        LSFitMap lsFits;
        // a sequence of contiguous strike keys without valid pcp values
        std::set<std::string> currentGap;
        // puts a fit for the corrent gap into the output map
        auto putFit = [&lsFits,&currentGap](const FitPoints& points, FitType type, 
            const std::string& lowerKey, const std::string& upperKey)
        {
            LSFit fit(points, type, lowerKey, upperKey);
            for (auto& key : currentGap)
            {
                lsFits.insert({key, fit});
            }
        };
        // puts a fit for a gap in between valid upper and lower values
        auto gapFitter = [&pcpMap, &lsFits, &putFit]( 
            PCPMap::const_iterator previous, PCPMap::const_iterator next)
        {
            auto pprevious = previous;
            FitPoints points;
            while (pprevious != pcpMap.begin()) {
                --pprevious;
                if (pprevious->second.m_isValid) {
                    points.push_back({OsiOption::fromStrikeKey(pprevious->first),
                        pprevious->second.m_pcpConsistentRate});
                    break;
                }
            }
            points.push_back({OsiOption::fromStrikeKey(previous->first),
                previous->second.m_pcpConsistentRate});
            points.push_back({OsiOption::fromStrikeKey(next->first),
                next->second.m_pcpConsistentRate});
            std::string upperKey = next->first;
            while (++next != pcpMap.end()) {
                if (next->second.m_isValid) {
                    points.push_back({OsiOption::fromStrikeKey(next->first),
                        next->second.m_pcpConsistentRate});
                    break;
                }
            }
            putFit(points, FitType::Gap, previous->first, upperKey);
        };
        // puts a fit at the lower end of strike keys, if no previous valid pcp value exists
        // fit is performed on logarithms of OTM put prices
        auto startFitter = [&pcpMap, &lsFits, &putFit](
            PCPMap::const_iterator next)
        {
            // at start of series, values of far OTM puts may be estimated
            // the fit is then done on logs of valid OTM put prices.
            // Loglinear interpols avoid overflows by setting lower limit of 
            // 1e-9
            constexpr size_t min_start_points = 24;
            FitPoints points;
            std::string upperKey = next->first;
            do {
                if (next->second.m_isValid) {
                    points.push_back({OsiOption::fromStrikeKey(next->first), 
                        std::log(std::max(m_logLowerLimit, next->second.m_putPrice))});
                }
            } while (points.size() < min_start_points && ++next != pcpMap.end());
            if (points.size() >= min_start_points/6) {
                putFit(points, FitType::Start, std::string{}, upperKey);
            }
        };
        // puts a fit at the upper and of strike keys, if no next valid pcp value exists
        // fit is performed on logarithm of OTM call prices
        auto endFitter = [&pcpMap, &lsFits, &putFit](
            PCPMap::const_iterator last)
        {
            // at end of series, values of far OTM calls may be estimated
            // fits are done on logs of valid OTM calls
            // Loglinear interpols avoid overflows by setting lower limit of 
            // 1e-9
            constexpr size_t min_end_points = 24;
            FitPoints points;
            std::string lowerKey = last->first;
            do  {
                if (last->second.m_isValid) {
                    points.push_front({OsiOption::fromStrikeKey(last->first),
                        std::log(std::max(m_logLowerLimit, last->second.m_callPrice))});
                }
                if (last != pcpMap.begin()) --last;
            } while(points.size() < min_end_points && last != pcpMap.begin());
            if (points.size() >= min_end_points/6) {
                putFit(points, FitType::End, lowerKey, std::string{});
            }
        };
        // Finally, invoke the appropriate type of fitter for existing gaps
        auto previousValid = pcpMap.end();
        for (auto it = pcpMap.begin(); it != pcpMap.end(); ++it)
        {
            if (it->second.m_isValid == false){
                // Gap without a valid PCP rate
                currentGap.insert(it->first);
            } else {
                if (!currentGap.empty())
                {
                    if (previousValid != pcpMap.end())
                    {
                        gapFitter(previousValid, it);
                    } else {
                        startFitter(it);
                    }
                    currentGap.clear();
                }
                previousValid = it;
            }
        }
        if (!currentGap.empty())
        {
            endFitter(previousValid);
        }
        return lsFits;
    }
    static Timestamp getRecvTime(const RecordMap& recordMap, const std::string& key)
    {
        auto it = recordMap.find(key);
        if (it != recordMap.end())
        {
            return it->second.m_recvTime;
        }
        throw std::invalid_argument("getTimestamp: no record for key " + key);
    }
    /// @brief Computes spread for a record matched by key
    /// @param recordMap maps strike keys to records
    /// @param key key to compute spread for
    /// @return bid-ask spread
    static double computeSpread(const RecordMap& recordMap, const std::string& key)
    {
        auto it = recordMap.find(key);
        if (it != recordMap.end())
        {
            if (it->second.bidAskValid())
            {
                return it->second.getSpread();
            }
            throw std::invalid_argument("computeSpread: no valid spread value for key " + key);
        }
        throw std::invalid_argument("computeSpread: no record for key " + key);
    }
    /// @brief Average bid-ask spread for two records matched by key
    /// @param recordMap maps strike keys to records
    /// @param key1 first key for spread
    /// @param key2 second key for spread
    /// @return average bid-ask spread
    static double computeSpread(const RecordMap& recordMap, const std::string& key1, const std::string& key2)
    {
        return (computeSpread(recordMap, key1) + computeSpread(recordMap, key2)) / 2.0;
    }

    /// @brief Estimates the ATM price of the put or call series, if minimum quality requirements met
    /// @param recordMap Record map to compute the ATM price from
    /// @param pcpRate Put-call-party rate for underlier
    /// @return Estimate of ATM price, or throws exception if insufficient data
    static double estimateAtmPrice(const RecordMap& recordMap, double pcpRate)
    {
        std::string atmKey = OsiOption::toStrikeKey(pcpRate);
        auto atmLower = recordMap.lower_bound(atmKey);
        if (atmLower != recordMap.begin())
        {
            auto atmPrevious = atmLower;
            do {
                --atmPrevious;
            } while (atmPrevious != recordMap.begin() && !atmPrevious->second.bidAskValid());
            while(atmLower != recordMap.end() && !atmLower->second.bidAskValid()) {
                ++atmLower;
            }
            if (atmPrevious->second.bidAskValid() && atmLower != recordMap.end())
            {
                auto distance = std::distance(atmPrevious, atmLower);
                if (distance < 4)
                {
                    return (atmPrevious->second.getMidPrice() + atmLower->second.getMidPrice()) / 2;
                }
            }
        }
        throw std::invalid_argument(fmt::format("Failed to estimate ATM price for PCP rate {}", pcpRate));
    }

    static double interpolate(const RecordMap& recordMap, 
        double targetStrike, const std::string& lowerKey, const std::string& upperKey)
    {
        auto lowerIt = recordMap.find(lowerKey);
        auto upperIt = recordMap.find(upperKey);
        if (lowerIt != recordMap.end() && upperIt != recordMap.end())
        {
            if (lowerIt->second.bidAskValid() && upperIt->second.bidAskValid())
            {
                double lowerMidPrice = lowerIt->second.getMidPrice();
                double upperMidPrice = upperIt->second.getMidPrice();
                double lowerStrike = OsiOption::fromStrikeKey(lowerKey);
                double upperStrike = OsiOption::fromStrikeKey(upperKey);
                double interpolated = lowerMidPrice 
                    + (upperMidPrice - lowerMidPrice)*(targetStrike - lowerStrike)/(upperStrike - lowerStrike);
                return interpolated;
            }

        }
        throw std::invalid_argument(fmt::format("Unable to perform linear interpolation on strike and keys {},{},{}",
            targetStrike, lowerKey, upperKey));
    }

    static void fillFitValue(double discountFactor, const LSFitMap& fitMap, RecordMap& putMap, RecordMap& callMap, 
        double atmPrice)
    {
        for (auto it = fitMap.begin(); it != fitMap.end(); ++it) 
        {
            auto putIt = putMap.find(it->first);
            auto callIt = callMap.find(it->first);
            if (putIt != putMap.end() && callIt != callMap.end())
            {
                double strike = OsiOption::fromStrikeKey(it->first);
                if (it->second.m_type == FitType::Gap)
                {
                    // If there is a gap in between good data, can fill it
                    // by put-call-parity, if at least the put or call side
                    // is filled. On the ends of the chain, put-call-parity
                    // does not necessarily give realisitc estimates.
                    // compute put/call parity rate from fit
                    double pcpRate = it->second.m_fit.first * strike + it->second.m_fit.second;
                    double computedPrice{};
                    double spread{};
                    Timestamp recvTime{};
                    std::list<RecordMap::iterator> targetElement;
                    std::list<std::reference_wrapper<RecordMap>> targetMap;
                    // Put/Call Parity: C+B=P+S
                    if (!putIt->second.bidAskValid() && callIt->second.bidAskValid())
                    {
                        // P = C+B-S
                        computedPrice = callIt->second.getMidPrice() 
                            + strike * discountFactor 
                            - pcpRate;
                        targetElement.push_back(putIt);
                        targetMap.push_back(putMap);
                        spread = computeSpread(putMap, it->second.m_lowerKey, it->second.m_upperKey);
                        recvTime = getRecvTime(putMap, it->second.m_upperKey);
                    } else if (!callIt->second.bidAskValid() && putIt->second.bidAskValid())
                    {
                        // C=P+S-B
                        computedPrice = putIt->second.getMidPrice()
                            + pcpRate
                            - strike * discountFactor;
                        targetElement.push_back(callIt);
                        targetMap.push_back(callMap);
                        spread = computeSpread(callMap, it->second.m_lowerKey, it->second.m_upperKey);
                        recvTime = getRecvTime(callMap, it->second.m_lowerKey);
                    }
                    if (!targetElement.empty())
                    {
                        // do not use put-call-parity fits when they result in low values, compared
                        // to ATM prices. So for comparison, first get the ATM price
                        std::string comment = m_pcpFitComment;
                        double atmPriceThreshold = atmPrice/4;
                        if (computedPrice < atmPriceThreshold) {
                            double linInterpolPrice = interpolate(targetMap.front(), 
                                strike, it->second.m_lowerKey, it->second.m_upperKey);
                            BOOST_LOG_TRIVIAL(info) << "Overwrite PCP computed price from " << computedPrice << " to "
                                << linInterpolPrice << " because it's less than threshold " << atmPriceThreshold;
                            computedPrice = linInterpolPrice;
                            comment = m_linInterpolComment;
                        }
                        // have a computed price for a put / call side to fill.
                        // but need bid/ask spread.
                        targetElement.front()->second.m_askPrice = OptionChain::PriceWeight(
                            computedPrice + spread / 2.0, 1);
                        targetElement.front()->second.m_bidPrice = OptionChain::PriceWeight(
                            std::max(0.0, computedPrice - spread / 2.0), 1);
                        addComment(targetElement.front()->second.m_comment, comment);
                        targetElement.front()->second.m_recvTime = recvTime;
                    }
                } else if (it->second.m_type == FitType::Start
                        || it->second.m_type == FitType::End) {
                    // may be able to extrapolate far OTM puts on lower end of strike series
                    // or far OTM calls on upper end of strike series
                    const RecordMap& map = it->second.m_type == FitType::Start ? 
                        putMap : callMap;
                    std::string key = it->second.m_type == FitType::Start ? 
                        it->second.m_upperKey : it->second.m_lowerKey;
                    RecordMap::iterator targetIt = it->second.m_type == FitType::Start ?
                        putIt : callIt;
                    double spread = computeSpread(map, key);
                    Timestamp recvTime = getRecvTime(map, key);
                    double logPrice = strike * it->second.m_fit.first + it->second.m_fit.second;
                    double price = std::exp(logPrice);
                    targetIt->second.m_askPrice = OptionChain::PriceWeight(
                        price + spread / 2.0, 1);
                    targetIt->second.m_bidPrice = OptionChain::PriceWeight(
                        std::max(0.0, price - spread / 2.0), 1);
                    addComment(targetIt->second.m_comment, m_logExtrapolateComment);
                    targetIt->second.m_recvTime = recvTime;
                }
            }
        }
    }
    static void addComment(std::string& target, const std::string& source)
    {
        if (!target.empty())
        {
            target += ":" + source;
        }
        else
        {
            target = source;
        }
    }
    static std::pair<double, bool> getSpread(const Record& record)
    {
        if (record.bidAskValid())
            return std::make_pair(record.getSpread(), true);
        else
            return std::make_pair(std::nan("0xbad"), false);
    }
    static void spreadFit(RecordMap& recordMap)
    {
        FitPoints spreadPoints;
        std::set<std::string> fitKeys;
        for (auto recordIt = recordMap.begin(); recordIt != recordMap.end(); ++recordIt)
        {
            auto pair = getSpread(recordIt->second);
            if (pair.second) {
                spreadPoints.push_back(std::make_pair(
                    OsiOption::fromStrikeKey(recordIt->first), pair.first)
                );
            } else if (recordIt->second.anyBidAskValid()) {
                fitKeys.insert(recordIt->first);
            }
        }
        if (fitKeys.empty()) {
            return;
        }
        try
        {
            LSFitValue fit(OptionChain::Util::fitLeastSquaresLine(spreadPoints));
            for (auto& fitKey : fitKeys)
            {
                double fitX = OsiOption::fromStrikeKey(fitKey);
                double fittedSpread = std::max(fitX * fit.first + fit.second, 0.01);
                Record& record = recordMap[fitKey];
                if (std::get<1>(record.m_askPrice) > 0)
                {
                    std::get<0>(record.m_bidPrice) = std::max(
                        std::get<0>(record.m_askPrice) - fittedSpread, 0.0
                    );
                    std::get<1>(record.m_bidPrice) = 1;
                } else {
                    std::get<0>(record.m_askPrice) = std::get<0>(record.m_bidPrice) + fittedSpread;
                    std::get<1>(record.m_askPrice) = 1;
                }
                addComment(record.m_comment, m_spreadFitComment);
            }
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(warning) << "Unable to perform spread-fit: " << e.what();
        }
    }
};

OptionChain OptionRecordGapFiller::fillGaps(const OptionChain& optionChain)
{
    OptionChain filledChain(optionChain);
    // First off, try to complete any incomplete records, typically having an ask, no bid.
    Algos::spreadFit(filledChain.m_callsStrikeKeyToRecord);
    Algos::spreadFit(filledChain.m_putsStrikeKeyToRecord);
    double fRiskFreeRate = m_marketEnvironment->getRiskFreeRate(
        optionChain.getChainTime(),
        optionChain.getExpiryTime(m_marketEnvironment->getExchangeClose())
    );
    double discountFactor = OptionChain::Util::getDiscountFactor(filledChain, fRiskFreeRate,
        m_marketEnvironment->getExchangeClose());
    Algos::PCPMap pcpMap = Algos::matchPutCall(filledChain, discountFactor);
    m_orphanedCalls = Algos::removeElementsNotInKeys(filledChain.m_callsStrikeKeyToRecord, pcpMap);
    m_orphanedPuts = Algos::removeElementsNotInKeys(filledChain.m_putsStrikeKeyToRecord, pcpMap);
    try {
        double parityRate = filledChain.getParityRate(fRiskFreeRate, m_marketEnvironment->getExchangeClose());
        double putAtmPrice = Algos::estimateAtmPrice(filledChain.m_putsStrikeKeyToRecord, parityRate);
        double callAtmPrice = Algos::estimateAtmPrice(filledChain.m_callsStrikeKeyToRecord, parityRate);
        Algos::LSFitMap fitMap = Algos::fitPCPRateForGaps(pcpMap);
        Algos::fillFitValue(discountFactor, fitMap, 
            filledChain.m_putsStrikeKeyToRecord, filledChain.m_callsStrikeKeyToRecord, 
            (putAtmPrice + callAtmPrice)/2);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(warning) << "Failed to perform advanced fill operations: " << e.what();
    }
    return filledChain;
}

