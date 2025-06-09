#include "bentoclient/retrieverinmemory.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "bentoclient/optionchain.hpp"
#include <fmt/core.h>
#include <fmt/chrono.h>

using namespace bentoclient;

RetrieverInMemory::RetrieverInMemory(TimeRange timeRange) :
    Retriever(timeRange),
    m_chainData{},
    m_marketEnvironmentData{},
    m_mutex{}
{
}

void RetrieverInMemory::submitOptionChain(OptionChain&& optionChain)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    TimeToChainMap& timeToChainMap = m_chainData[optionChain.getUnderlier()]
        [optionChain.getExpiryDate()];
    Timestamp chainTime = optionChain.getChainTime();
    timeToChainMap.emplace(chainTime, std::move(optionChain));
}

void RetrieverInMemory::submitMarketEnvironment(const std::string& symbol, 
    std::shared_ptr<MarketEnvironment> marketEnvironment)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_marketEnvironmentData[symbol] = marketEnvironment;
}

bool RetrieverInMemory::hasOptionChain(
    const std::string& symbol,
    Timestamp dateTime,
    const std::string& expiryDate)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto symIt = m_chainData.find(symbol);
    if (symIt != m_chainData.end())
    {
        auto expIt = symIt->second.find(expiryDate);
        if (expIt != symIt->second.end())
        {
            return getNextInTimeRange(dateTime, expIt->second).second;
        }
    }
    return false; 
}

const OptionChain& RetrieverInMemory::getRawOptionChain(
    const std::string& symbol,
    Timestamp dateTime,
    const std::string& expiryDate)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto symIt = m_chainData.find(symbol);
    if (symIt != m_chainData.end())
    {
        auto expIt = symIt->second.find(expiryDate);
        if (expIt != symIt->second.end())
        {
            auto val = getNextInTimeRange(dateTime, expIt->second);
            if (val.second)
            {
                auto chainIt = expIt->second.find(val.first);
                if (chainIt != expIt->second.end())
                {
                    return chainIt->second;
                }
                throw std::runtime_error(fmt::format("Corrupted data retrieving chain"
                    " for {:%Y-%m-%d %H:%M:%S} of {} EXP {}",  dateTime, symbol, expiryDate));
            }
        }
    }

    throw std::invalid_argument(fmt::format("No option chain in acceptable time range"
        " for {:%Y-%m-%d %H:%M:%S} of {} EXP {}",  dateTime, symbol, expiryDate));
}

std::shared_ptr<MarketEnvironment> RetrieverInMemory::getMarketEnvironment(
    const std::string& symbol) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto symIt = m_marketEnvironmentData.find(symbol);
    if (symIt != m_marketEnvironmentData.end())
    {
        return symIt->second;
    }
    throw std::invalid_argument(fmt::format("No market environment data for symbol {}",
        symbol));
}
