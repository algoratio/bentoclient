#pragma once
#include "bentoclient/requester.hpp"
#include <memory>
#include <functional>

namespace bentoclient
{
    class MarketEnvironment;
    /// @brief A requester that loads option chains in the calling thread
    class RequesterSynchronous : public Requester
    {
        class Internal;
        friend class Internal;
    public:
        RequesterSynchronous() = delete;
        /// @brief Sets up a synchronous requester interface
        /// @param getter Data getter interface
        /// @param retriever Data intermediate storage interface
        /// @param persister Data output persister interface
        /// @param sDataset Databento dataset, such as OPRA.PILLAR
        /// @param cbbo1sRange CBBO 1S lookback time range
        /// @param cbbo1mRange CBBO 1M lookback time range
        /// @param nInstrumentsSplit Max number of instruments in databento timeseries requests
        /// @param fDefaultRiskFreeRate Default for risk free continuous rate, if yield curve missing
        /// @param sInterestRatesCsv Path to a CSV file having a yield curve in TSY par rate format
        RequesterSynchronous(std::unique_ptr<Getter>&& getter, 
            std::unique_ptr<Retriever>&& retriever,
            std::unique_ptr<Persister>&& persister,
            const std::string& sDataset,
            TimeRange cbbo1sRange,
            TimeRange cbbo1mRange,
            std::uint64_t nInstrumentsSplit,
            double fDefaultRiskFreeRate,
            const std::string& sInterestRatesCsv
            );
        ~RequesterSynchronous() override;

        JobId requestOptionChains(const std::string& symbol, 
            const bentoclient::Timestamp& dateTime, int nDte) override;

        void getOptionChains(const std::string& symbol, 
            const bentoclient::Timestamp& dateTime, int nDte);

        void setTerminateSignal(std::function<bool()> terminateSignal);
    private:
        std::unique_ptr<Internal> m_internal;
        std::shared_ptr<MarketEnvironment> m_marketEnvironment;
    protected:
        std::function<bool()> m_terminateSignal; 
        TimeRange m_cbbo1sRange;
        TimeRange m_cbbo1mRange;
        std::string m_sDataset;
        std::uint64_t m_nInstrumentsSplit;
    };
}
