#include "bentoclient/requestersynchronous.hpp"
#include "bentoclient/getter.hpp"
#include "bentoclient/retriever.hpp"
#include "bentoclient/persister.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/datagrid.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/marketenvironmentextended.hpp"
#include "bentoclient/clienttypes.hpp"
#include "bentoclient/logging.hpp"
#include "bentoclient/retry.hpp"
#include <boost/log/trivial.hpp>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <mutex>

#define STREAM_DEBUG 0

#if STREAM_DEBUG
#include <fstream>
#include "bentoclient/bentoserializer.hpp"
#endif
using namespace bentoclient;


/// @brief  Internal class removes databento dependencies from requester interface 
///         and thus applications submitting data requests
class RequesterSynchronous::Internal
{
public:
    Internal(){}

    const OptionInstruments& getOptionInstruments(Getter* getter, 
        const std::string& dataSet,
        const std::string& symbol,
        const std::string& date)
    {
        std::string key = makeKey(symbol, date);
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_instruments.find(key);
        if (it == m_instruments.end())
        {
            databento::SymbologyResolution symbologyResolution = 
                getter->getSymbologyResolution(dataSet, symbol, date);
#if STREAM_DEBUG
{
    std::ofstream ofs(symbol + "_symbologyResolution_" + date  + ".txt");
    boost::archive::text_oarchive oa(ofs);
    oa << symbologyResolution;
}
#endif
            it = m_instruments.emplace(key, 
                std::move(OptionInstruments{})).first;   
            it->second.insert(symbologyResolution);
        }
        return it->second;
    }
    static std::string makeKey(const std::string& symbol, const std::string& date)
    {
        return fmt::format("{}_{}", symbol, date);
    }

    static OptionChain::PutCallRecordMap getPutCallRecordMap(
        const RequesterSynchronous& requester,
        Timestamp dateTime,
        const std::map<std::string, std::string>& idToOsi
#if STREAM_DEBUG
        , const std::string& _symbol, const std::string& _date, const std::string& _expiryDate
#endif        
        )
    {
        // Due to issues with spotty data, get data from two cbbo schemata and join maps
        // In addition to spotty data, there is a databento limit on the size of 
        // request header, as well as response buffer. Example:
        // Received error for symbol QQQ: 
        // Zstd error decompressing: Operation made no progress over multiple calls, 
        // due to output buffer being full.
        // Aim for the response buffer to not receive more than like a couple of thousand records. 
        // Overflow errors may depend on other factors, so below value is experimental. It shouldn't be 
        // too low to not affect performance negatively. When the error happens, the record size is halved 
        // in retries.
        // So, for instance, if an initial size of 4000 turns out too ambitious, the next try will be
        // 2000, and then 1000, up to the max number of retries below.
        // Since data is metered, and may be charged at that depending on plan, a lower nMaxRecords 
        // help reducing overall costs, or server load on provider side.
        std::uint64_t nMaxRecords = 1600;
        std::uint64_t nMaxZstdBufferRetries = 3;

        // As the number of instruments decreases with each iteration, need firstly a functor to get a subset of idToOsi
        auto getSubIdToOsi = [](const std::map<std::string, std::string>& idToOsi, const std::vector<std::string>& filter){
            std::map<std::string, std::string> subMap;
            for (auto& id : filter)
            {
                auto it = idToOsi.find(id);
                if (it != idToOsi.end())
                subMap.insert(*it);
            }
            return subMap;
        };

        // the complete id map is initially missing.
        std::vector<std::string> missingInstrumentIds = AppUtils::keyVector(idToOsi);
        std::list<OptionChain::InstrumentIdToCbboMap> cbboMaps;
        // Second, the max number of records per instrument will increase, as the number of missing instruments 
        // decreases. Therefore, limits will be computed within a loop.
        auto minDivisor = [] (const TimeRange& timeRange) -> std::uint64_t {
            return timeRange / std::chrono::minutes(1);
        };
        auto secDivisor = [] (const TimeRange& timeRange) -> std::uint64_t {
            return timeRange / std::chrono::seconds(1);
        };

        // getter loop slizes required instruments (== missingInstrumentIds) into
        // slices that fit into http request and response buffers (hopefully)
        auto getterLoop = [getSubIdToOsi, &requester, &missingInstrumentIds, &idToOsi](
            Timestamp dateTime,
            TimeRange timeRange,
            databento::Schema schema,
            std::uint64_t nMaxRecords, 
            std::function<std::uint64_t(const TimeRange&)> divisor){
            std::list<OptionChain::InstrumentIdToCbboMap> localCbboMaps;
            while (missingInstrumentIds.size() > 0 && !(timeRange==TimeRange::zero()) )
            {
                std::uint64_t nMaxPerInstrument = nMaxRecords 
                    / std::min(requester.m_nInstrumentsSplit, missingInstrumentIds.size());
                std::uint64_t nExpectedPerInstrument = divisor(timeRange);
                std::uint64_t nSplit = nExpectedPerInstrument / nMaxPerInstrument + 1;
                TimeRange subRange = timeRange / nSplit;
                timeRange = timeRange - subRange;
                Timestamp at = dateTime;
                dateTime = dateTime - subRange;
                std::list<databento::CbboMsg> cbboMsgs = requester.m_getter->getCbboTimeseriesRange(
                    missingInstrumentIds,
                    requester.m_sDataset, 
                    schema, at, subRange);
                // check for missing instruments
                std::map<std::string, std::string> subIdToOsi = getSubIdToOsi(idToOsi,  missingInstrumentIds);
                OptionChain::InstrumentIdToCbboMap cbboMap = OptionChain::mapCbboMsgsToInstruments(
                    std::move(cbboMsgs), subIdToOsi);
                missingInstrumentIds = OptionChain::findInstrumentsMissingCbboMsgs(cbboMap, subIdToOsi);
                localCbboMaps.emplace_back(std::move(cbboMap));
            }
            return localCbboMaps;
        };

        // the getter retry loop handles errors related to exceeded buffer sizes
        auto getterRetryLoop = [getterLoop, nMaxRecords, nMaxZstdBufferRetries](
            Timestamp dateTime,
            TimeRange timeRange,
            databento::Schema schema,
            std::function<std::uint64_t(const TimeRange&)> divisor){
            std::uint64_t nMaxRecordsInLoop = nMaxRecords;
            std::uint64_t nRetryCount = 0;
            while (true) {
                try {
                    return getterLoop(dateTime, timeRange, schema, nMaxRecordsInLoop, divisor);
                } catch (const std::exception& e)
                {
                    if (Retry::isZstdBufferOverflow(e) && nRetryCount++ < nMaxZstdBufferRetries)
                    {
                        nMaxRecordsInLoop /= 2;
                        BOOST_LOG_TRIVIAL(warning) << "Attempting error recovery for getter loop with max records reduced to "
                            << nMaxRecordsInLoop << " from initial " << nMaxRecords;
                    } else {
                        throw;
                    }
                }
            }            
        };

        cbboMaps.splice(cbboMaps.end(), 
            getterRetryLoop(dateTime, requester.m_cbbo1sRange, databento::Schema::Cbbo1S, secDivisor));
        
        // the 1m data is useful estimating the movements of the underlier from put/call parity
        // and allows estimating the delta of options along the strike axis. This in turn allows
        // fitting past results into the snapshot at {dateTime}.
        // However, using a delta estimate to shift out of date cbbo records to their probable
        // value at {dateTime} is not necessarily an improvement, depending on user needs.
        BOOST_LOG_TRIVIAL(info) << "Missing instruments number " << missingInstrumentIds.size() 
            << " after cbbo1s run. Example: " 
            << (missingInstrumentIds.empty() ? "None" : idToOsi.at(missingInstrumentIds[0]));

        // trace logger as a development aid with spotty option chain instruments
        auto traceLogger = [&idToOsi](const std::vector<std::string> missingIds, const std::string& run) {
            std::ostringstream ostr;
            ostr << "Missing instruments after " << run << " run details" << std::endl;
            std::map<std::string, std::string> osiToMissing;
            for (auto& missingId : missingIds) {
                auto osiIt = idToOsi.find(missingId);
                osiToMissing[(osiIt == idToOsi.end() ? missingId : osiIt->second)] = missingId;
            }
            for (auto it = osiToMissing.begin(); it != osiToMissing.end(); ++it) {
                ostr << "   " << it->first << ":" << it->second << std::endl;
            }
            BOOST_LOG_TRIVIAL(trace) << ostr.str();
        };
        if (logging::trace_logging_enabled())
        {
            traceLogger(missingInstrumentIds, "databento::Schema::Cbbo1S");
        }

        // Requesting a full set of 1m records for {dateTime} may improve results even if 
        // no advanced estimations happens here, like estimating delta to shift older records.
        // However, the extra data load is probably not justified.
        // missingInstrumentIds = AppUtils::keyVector(idToOsi);
        
        // Get 1m data
        cbboMaps.splice(cbboMaps.end(),
            getterRetryLoop(dateTime, requester.m_cbbo1mRange, databento::Schema::Cbbo1M, minDivisor));
        BOOST_LOG_TRIVIAL(info) << "Missing instruments number " << missingInstrumentIds.size() 
            << " after cbbo1m run. Example: " 
            << (missingInstrumentIds.empty() ? "None" : idToOsi.at(missingInstrumentIds[0]));
        if (logging::trace_logging_enabled())
        {
            traceLogger(missingInstrumentIds, "databento::Schema::Cbbo1M");
        }

        OptionChain::InstrumentIdToCbboMap cbboMap = joinCbboMaps(std::move(cbboMaps));
#if STREAM_DEBUG
{
    std::ofstream ofs(_symbol + "_cbboMap_" + _date + "_exp_" + _expiryDate + ".txt");
    boost::archive::text_oarchive oa(ofs);
    oa << cbboMap;
}
#endif
        // At this points, CBBO map is as good as it gets. In order to find the most relevant data,
        // first reshuffle to a timeline of 2 second buckets
        OptionChain::RecordTimeline timeline = OptionChain::buildRecordTimeline(cbboMap,
            idToOsi, std::chrono::seconds(2));
        // Based on this timeline, one *could* now attempt a put-call-parity analysis, and then
        // request a larger timeframe of ATM instruments to compute a pcp-rate history for shifting
        // of out-of-date elements. For now, just produce the RecordMaps for the raw option chain
        // from the last-best values in the timeline. Otherwise, put another call of the getterRetryLoop here...

        OptionChain::PutCallRecordMap putCallRecords = OptionChain::mapLatestBestInTimelineToRecord(timeline);
        return putCallRecords;
    }

    static OptionChain::InstrumentIdToCbboMap joinCbboMaps(std::list<OptionChain::InstrumentIdToCbboMap>&& cbboMaps)
    {
        OptionChain::InstrumentIdToCbboMap joined;
        for (auto rit = cbboMaps.rbegin(); rit != cbboMaps.rend(); ++rit)
        {
            for (auto mit = rit->begin(); mit != rit->end(); ++mit)
            {
                auto& cbboList = joined[mit->first];
                cbboList.splice(cbboList.end(), mit->second);
            }
        }
        return joined;
    }


private:
    std::map<std::string, OptionInstruments> m_instruments;
    std::mutex m_mutex;
};

RequesterSynchronous::RequesterSynchronous(std::unique_ptr<Getter>&& getter, 
    std::unique_ptr<Retriever>&& retriever,
    std::unique_ptr<Persister>&& persister,
    const std::string& sDataset,
    TimeRange cbbo1sRange,
    TimeRange cbbo1mRange,
    std::uint64_t nInstrumentsSplit,
    double fDefaultRiskFreeRate,
    const std::string& sInterestRatesCsv
    ) :
    Requester(std::move(getter), std::move(retriever), std::move(persister)),
    m_internal(std::make_unique<Internal>()),
    m_marketEnvironment(std::make_shared<MarketEnvironmentExtended>(
        fDefaultRiskFreeRate,
        DateUtils::m_nasdaqClose,
        sInterestRatesCsv)),
    m_terminateSignal([](){return false;}),
    m_cbbo1sRange(cbbo1sRange),
    m_cbbo1mRange(cbbo1mRange),
    m_sDataset(sDataset),
    m_nInstrumentsSplit(nInstrumentsSplit)
{
}

RequesterSynchronous::~RequesterSynchronous()
{}

Requester::JobId RequesterSynchronous::requestOptionChains(const std::string& symbol, 
    const Timestamp& dateTime, int nDte)
{
    if (!m_terminateSignal()) {
        getOptionChains(symbol, dateTime, nDte);
    }
    return 0;
}

void RequesterSynchronous::getOptionChains(const std::string& symbol, 
    const Timestamp& dateTime, int nDte)
{
    // TODO: marketEnvironments per symbol should distinguish yield curves and exchange close
    m_retriever->submitMarketEnvironment(symbol, m_marketEnvironment);
    std::string date = fmt::format(DataGrid::Format::makeFmtString(DataGrid::m_defaultDateFormat), dateTime);
    const OptionInstruments& optionInstruments =
        m_internal->getOptionInstruments(m_getter.get(), m_sDataset, symbol, date);
    if (m_terminateSignal()) {
        return;
    }
    std::list<std::string> expiryDates = optionInstruments.getExpiryDatesForDTE(symbol, date, nDte);
    if (expiryDates.empty() || 
        (expiryDates.size() == 1 && expiryDates.front() == date)) {
        expiryDates = optionInstruments.getNextExpiryDate(symbol, date);
    }
    BOOST_LOG_TRIVIAL(info) << "Found expiry dates " << AppUtils::joinList(expiryDates) << " for symbol " << symbol 
        << " at " << serializeTimestamp(dateTime) << " and " << nDte << " days to expiration";

    std::list<std::pair<Timestamp, std::string>> chainTimes;
    std::list<std::pair<Timestamp, std::string>> missingChains;
    for (auto& expiryDate : expiryDates)
    {
        if (m_terminateSignal()) {
            BOOST_LOG_TRIVIAL(warning) << "Quitting for symbol " << symbol << " and expiry date " << expiryDate
                << " after terminateSignal";
            break;
        }

        OptionInstruments specificDateInstruments = optionInstruments.get(symbol, date, expiryDate);
        std::map<std::string, std::string> idToOsi = specificDateInstruments.getInstrumentIdToOsiMap();
        std::vector<std::string> instrumentIds = AppUtils::keyVector(idToOsi);
        BOOST_LOG_TRIVIAL(info) << "Getting CBBOs for symbol " << symbol << " and expiry date " 
            << expiryDate;
        OptionChain::PutCallRecordMap putCallRecordMap = Internal::getPutCallRecordMap(
            *this, dateTime, idToOsi
#if STREAM_DEBUG
            , symbol, date, expiryDate
#endif        
        );

        BOOST_LOG_TRIVIAL(info) << "Starting to build option chain from CBBO and instrument data for symbol " << symbol
            << " and expiry date " << expiryDate;
        try {
            OptionChain rawChain(OptionChain::build(std::move(putCallRecordMap), specificDateInstruments));
            if (rawChain.isValid())
            {
                BOOST_LOG_TRIVIAL(info) << "Built raw chain for symbol " << symbol << " and expiry date " 
                    << expiryDate;
                chainTimes.push_back({rawChain.getChainTime(), expiryDate});
                m_retriever->submitOptionChain(std::move(rawChain));
            } else {
                BOOST_LOG_TRIVIAL(warning) << "Missing chain data for symbol " << symbol << " at " 
                    << serializeTimestamp(dateTime) << " for expiry date " << expiryDate;
                missingChains.push_back({dateTime, expiryDate});
            }
        } catch (const std::exception& e)
        {
            BOOST_LOG_TRIVIAL(error) << "Failed to build option chain for symbol " << symbol << " at " 
                << serializeTimestamp(dateTime) << " and expiry date " << expiryDate << ": " << e.what();
            missingChains.push_back({dateTime, expiryDate});
        }
    }
    for (auto chainPair : chainTimes)
    {
        try {
            OptionChain enhancedChain = m_retriever->getOptionChain(symbol, chainPair.first, chainPair.second);
            BOOST_LOG_TRIVIAL(info) << "Persisting enhanced chain for symbol " << symbol << " at " 
                << serializeTimestamp(dateTime) << " and expiry date " << enhancedChain.getExpiryDate();
            m_persister->persist(std::move(enhancedChain),
                m_retriever->getMarketEnvironment(symbol));
        } catch (const std::exception& e)
        {
            BOOST_LOG_TRIVIAL(error) << "Failed to retrieve enhanced chain and persist it for symbol " << symbol << " at "
                << serializeTimestamp(dateTime) << " for expiry date " << chainPair.second << ": " << e.what();
            missingChains.push_back(chainPair);
        }
    }
    if (!missingChains.empty())
    {
        m_persister->persistMissing(symbol, date, std::move(missingChains));
    }
}

void RequesterSynchronous::setTerminateSignal(std::function<bool()> terminateSignal)
{
    m_terminateSignal = terminateSignal;
}
