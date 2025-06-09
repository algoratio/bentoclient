#pragma once
#include <string>
#include <list>
#include <memory>
#include "bentoclient/clienttypes.hpp"
namespace bentoclient
{
    class OptionChain;
    class MarketEnvironment;
    /// @brief Option chain persister base interface
    class Persister
    {
    public:
        Persister();
        Persister(const Persister&) = delete;
        Persister& operator = (const Persister&) = delete;
        Persister(Persister&&) = default;
        Persister& operator = (Persister&&) = default;
        virtual ~Persister() = default;

        /// @brief Persist an option chain
        /// @param optionChain Chain to persist
        /// @param marketEnvironment Market enviroment for put-call-parity compatible rte
        virtual void persist(OptionChain&& optionChain, 
            std::shared_ptr<MarketEnvironment> marketEnvironment) = 0;

        /// @brief Perist missing chain notice
        virtual void persistMissing(const std::string& symbol, const std::string& sDate,
            std::list<std::pair<Timestamp, std::string>>&& missingList) = 0;
    };
}