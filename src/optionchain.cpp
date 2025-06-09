#include "bentoclient/optionchain.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/osioption.hpp"
#include <boost/log/trivial.hpp>
#include <fmt/core.h>
#include <tuple>
#include <cmath>

using namespace bentoclient;

class OptionChain::Algos{
public:
    static void mapCbboMsgsToInstrumentId(std::list<databento::CbboMsg>& cbboSource, 
        std::map<std::string, std::list<databento::CbboMsg>>& cbboMap, 
        const std::map<std::string, std::string>& instrumentIdToOsiMap)
    {
        for (auto sourceIt = cbboSource.begin(); sourceIt != cbboSource.end(); ) {
            auto instrumentId = std::to_string(sourceIt->hd.instrument_id);
            if (instrumentIdToOsiMap.find(instrumentId) == instrumentIdToOsiMap.end()) {
                // Skip the cbbo message if the instrument ID is not found in the mapping
                ++sourceIt;
                continue;
            }
            auto& instrumentList = cbboMap[instrumentId];
            auto instrumentRecord = sourceIt++;
            instrumentList.splice(instrumentList.end(), cbboSource, instrumentRecord);
        }
    }
};

const std::uint64_t OptionChain::Record::priceScaling = 1000000000;
OptionChain::Record::Record() :
    m_price(std::nan("0xbad"), 0),
    m_priceTime{},
    m_askPrice(std::nan("0xbad"), 0),
    m_bidPrice(std::nan("0xbad"), 0),
    m_recvTime{},
    m_comment{}
{
}

OptionChain::Record::Record(
    PriceWeight&& price,
    Timestamp&& priceTime,
    PriceWeight&& ask,
    PriceWeight&& bid,
    Timestamp&& recvTime) :
    m_price(std::move(price)),
    m_priceTime(std::move(priceTime)),
    m_askPrice(std::move(ask)),
    m_bidPrice(std::move(bid)),
    m_recvTime(std::move(recvTime)),
    m_comment{}
{
}

OptionChain::Record::Record(const databento::CbboMsg& cbboMsg) :
    m_price(static_cast<double>(cbboMsg.price)/priceScaling, cbboMsg.size),
    m_priceTime(cbboMsg.hd.ts_event),
    m_askPrice(static_cast<double>(cbboMsg.levels[0].ask_px)/priceScaling, cbboMsg.levels[0].ask_sz),
    m_bidPrice(static_cast<double>(cbboMsg.levels[0].bid_px)/priceScaling, cbboMsg.levels[0].bid_sz),
    m_recvTime(cbboMsg.ts_recv),
    m_comment{}
{

}

bool OptionChain::Record::operator == (const Record& other) const
{
    if (this == &other)
        return true;
    return m_price == other.m_price
        && m_priceTime == other.m_priceTime
        && m_askPrice == other.m_askPrice
        && m_bidPrice == other.m_bidPrice
        && m_recvTime == other.m_recvTime
        && m_comment == other.m_comment;
}


OptionChain::InstrumentIdToCbboMap
OptionChain::mapCbboMsgsToInstruments(std::list<databento::CbboMsg>&& cbboMsgs,
    const std::map<std::string, std::string>& instrumentIdToOsiMap)
{
    // take ownership of the cbbo messages
    std::list<databento::CbboMsg> cbboSource(std::move(cbboMsgs));
    // now shuffle cbbo messages into lists per instrument ID
    InstrumentIdToCbboMap cbboMap;
    Algos::mapCbboMsgsToInstrumentId(cbboSource, cbboMap, instrumentIdToOsiMap);
    return cbboMap;
}

OptionChain::RecordTimeline OptionChain::buildRecordTimeline(const InstrumentIdToCbboMap& cbboMap,
    const std::map<std::string, std::string>& idToOsi,
    TimeRange slotWindow)
{
    auto slotTime = [slotWindow](const Timestamp& timestamp)
    {
        auto since_epoch = timestamp.time_since_epoch();
        std::uint64_t slot_count = since_epoch / slotWindow;
        auto truncated = slot_count * slotWindow;
        return Timestamp(truncated);
    };
    // The time line works on strike keys rather than instrument IDs to allow for more advanced
    // analysis and modification of stale data. For illiquid instruments, any data available
    // could be hours old, and having a timeline on strike keys could allow for an assessment
    // of options delta, as well as put-call-parity compatible rates, to shift out of time data
    // to according to changes of the underlier and estimates of delta. This is however optional,
    // and may, depending of user needs, not improve results. Still, since it's relatively simple
    // to restore the instrument mappings, the timeline implements this complication.
    // The output matches the structure of OptionChain internals, and therefore also effects
    // a simplification later on. Mappings to instruments can easily be restored from matching
    // OptionInstruments data.
    
    RecordTimeline timeline;
    for (auto instrumentIt = cbboMap.begin(); instrumentIt != cbboMap.end(); ++instrumentIt)
    {
        auto osiIt = idToOsi.find(instrumentIt->first);
        if (osiIt == idToOsi.end())
        {
            BOOST_LOG_TRIVIAL(error) << "Missing OSI mapping in buildRecordTimeline for instrument " << instrumentIt->first;
            continue;
        }
        OsiOption osiOpt(osiIt->second);
        for (auto cbboListIt = instrumentIt->second.begin(); cbboListIt != instrumentIt->second.end(); 
            ++cbboListIt)
        {
            OptionChain::Record optionRecord(*cbboListIt);
            if (optionRecord.anyBidAskValid())
            {
                Timestamp recordSlot = slotTime(optionRecord.m_recvTime);
                RecordTimeline::mapped_type& putCallRecordMap = timeline[recordSlot];
                RecordMap& recordMap = osiOpt.isPut() ?
                    putCallRecordMap.first : putCallRecordMap.second;
                auto recordPair = recordMap.emplace(osiOpt.getStrikeKey(), std::move(optionRecord));
                if (recordPair.second == false)
                {
                    // a previous record for this timeslot exists. Check if it should be replaced.
                    auto& prev = recordPair.first->second;
                    // create new record as previous one moved
                    OptionChain::Record potentialOverwrite(*cbboListIt);
                    if ((potentialOverwrite.bidAskValid() 
                        && (!prev.bidAskValid() || potentialOverwrite.m_recvTime > prev.getRecvTime()))
                        ||
                        (potentialOverwrite.anyBidAskValid()
                        && (!prev.anyBidAskValid() || potentialOverwrite.m_recvTime > prev.getRecvTime()))) {
                        // if more than one candidate for this time and value, put newest or one with
                        // more information
                        prev = std::move(potentialOverwrite);
                    }
                }
            }
        }
    }
    return timeline;
}

OptionChain::PutCallRecordMap OptionChain::mapLatestBestInTimelineToRecord(
    const RecordTimeline& timeline
)
{
    PutCallRecordMap putCallMap;
    auto recordMapper = [](RecordMap& recordMap, const RecordMap& recordMapAtTimePoint){
        for (auto strikeIt = recordMapAtTimePoint.begin(); strikeIt != recordMapAtTimePoint.end(); ++strikeIt)
        {
            auto recordPair = recordMap.insert(*strikeIt);
            if (recordPair.second == false)
            {
                // a previous record for this timeslot exists. Check if it should be replaced.
                auto& prev = recordPair.first->second;
                // create new record as previous one moved
                const OptionChain::Record& potentialOverwrite(strikeIt->second);
                if ((potentialOverwrite.bidAskValid() 
                    && (!prev.bidAskValid() || potentialOverwrite.m_recvTime > prev.getRecvTime()))
                    ||
                    (potentialOverwrite.anyBidAskValid()
                    && (!prev.anyBidAskValid() || potentialOverwrite.m_recvTime > prev.getRecvTime()))) {
                    // if more than one candidate for this time and value, put newest or one with
                    // more information
                    prev = potentialOverwrite;
                }
            }
        }
    };
    for (auto timelineIt = timeline.begin(); timelineIt != timeline.end(); ++timelineIt)
    {
        // From the timeline, put the oldest data first. If more complete record data
        // is available at later time points, it will overwrite data in the output map.
        recordMapper(putCallMap.first, timelineIt->second.first);
        recordMapper(putCallMap.second, timelineIt->second.second);
    }
    return putCallMap;
}

std::vector<std::string>
OptionChain::findInstrumentsMissingCbboMsgs(const InstrumentIdToCbboMap& cbboMap,
    const std::map<std::string, std::string>& instrumentIdToOsiMap)
{
    std::vector<std::string> missingInstruments;
    for (auto it = instrumentIdToOsiMap.begin(); it != instrumentIdToOsiMap.end(); ++it)
    {
        auto cbboIt = cbboMap.find(it->first);
        if (cbboIt == cbboMap.end())
        {
            missingInstruments.push_back(it->first);
        }
        else
        {
            // check if we have at least one valid bid/ask pair in the data and a last trade size
            // which is a strict test aiming to improve data quality by requesting
            // minutely data in the case. For records with good data, no need to get back to
            // the minutely fallback. 
            bool foundValid = false;
            auto& valueList = cbboIt->second;
            for(auto vit = valueList.begin(); foundValid == false && vit != valueList.end(); ++vit)
            {
                //if (vit->size > 0)
                //{
                    for (auto& bidAskPair : vit->levels)
                    {
                        if (bidAskPair.ask_sz > 0 && bidAskPair.bid_sz > 0)
                        {
                            foundValid = true;
                            break;
                        }
                    }
                //}
            }
            if (!foundValid)
            {
                missingInstruments.push_back(it->first);
            }
        }
    }
    return missingInstruments;
}

OptionChain OptionChain::build(PutCallRecordMap&& recordMaps,
    const OptionInstruments& optionInstruments)
{
    OptionChain optionChain;
    optionChain.m_underlier = optionInstruments.getUnderlier();
    optionChain.m_valuationDate = optionInstruments.getValuationDate();
    optionChain.m_expiryDate = optionInstruments.getExpiryDate();
    // Initialize the option chain's record maps directly from input
    optionChain.m_putsStrikeKeyToRecord = std::move(recordMaps.first);
    optionChain.m_callsStrikeKeyToRecord = std::move(recordMaps.second);
    const OptionInstruments::StrikeKeyPutCallMap& strikeKeyPutCallMap(
        optionInstruments.getStrikeKeyPutCallMap());
    std::map<std::string, std::string> putStrikeToInstrument(
        OptionInstruments::makeStrikeKeyToInstrumentIdMap(strikeKeyPutCallMap.first));
    std::map<std::string, std::string> callStrikeToInstrument(
        OptionInstruments::makeStrikeKeyToInstrumentIdMap(strikeKeyPutCallMap.second));
    std::map<std::string, std::string> idToOsiMap = optionInstruments.getInstrumentIdToOsiMap();
    auto clearFilledInstruments = [&idToOsiMap](
        const std::map<std::string, std::string>& strikeToInstrument,
        const RecordMap& recordMap
    )
    {
        for (auto recordIt = recordMap.begin(); recordIt != recordMap.end(); ++recordIt)
        {
            auto instrumentIt = strikeToInstrument.find(recordIt->first);
            if (instrumentIt != strikeToInstrument.end())
            {
               auto idToOsiIt = idToOsiMap.find(instrumentIt->second);
               if (idToOsiIt != idToOsiMap.end())
                idToOsiMap.erase(idToOsiIt); 
            }
        }
    };
    clearFilledInstruments(putStrikeToInstrument, optionChain.m_putsStrikeKeyToRecord);
    clearFilledInstruments(callStrikeToInstrument, optionChain.m_callsStrikeKeyToRecord);
    // Document the still missing / faulty instrument IDs
    optionChain.m_missingInstrumentIdToOsiMap = std::move(idToOsiMap);
    // Fill the missing instruments with empty records. In option chain printouts, if 
    // nothing else can estimate them, they will appear as nan, {null} or similar.
    for (auto osiIt = optionChain.m_missingInstrumentIdToOsiMap.begin(); 
        osiIt != optionChain.m_missingInstrumentIdToOsiMap.end(); ++osiIt)
    {
        OsiOption osiOpt(osiIt->second);
        RecordMap& target = osiOpt.isPut() ? 
            optionChain.m_putsStrikeKeyToRecord : optionChain.m_callsStrikeKeyToRecord;
        auto pair = target.insert(std::make_pair(osiOpt.getStrikeKey(), Record()));
        if (!pair.second)
        {
            BOOST_LOG_TRIVIAL(error) << "Not blanking existing data in OptionChain::build for strike key "
                << osiOpt.getStrikeKey() << " OSI id " << osiIt->second;
        }
    }
    return optionChain;
}

Timestamp OptionChain::getChainTime() const
{
    std::function<bentoclient::Timestamp(const bentoclient::OptionChain::Record&)> getRecvTime =
    [](const bentoclient::OptionChain::Record& record) {
        return record.m_recvTime;
    };
    std::list<bentoclient::Timestamp> recvTimes =
        Util::onAllRecords(*this, getRecvTime);
    
    if (recvTimes.empty()) {
        return Timestamp{};
    }

    auto maxIt = std::max_element(recvTimes.begin(), recvTimes.end());
    return *maxIt;
}

Timestamp OptionChain::getExpiryTime(const DateUtils::ExchangeClose& exchangeClose) const
{
    int year = std::stoi(m_expiryDate.substr(0, 4));
    int month = std::stoi(m_expiryDate.substr(5, 2));
    int day = std::stoi(m_expiryDate.substr(8, 2));
    return DateUtils::makeTimestamp(year, month, day, 
        exchangeClose.m_hour, exchangeClose.m_minute, 0, exchangeClose.m_timeZone);
}

/// @brief Get the put-call parity rate consistent with put/call records
double OptionChain::getParityRate(double fRiskFreeRate, 
    const DateUtils::ExchangeClose& exchangeClose) const
{
    double discountFactor = Util::getDiscountFactor(*this, fRiskFreeRate, exchangeClose);
    Util::PutCallParityRate putCallParityRate(discountFactor);
    std::function<double(const RecordMap::value_type&, const RecordMap::value_type&)> parityRate = putCallParityRate;
    auto parityCalculator = [this,&parityRate](bool bRelaxedValid) {
        auto parityRates = Util::onAllPutCallRecords(*this, parityRate, true, bRelaxedValid);
        // compute average of parity rates
        double sum = 0.0;
        for (auto& pair : parityRates) {
            sum += pair.second;
        }
        double avgParityRate = sum / parityRates.size();
        // the overall average is likely not the best consistent parity rate. Better take
        // four values around that average and compute the average of those.
        std::string parityKey = OsiOption::toStrikeKey(avgParityRate);
        auto upperBound = parityRates.upper_bound(parityKey);
        auto lowerBound = upperBound;
        for (int i = 0; i < 2; ++i) {
            if (lowerBound != parityRates.begin()) {
                --lowerBound;
            }
        }
        for (int i = 0; i < 2; ++i) {
            if (upperBound != parityRates.end()) {
                ++upperBound;
            }
        }
        double avg4ParityRate = 0.0;
        int count = 0;
        for (auto it = lowerBound; it != upperBound; ++it) {
            avg4ParityRate += it->second;
            ++count;
        }
        if (count == 0) {
            throw std::invalid_argument("No valid parity rates found.");
        }
        avg4ParityRate /= count;
        return avg4ParityRate;
    };
    try {
        // first try with strictly valid records
        return parityCalculator(false);
    } catch(const std::invalid_argument& e)
    {
        BOOST_LOG_TRIVIAL(warning) << "Failed to compute parity rate for symbol " << m_underlier << " at valuation date " << m_valuationDate
            << " and expiry date " << m_expiryDate << ", retry with relaxed validity, error cause: " << e.what();
        try {
            double parityRate = parityCalculator(true);
            BOOST_LOG_TRIVIAL(info) << "Succeeded to compute parity rate " << parityRate << " for " 
                << m_underlier << "/" << m_expiryDate << " on retry";
            return parityRate;
        } catch (const std::invalid_argument& e)
        {
            BOOST_LOG_TRIVIAL(error) << "Failed to compute parity rate for symbol " << m_underlier << " at valuation date " << m_valuationDate
                << " and expiry date " << m_expiryDate << " on retry with relaxed validity, error cause: " << e.what();
            throw std::runtime_error(fmt::format("Failed to compute parity rate for symbol {} valuation date {} expiry date {}, root cause {}",
                m_underlier, m_valuationDate, m_expiryDate, e.what()));
        }
    }
}
double OptionChain::getParityRateQualityScore(double fRiskFreeRate, 
    const DateUtils::ExchangeClose& exchangeClose) const
{
    double discountFactor = Util::getDiscountFactor(*this, fRiskFreeRate, exchangeClose);
    Util::PutCallParityRate putCallParityRate(discountFactor);
    std::function<double(const RecordMap::value_type&, const RecordMap::value_type&)> parityRate = putCallParityRate;
    auto parityRates = Util::onAllPutCallRecords(*this, parityRate, true);
    std::list<std::pair<double, double>> parityRatesByStrike;
    for (auto& pair : parityRates) {
        double strike = OsiOption::fromStrikeKey(pair.first);
        double parityRate = pair.second;
        parityRatesByStrike.emplace_back(strike, parityRate);
    }
    std::pair<double, double> line = Util::fitLeastSquaresLine(parityRatesByStrike);
    double variance = Util::computeVarianceAlongFittedLine(
        parityRatesByStrike, line);
    return variance;
}
double OptionChain::Util::PutCallParityRate::operator()(const std::pair<const std::string, OptionChain::Record>& put,
    const std::pair<const std::string, OptionChain::Record>& call) const
{
    double putPrice = put.second.getMidPrice();
    double callPrice = call.second.getMidPrice();
    double strike = OsiOption::fromStrikeKey(call.first);
    // Put/Call Parity: P + S = C + K * e^(-rT)
    double S = callPrice - putPrice + strike * m_discountFactor;
    return S;
}

double OptionChain::Util::getDiscountFactor(const OptionChain& optionChain, double fContinuousRate,
    const DateUtils::ExchangeClose& exchangeClose)
{
    Timestamp chainTime = optionChain.getChainTime();
    Timestamp expiryTime = optionChain.getExpiryTime(exchangeClose);
    // Ensure expiryTime is after chainTime
    if (expiryTime < chainTime) {
        throw std::invalid_argument("Expiry time must be after chain time.");
    }
    // Calculate the duration between the two timestamps in seconds
    auto duration = expiryTime - chainTime;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

    // Define the number of seconds in a year (365.25 days to account for leap years)
    constexpr double secondsInYear = 365.25 * 24 * 60 * 60;

    // Calculate the year fraction
    double yearFraction = static_cast<double>(seconds) / secondsInYear;
    double discountFactor = std::exp(-fContinuousRate * yearFraction);
    return discountFactor;
}


std::pair<double, double> OptionChain::Util::fitLeastSquaresLine(
    const std::list<std::pair<double, double>>& dataSeries) 
{
    // Ensure there are enough points to fit a line
    if (dataSeries.size() < 2) {
        throw std::invalid_argument("Not enough data points to fit a line.");
    }

    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    int N = 0;

    // Iterate through the data points
    for (const auto& pair : dataSeries) {
        double x = pair.first;  // e.g. a Strike price
        double y = pair.second; // e.g. a Parity rate
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
        ++N;
    }

    // Calculate the slope (m) and intercept (b)
    double denominator = N * sumX2 - sumX * sumX;
    if (std::abs(denominator) < 1e-10) {
        throw std::runtime_error("Denominator is too small, cannot fit a line.");
    }

    double slope = (N * sumXY - sumX * sumY) / denominator;
    double intercept = (sumY - slope * sumX) / N;

    return {slope, intercept};
}

double OptionChain::Util::computeVarianceAlongFittedLine(
    const std::list<std::pair<double, double>>& dataSeries,
    const std::pair<double, double>& fittedLine)
{
    // Ensure there are enough points to compute variance
    if (dataSeries.empty()) {
        throw std::invalid_argument("No data points to compute variance.");
    }

    double slope = fittedLine.first;    // m
    double intercept = fittedLine.second; // b

    double sumSquaredDifferences = 0.0;
    int N = 0;

    // Iterate through the data points
    for (const auto& pair : dataSeries) {
        double x = pair.first;  // Strike price
        double y = pair.second; // Parity rate

        // Predicted y value from the fitted line
        double predictedY = slope * x + intercept;

        // Compute the squared difference
        double difference = y - predictedY;
        sumSquaredDifferences += difference * difference;

        ++N;
    }

    // Compute the variance
    return sumSquaredDifferences / N;
}