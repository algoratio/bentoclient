#pragma once
#include <list>
#include <string>
#include <cstdint>
#include <memory>
#include "bentoclient/clienttypes.hpp"

namespace bentoclient
{
    class Getter;
    class Retriever;
    class Persister;
    /// @brief Option chain requester base interface
    class Requester
    {
    public:
        /// @brief Job ID from potentially underlying thread pool
        typedef std::uint64_t JobId;
    public:
        Requester() = delete;
        /// @brief Constructs a requester from getter, retriever and persister interfaces
        /// @param getter Data getter interface
        /// @param retriever Data intermediate storage handler
        /// @param persister Data persister interface
        Requester(std::unique_ptr<Getter>&& getter, 
            std::unique_ptr<Retriever>&& retriever,
            std::unique_ptr<Persister>&& persister);
        Requester(const Requester&) = delete;
        Requester& operator = (const Requester&) = delete;
        Requester(Requester&&) = default;
        Requester& operator = (Requester&&) = default;
        virtual ~Requester() = default;
        
        /// @brief Requests loading of option chains over several expiry dates
        /// @param symbol Underlying stock symbol, like SPY
        /// @param dateTime Valuation date and time
        /// @param nDte Maximum number of days to expiry from symbology
        /// @return A job ID, possibly from an underlying thread pool
        virtual JobId requestOptionChains(const std::string& symbol, 
            const bentoclient::Timestamp& dateTime, int nDte) = 0;

    protected:
        std::unique_ptr<Getter> m_getter;
        std::unique_ptr<Retriever> m_retriever;
        std::unique_ptr<Persister> m_persister;

    };

}