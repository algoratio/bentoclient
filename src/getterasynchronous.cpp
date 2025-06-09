#include "bentoclient/getterasynchronous.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/retry.hpp"
#include <boost/log/trivial.hpp>
#include <fmt/core.h>
#include <fmt/chrono.h>

using namespace bentoclient;


GetterAsynchronous::GetterAsynchronous(std::unique_ptr<Getter>&& getter,
            std::uint64_t nSymbologyThreads,
            std::uint64_t nTimeseriesThreads,
            std::uint64_t nInstrumentsSplit,
            std::uint64_t nRetries) :
    m_getter(std::move(getter)),
    m_symbologyPool(nSymbologyThreads),
    m_timeseriesPool(nTimeseriesThreads),
    m_nInstrumentsSplit(nInstrumentsSplit),
    m_nRetries(nRetries)
{}

GetterAsynchronous::~GetterAsynchronous()
{}

databento::SymbologyResolution GetterAsynchronous::getSymbologyResolution(
    const std::string& dataSet,
    const std::string& sUnderlier, const std::string& sDate)
{
    std::function<databento::SymbologyResolution(
        const std::string&, 
        const std::string&, 
        const std::string&
    )> symbologyFunc = 
        [this](
            const std::string& dataSet, 
            const std::string& underlier, 
            const std::string& date) 
        {
            return m_getter->getSymbologyResolution(dataSet, underlier, date);
        };
    
    Retry retry(m_nRetries);
    std::function<databento::SymbologyResolution()> toPost = 
    [this, &symbologyFunc, &dataSet, &sUnderlier, &sDate]() {
        // push on pool to enforce rate limit
        auto future = this->m_symbologyPool.post(symbologyFunc, dataSet, sUnderlier, sDate);
        return future.get();
    };
    Retry::ErrorLogger loggerFunc = [&sUnderlier, &sDate](std::uint64_t nTry, const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "getSymbologyResolution retry[" << nTry << "] for "
            << sUnderlier << ", " << sDate << " after " << e.what(); 
    };
    return retry.run(toPost, loggerFunc);
}

std::list<databento::CbboMsg> GetterAsynchronous::getCbboTimeseriesRange(
    const std::vector<std::string>& instrumentIds,
    const std::string& dataSet,
    databento::Schema schema,
    Timestamp at,
    TimeRange timeRange)
{
    std::function<std::list<databento::CbboMsg>(const std::vector<std::string>&)> getterFunc = 
        [this, at, &dataSet, schema, timeRange](const std::vector<std::string>& ids) {
            return m_getter->getCbboTimeseriesRange(ids, dataSet, schema, at, timeRange);
        };
    if (instrumentIds.size() <= m_nInstrumentsSplit)
    {
        Retry retry(m_nRetries);
        std::function<std::list<databento::CbboMsg>()> toPost = 
        [this,&getterFunc,&instrumentIds]() {
            // push execution on pool to ensure time series rate limits won't be exceeded
            auto future = m_timeseriesPool.post(getterFunc, instrumentIds);
            return future.get();
        };
        Retry::ErrorLogger loggerFunc = [&at, &instrumentIds](std::uint64_t nTry, const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "getCbboTimeseriesRange retry [" << nTry << "] at " << 
                fmt::format("{:%Y-%m-%d %H:%M:%S}", at) << " for IDS(" << std::endl <<
                AppUtils::joinVector(instrumentIds);
        };
        return retry.run(toPost, loggerFunc);
    } else {
        auto split = splitVector(instrumentIds, m_nInstrumentsSplit);
        using RetryCbbo = RetryDelayed<std::list<databento::CbboMsg>>; 
        std::list<RetryCbbo> futures; 
        std::list<std::list<databento::CbboMsg>> subLists;
        // fire all requests at once with pool size guarding rate limits
        for (auto it = split.begin(); it != split.end(); ++it)
        {
            auto& subVector = *it;
            RetryCbbo::FuncToRetry funcToRetry = [this, &getterFunc, &subVector](){
                return this->m_timeseriesPool.post(getterFunc, subVector);
            };
            RetryCbbo::ErrorLogger loggerFunc = [&at, &subVector](std::uint64_t nTry, const std::exception& e) {
                BOOST_LOG_TRIVIAL(error) << "getCbboTimeseriesRange retry [" << nTry << "] at " << 
                    fmt::format("{:%Y-%m-%d %H:%M:%S}", at) << " for IDS(" << std::endl <<
                    AppUtils::joinVector(subVector);
            };
            futures.emplace_back(std::move(RetryCbbo(m_nRetries, funcToRetry, loggerFunc)));
        }
        for (auto& future : futures)
        {
            // join all threads with results
            subLists.emplace_back(std::move(future.retrieve()));
        }
        return joinLists(subLists);
    }   
}
