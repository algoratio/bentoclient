#pragma once

#include "bentoclient/getter.hpp"
#include "bentoclient/variadicthreadpool.hpp"
#include <memory>

namespace bentoclient
{
    /// @brief Aggregates a synchronous getter with thread pools
    class GetterAsynchronous : public Getter
    {
    public:
        /// @brief Creates an asynchronous getter
        /// @param getter Synchronous getter instance to run in threads 
        /// @param nSymbologyThreads Number of symbology request threads
        /// @param nTimeseriesThreads Number of timeseries request threads
        /// @param nInstrumentsSplit Max number of instruments in timeseries request
        /// @param nRetries Number of retries on sporadic request failures
        GetterAsynchronous(std::unique_ptr<Getter>&& getter,
            std::uint64_t nSymbologyThreads,
            std::uint64_t nTimeseriesThreads,
            std::uint64_t nInstrumentsSplit,
            std::uint64_t nRetries);
        ~GetterAsynchronous() override;

        databento::SymbologyResolution getSymbologyResolution(
            const std::string& dataSet,
            const std::string& sUnderlier, const std::string& sDate) override;

        std::list<databento::CbboMsg> getCbboTimeseriesRange(
            const std::vector<std::string>& instrumentIds,
            const std::string& dataSet,
            databento::Schema schema,
            bentoclient::Timestamp at,
            TimeRange timeRange) override;

        /// @brief splits a vector into subvectors of max length nSplit
        /// @details avoids https request buffer overflows
        template<typename T> 
        static std::list<std::vector<T>> splitVector(
            const std::vector<T>& toSplit, std::uint64_t nSplit)
        {
            std::list<std::vector<T>> ret;
            if (toSplit.size() <= nSplit)
            {
                ret.push_back(toSplit);
            } 
            else 
            {
                std::uint64_t n = toSplit.size() / nSplit;
                std::uint64_t segment = toSplit.size() / (n+1);
                auto it = toSplit.begin();
                for (uint64_t i = 0; i < n; ++i)
                {
                    std::vector<T> subVector(it+i*segment, it+(i+1)*segment);
                    ret.emplace_back(std::move(subVector));
                }
                std::vector<T> subVector(it+n*segment, toSplit.end());
                ret.emplace_back(std::move(subVector));
            }
            return ret;
        }
        /// @brief Joins response lists for split request vectors
        template<typename T>
        static std::list<T> joinLists(std::list<std::list<T>>& toJoin)
        {
            std::list<T> ret;
            for (auto& subList : toJoin)
            {
                ret.splice(ret.end(), std::move(subList));
            }
            return ret;            
        }

    private:
        std::unique_ptr<Getter> m_getter;
        VariadicThreadPool m_symbologyPool;
        VariadicThreadPool m_timeseriesPool;
        const std::uint64_t m_nInstrumentsSplit;
        const std::uint64_t m_nRetries;
    };
}