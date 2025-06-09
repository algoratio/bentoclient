#pragma once

#include "bentoclient/requestersynchronous.hpp"
#include "bentoclient/threadpool.hpp"
#include "bentoclient/retriever.hpp"

namespace bentoclient
{
    /// @brief A requester that submits jobs to a thread pool to work asynchronously 
    /// on requests 
    class RequesterAsynchronous : public RequesterSynchronous
    {
    public:
        RequesterAsynchronous() = delete;
        /// @brief Constructs a getter posting jobs to an internal thread pool
        /// @param getter Data getter interface
        /// @param retriever Data intermediate storage interface
        /// @param persister Data persister interface
        /// @param sDataset The dataset to fetch data for, like OPRA.PILLAR
        /// @param cbbo1sRange The lookback time range for CBBO 1S timeseries
        /// @param cbbo1mRange The lookback time range for CBBO 1M timeseries
        /// @param nInstrumentsSplit The max number of instruments for a single time series call
        /// @param nThreads The number of threads in internal pool
        /// @param terminateSignal A signal handler function for bailout on interrupt
        /// @param fDefaultRiskFreeRate Default for risk free continuous rate, if yield curve missing
        /// @param sInterestRatesCsv Path to a CSV file having a yield curve in TSY par rate format
        RequesterAsynchronous(std::unique_ptr<Getter>&& getter, 
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
            );
        ~RequesterAsynchronous() override;

        /// @brief 
        /// @param symbol 
        /// @param dateTime 
        /// @param nDte 
        /// @return 
        JobId requestOptionChains(const std::string& symbol, 
            const bentoclient::Timestamp& dateTime, int nDte) override;

        /// @brief Checks the progress of submitted jobs, returns empty map when all done
        ThreadPool::ResultMap query();

        /// @brief Sets up an asynchronous requester interface
        /// @param sApiKey Databento API key
        /// @param sDataset Databento data set, such as OPRA.PILLAR
        /// @param sBasePath The base storage path for CSV persists
        /// @param nThreadsRequester Number of threads in underlying pool
        /// @param nThreadsSymbology Number of threads for symbology requests to databento
        /// @param nThreadsTimeseries Number of threads for time series requests to databento
        /// @param terminateSignal Signal handler to bail out on interrupt
        /// @param nSplitInstrumentIds Max number of instrument IDs in single time series call to databento
        /// @param nRetries Number of retries on sporadic request failures
        /// @param chainLookupTimeRange Time range for looking up chains from intermediate storage (retriever) interface
        /// @param cbbo1sTimeRange Lookback time range for CBBO 1S
        /// @param cbbo1mTimeRange Lookback time range for CBBO 1M
        /// @param fDefaultRiskFreeRate Default for risk free continuous rate, if yield curve missing
        /// @param sInterestRatesCsv Path to a CSV file having a yield curve in TSY par rate format
        /// @param bStacked Stacked CSV output instead of side by side
        /// @param bDateDirs Add valuation date directories ot the base CSV output path sBasePath
        /// @return The constructed requester interface
        static std::unique_ptr<RequesterAsynchronous> makeRequesterCSV(
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
            bool bDateDirs);

    private:
        ThreadPool m_threadPool;
    };

}