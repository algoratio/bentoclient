#include "bentoclient/requesterasynchronous.hpp"
#include "bentoclient/getterasynchronous.hpp"
#include "bentoclient/gettersynchronous.hpp"
#include "bentoclient/retrieverinmemory.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/persistercsv.hpp"
#include <boost/log/trivial.hpp>

using namespace bentoclient;

RequesterAsynchronous::RequesterAsynchronous(
    std::unique_ptr<Getter>&& getter, 
    std::unique_ptr<Retriever>&& retriever,
    std::unique_ptr<Persister>&& persister,
    const std::string& sDataset,
    TimeRange cbbo1sRange,
    TimeRange cbbo1mRange,
    std::uint64_t nInstrumentsSplit,
    std::uint64_t nThreads,
    std::function<bool()> terminateSignal,
    double fDefaultRiskFreeRate,
    const std::string& sInterestRatesCsv
    ) :
    RequesterSynchronous(
        std::move(getter),
        std::move(retriever),
        std::move(persister),
        sDataset,
        cbbo1sRange,
        cbbo1mRange,
        nInstrumentsSplit,
        fDefaultRiskFreeRate,
        sInterestRatesCsv
        ),
        m_threadPool(nThreads)
{
    this->setTerminateSignal(terminateSignal);
}

RequesterAsynchronous::~RequesterAsynchronous()
{
}

Requester::JobId RequesterAsynchronous::requestOptionChains(const std::string& symbol, 
    const bentoclient::Timestamp& dateTime, int nDte)
{
    JobId id(0);
    if (!m_terminateSignal())
    {
        id = m_threadPool.post([this, &symbol, dateTime, nDte](){
            this->getOptionChains(symbol, dateTime, nDte);
        });
    }
    else
    {
        BOOST_LOG_TRIVIAL(warning) << "Skipping option chain request for " << symbol << " at " 
            << serializeTimestamp(dateTime) << " due to terminateSignal";
    }
    return id;
}

ThreadPool::ResultMap RequesterAsynchronous::query()
{
    return m_threadPool.query();
}


std::unique_ptr<RequesterAsynchronous> RequesterAsynchronous::makeRequesterCSV(
    const std::string& sApiKey,
    const std::string& sDataset,
    const std::string& sBasePath,
    std::uint64_t nThreadsRequester,
    std::uint64_t nThreadsSymbology,
    std::uint64_t nThreadsTimeseries,
    std::function<bool()> terminateSignal,
    std::uint64_t nSplitInstrumentIds,
    std::uint64_t nRetries,
    TimeRange chainLookupTimeRange,
    TimeRange cbbo1sTimeRange,
    TimeRange cbbo1mTimeRange,
    double fDefaultRiskFreeRate,
    const std::string& sInterestRatesCsv,
    bool bStacked,
    bool bDateDirs)
{
    std::unique_ptr<databento::Historical> clientPtr(std::make_unique<databento::Historical>(
        std::move(databento::HistoricalBuilder{}
        .SetKey(sApiKey)
        .Build())));
    
    std::unique_ptr<Getter> _getterPtr = std::make_unique<GetterSynchronous>(
        std::move(clientPtr));

    std::unique_ptr<Getter> getterPtr = std::make_unique<GetterAsynchronous>(
        std::move(_getterPtr),
        nThreadsSymbology,
        nThreadsTimeseries,
        nSplitInstrumentIds,
        nRetries);

    std::unique_ptr<Retriever> retrieverPtr = std::make_unique<RetrieverInMemory>(
        chainLookupTimeRange
    );

    std::unique_ptr<Persister> persisterPtr = std::make_unique<PersisterCSV>(
        sBasePath,
        bDateDirs,
        bStacked? PersisterCSV::CSVFormat::Stacked : PersisterCSV::CSVFormat::SideBySide
    );

    std::unique_ptr<RequesterAsynchronous> requesterPtr = std::make_unique<RequesterAsynchronous>(
        std::move(getterPtr),
        std::move(retrieverPtr),
        std::move(persisterPtr),
        sDataset,
        cbbo1sTimeRange,
        cbbo1mTimeRange,
        nSplitInstrumentIds,
        nThreadsRequester,
        terminateSignal,
        fDefaultRiskFreeRate,
        sInterestRatesCsv
    );
    return requesterPtr;
}
