#pragma once

#include "bentoclient/retriever.hpp"
#include <map>
#include <mutex>
namespace bentoclient
{
    /// @brief A retriever internal storage using in-memory collections
    class RetrieverInMemory : public Retriever
    {
    public:
        typedef std::map<std::string, TimeToChainMap> ExpiryToChainsMap;
        typedef std::map<std::string, ExpiryToChainsMap> SymbolToExpiryMap;
        typedef std::map<std::string, std::shared_ptr<MarketEnvironment>> SymbolToMarketEnvironmentMap;
    public:
        RetrieverInMemory(TimeRange timeRange);
        ~RetrieverInMemory() override = default;

        void submitOptionChain(OptionChain&& optionChain) override;

        void submitMarketEnvironment(const std::string& symbol, 
            std::shared_ptr<MarketEnvironment> marketEnvironment) override;

        bool hasOptionChain(
            const std::string& symbol,
            Timestamp dateTime,
            const std::string& expiryDate) override;

        const OptionChain& getRawOptionChain(
            const std::string& symbol,
            Timestamp dateTime,
            const std::string& expiryDate) override;
        
        std::shared_ptr<MarketEnvironment> getMarketEnvironment(
            const std::string& symbol) const override;

    private:
        SymbolToExpiryMap m_chainData;
        SymbolToMarketEnvironmentMap m_marketEnvironmentData;
        mutable std::mutex m_mutex;
    };
}