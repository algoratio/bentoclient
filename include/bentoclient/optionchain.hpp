#pragma once

#include <map>
#include <string>
#include <list>
#include <databento/record.hpp>
#include <tuple>
#include "bentoclient/dateutils.hpp"

namespace bentoclient {

class OptionInstruments;
class OsiOption;
class OptionRecordGapFiller;
/// @brief Option chain data per symbol, date and expiry date.
/// @details Internally maps strike keys to option price records. In conjunction with
/// the associated OptionInstruments instance, strike keys return the OsiOption 
/// instrument description. 
class OptionChain {
    class Algos;
    friend class Algos;
    friend class OptionRecordGapFiller;
public:
    typedef std::map<std::string, std::list<databento::CbboMsg>> InstrumentIdToCbboMap;
    /// @brief PriceWeight behaves like a tuple, but offers an operator ==
    /// taking into account possible nan or undefined values, which are irrelevant
    /// due to their weight() == 0.
    struct PriceWeight : public std::tuple<double, uint64_t> {
        // avoid redefining constructors
        using std::tuple<double, uint64_t>::tuple;
        // named accessors
        double& price() { return std::get<0>(*this); }
        uint64_t& weight() { return std::get<1>(*this); }
        const double& price() const { return std::get<0>(*this); }
        const uint64_t& weight() const { return std::get<1>(*this); }
        // custom operator not conflicting with other overloads for tuples
        bool operator==(const PriceWeight& other) const {
            // for empty PriceWeights possibly having nan as a value,
            // only compare weights
            return (weight() == 0 && other.weight() == 0)
                || (weight() == other.weight()
                    && price() == other.price());
        }
    };    
    /// @brief Record is a handy subset of CbboMsg
    struct Record {
        Record();
        Record(PriceWeight&& price,
            Timestamp&& priceTime,
            PriceWeight&& ask,
            PriceWeight&& bid,
            Timestamp&& recvTime);
        Record(const databento::CbboMsg& cbboMsg);
        Record(Record&&) = default;
        Record& operator=(Record&&) = default;
        Record(const Record&) = default;
        Record& operator=(const Record&) = default;

        bool operator == (const Record& other) const;
        double getBidPrice() const
        {
            if (std::get<1>(m_bidPrice) == 0)
            {
                return std::nan("0xbad");
            }
            return std::get<0>(m_bidPrice);
        }

        double getAskPrice() const
        {
            if (std::get<1>(m_askPrice) == 0)
            {
                return std::nan("0xbad");
            }
            return std::get<0>(m_askPrice);
        }

        double getMidPrice() const
        {
            return (getAskPrice() + getBidPrice()) / 2.0;
        }

        bool isValid() const {
            return std::get<1>(m_price) > 0 
                && std::get<1>(m_askPrice) > 0
                && std::get<1>(m_bidPrice) > 0;       
        }

        bool bidAskValid() const {
            return std::get<1>(m_askPrice) > 0
                && std::get<1>(m_bidPrice) > 0;
        }

        bool anyBidAskValid() const {
            return std::get<1>(m_askPrice) > 0
                || std::get<1>(m_bidPrice) > 0;
        }

        double getSpread() const
        {
            return std::get<0>(m_askPrice) -
                std::get<0>(m_bidPrice);
        }

        double getTradePrice() const
        {
            if (std::get<1>(m_price) == 0)
            {
                return std::nan("0xbad");
            }
            return std::get<0>(m_price);
        }

        Timestamp getTradeTime() const
        {
            if (std::get<1>(m_price) == 0)
            {
                return Timestamp{};
            }
            return m_priceTime;
        }

        Timestamp getRecvTime() const
        {
            if (anyBidAskValid())
                return m_recvTime;
            return Timestamp{};
        }

        bool empty() const
        {
            return std::get<1>(m_price) == 0
                && std::get<1>(m_askPrice) == 0
                && std::get<1>(m_bidPrice) == 0;
        }

        PriceWeight m_price;
        Timestamp m_priceTime;
        PriceWeight m_askPrice;
        PriceWeight m_bidPrice;
        Timestamp m_recvTime;
        std::string m_comment;
        static const std::uint64_t priceScaling;
    };
    /// @brief strike key to Record
    typedef std::map<std::string, Record> RecordMap;
    /// @brief Put/call combination of record maps
    typedef std::pair<RecordMap, RecordMap> PutCallRecordMap;
    /// @brief Timeline maps record time slots to pairs of put and call records
    typedef std::map<Timestamp, PutCallRecordMap> RecordTimeline;
public:
    OptionChain() = default;
    // Deleted copy constructor and assignment operator
    OptionChain(const OptionChain&) = default;
    OptionChain& operator=(const OptionChain&) = default;
    // Deleted move constructor and assignment operator
    OptionChain(OptionChain&&) = default;
    OptionChain& operator=(OptionChain&&) = default;
    ~OptionChain() = default;
public:

    // ** Methods to build an option chain **


    /// @brief Maps cbbo messages to their instrument IDs
    static InstrumentIdToCbboMap mapCbboMsgsToInstruments(
        std::list<databento::CbboMsg>&& cbboMsgs,
        const std::map<std::string, std::string>& instrumentIdToOsiMap);

    /// @brief Record timeline maps time slots to available strike keys with data
    static RecordTimeline buildRecordTimeline(const InstrumentIdToCbboMap& cbboMap,
        const std::map<std::string, std::string>& idToOsi,
        TimeRange slotWindow);

    /// @brief Revert the timeline back to RecordMaps having just the latest and best data
    static PutCallRecordMap mapLatestBestInTimelineToRecord(
        const RecordTimeline& timeline);

    /// @brief Find instrument IDs without mapped cbbo messages
    /// @details Called for cbbo 1s data to find instruments for a 
    /// secondary cbbo 1m query to reduce data gaps
    static std::vector<std::string> findInstrumentsMissingCbboMsgs(
        const InstrumentIdToCbboMap& cbboMap,
        const std::map<std::string, std::string>& instrumentIdToOsiMap);

    /// @brief Build optionchain from timeline reduce
    static OptionChain build(PutCallRecordMap&& recordMaps,
        const OptionInstruments& optionInstruments);

    // ** Attributes 

    /// @brief gets the symbol / underlier of the option
    const std::string& getUnderlier() const {
        return m_underlier;
    }

    /// @brief gets the yyyy-mm-dd valuation date
    const std::string& getValuationDate() const {
        return m_valuationDate;
    }

    /// @brief gets the yyyy-mm-dd expiry date
    const std::string& getExpiryDate() const {
        return m_expiryDate;
    }

    /// @brief Gets the put record map
    /// @return Record map
    const RecordMap& getPuts() const {
        return m_putsStrikeKeyToRecord;
    }

    /// @brief Gets the call record map
    /// @return Record map
    const RecordMap& getCalls() const {
        return m_callsStrikeKeyToRecord;
    }

    /// @brief Gets missing instruments, if any
    /// @return map of instrument id to OSI id
    const std::map<std::string, std::string>& getMissingInstrumentIdToOsiMap() const {
        return m_missingInstrumentIdToOsiMap;
    }

    /// @brief Brief check if option data is valid
    bool isValid() const {
        return m_putsStrikeKeyToRecord.size() > 0
            && m_callsStrikeKeyToRecord.size() > 0
            && m_putsStrikeKeyToRecord.size() > m_missingInstrumentIdToOsiMap.size()
            && m_callsStrikeKeyToRecord.size() > m_missingInstrumentIdToOsiMap.size();
    }
    class Util;
    friend class Util;
    /// @brief Get the (average) time stamp for Records
    Timestamp getChainTime() const;
    /// @brief Get the expiry time for the option chain, given close time for the exchange
    Timestamp getExpiryTime(
        const DateUtils::ExchangeClose& exchangeClose = DateUtils::m_nasdaqClose) const;
    /// @brief Get the put-call parity rate consistent with put/call records
    double getParityRate(double fRiskFreeRate, 
        const DateUtils::ExchangeClose& exchangeclose = DateUtils::m_nasdaqClose) const;
    /// @brief Get a parity rate quality score
    double getParityRateQualityScore(double fRiskFreeRate, 
        const DateUtils::ExchangeClose& exchangeClose = DateUtils::m_nasdaqClose) const;
private:
    std::string m_underlier;
    std::string m_valuationDate;
    std::string m_expiryDate;
    RecordMap m_putsStrikeKeyToRecord;
    RecordMap m_callsStrikeKeyToRecord;
    std::map<std::string, std::string> m_missingInstrumentIdToOsiMap;
};

/// @brief Utilities around computing and filling and option chain
class OptionChain::Util {
public:
    /// @brief Functor for computation of put-call-parity rate
    class PutCallParityRate {
    public:
        PutCallParityRate(double discountFactor)
            : m_discountFactor(discountFactor) {}
        PutCallParityRate(const PutCallParityRate&) = default;
        PutCallParityRate& operator=(const PutCallParityRate&) = default;
        PutCallParityRate(PutCallParityRate&&) = default;
        PutCallParityRate& operator=(PutCallParityRate&&) = default;
        ~PutCallParityRate() = default;
        double operator()(const RecordMap::value_type& put, const RecordMap::value_type& call) const;

    private:
        double m_discountFactor;  
    };
    
    /// @brief Computes discount factor for the discounting of strikes in put-call-parity
    /// @param optionChain Reference to an option chain 
    /// @param fContinuousRate The continuously compounded risk-free rate
    /// @param exchangeClose The definition of the exchange for expiry time
    /// @return 
    static double getDiscountFactor(const OptionChain& optionChain, double fContinuousRate,
        const DateUtils::ExchangeClose& exchangeClose = DateUtils::m_nasdaqClose);
    
    /// @brief Runs functor on all records
    /// @tparam T Return type of functor
    /// @param optionChain Option chain instance
    /// @param callback functor to call
    /// @return List of result values
    template <typename T>
    static std::list<T> onAllRecords(const OptionChain& optionChain,
        const std::function<T(const OptionChain::Record&)>& callback) {
        std::list<T> records;
        auto caller = [&records, &callback](const std::map<std::string, Record>& recordMap) {
            for (auto it = recordMap.begin(); it != recordMap.end(); ++it) {
                records.push_back(callback(it->second));
            }
        };
        caller(optionChain.m_callsStrikeKeyToRecord);
        caller(optionChain.m_putsStrikeKeyToRecord);
        return records;
    }
    /// @brief Runs a functor on all pairs of matching strike put and call records
    /// @tparam T Return type of functor
    /// @param optionChain Option chain instance
    /// @param callback The functor
    /// @param bOnlyValid Only run on fully valid pairs of records
    /// @param bRelaxedBidAskValid Only run on pairs of records with valid bid and ask
    /// @return Map of strike key to results
    template <typename T>
    static std::map<std::string, T> onAllPutCallRecords(const OptionChain& optionChain,
        const std::function<T(const RecordMap::value_type&, 
            const RecordMap::value_type&)>& callback,
        bool bOnlyValid = true, bool bRelaxedBidAskValid = false)
    {
        std::map<std::string, T> records;
        for (auto callIt = optionChain.m_callsStrikeKeyToRecord.begin(); 
                callIt != optionChain.m_callsStrikeKeyToRecord.end(); ++callIt) {
            auto putIt = optionChain.m_putsStrikeKeyToRecord.find(callIt->first);
            if (putIt != optionChain.m_putsStrikeKeyToRecord.end()) {
                if (!bOnlyValid 
                    || (bRelaxedBidAskValid && putIt->second.bidAskValid() && callIt->second.bidAskValid())
                    || (putIt->second.isValid() && callIt->second.isValid())) {
                    // both put and call are valid
                    records.insert({ callIt->first, callback(*putIt, *callIt) });
                }
            }
        }
        return records;
    }
    /// @brief Esimate slope and intercept of a least squares line fitted to the data series
    /// @param dataSeries (X, Y) pairs of data points
    /// @return pair of slope and intercept
    static std::pair<double, double> fitLeastSquaresLine(const std::list<std::pair<double, double>>& dataSeries);

    /// @brief Returns variance from sum of squared residuals of line fit to data series
    /// @param dataSeries 
    /// @param fittedLine 
    /// @return Variance
    static double computeVarianceAlongFittedLine(
        const std::list<std::pair<double, double>>& dataSeries,
        const std::pair<double, double>& fittedLine);



};

}
