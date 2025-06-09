#pragma once
#include "bentoclient/clienttypes.hpp"
#include "bentoclient/dateutils.hpp"
#include <chrono>
#include <map>
#include <memory>
namespace bentoclient
{
    class OptionChain;
    class MarketEnvironment;
    /// @brief Intermediate storage handler base interface
    class Retriever
    {
    public:
        typedef std::map<Timestamp, OptionChain> TimeToChainMap;
    public:
        /// @brief Constructs a retriever for a lookup leniency time range
        /// @param timeRange The lookup timerange, within which stored objects match requested datetimes
        Retriever(TimeRange timeRange);
        Retriever(const Retriever&) = delete;
        Retriever& operator = (const Retriever&) = delete;
        Retriever(Retriever&&) = default;
        Retriever& operator = (Retriever&&) = default;
        virtual ~Retriever() = default;

        /// @brief Add a raw option chain to internal storage
        /// @param optionChain Option chain as received from market source
        virtual void submitOptionChain(OptionChain&& optionChain) = 0;

        /// @brief  Add market environment information to internal storage
        /// @details Subclass MarketEnvironment to integrate yield curves
        /// @param symbol Underlier for the market environment
        /// @param marketEnvironment Market environment info
        virtual void submitMarketEnvironment(const std::string& symbol, 
            std::shared_ptr<MarketEnvironment> marketEnvironment) = 0;

        /// @brief Checks if an OptionChain withing acceptable time range is present
        /// @param symbol Symbol for option chain
        /// @param dateTime Requested valuation time and date
        /// @param expiryDate Expiry date for option chain
        /// @return True if OptionChain is present in acceptable distance from dateTime
        virtual bool hasOptionChain(
            const std::string& symbol,
            Timestamp dateTime,
            const std::string& expiryDate) = 0;

        /// @brief Returns completed copy of OptionChain if one exists 
        /// in acceptable distance from {dateTime}
        /// @param symbol Symbol for option chain
        /// @param dateTime Requested valuation time and date
        /// @param expiryDate Expiry date for option chain
        /// @return OptionChain with enhancements, or throws an error if none exists.
        virtual OptionChain getOptionChain(
            const std::string& symbol,
            Timestamp dateTime,
            const std::string& expiryDate);

        /// @brief Returns OptionChain as received from databento with filled gaps marked
        /// in acceptable distance from {dateTime}
        /// @param symbol Symbol for option chain
        /// @param dateTime Requested valuation time and date
        /// @param expiryDate Expiry date for option chain
        /// @return original OptionChain without enhancements
        virtual const OptionChain& getRawOptionChain(
            const std::string& symbol,
            Timestamp dateTime,
            const std::string& expiryDate) = 0;

        /// @brief Get the market environment for a symbol for the completion of options data
        /// @param symbol Option's underlier
        /// @return Market environment data
        virtual std::shared_ptr<MarketEnvironment> getMarketEnvironment(
            const std::string& symbol) const = 0;

        /// @brief Find closest key to {at} and verify it's in internal time range
        /// @param at Time to find closest key for
        /// @param timeToChainMap Map with all chains for expiry date and symbol
        /// @return Timestamp and boolean if Timestamp is in range
        std::pair<Timestamp, bool> getNextInTimeRange(Timestamp at, 
            const TimeToChainMap& timeToChainMap) const;
    private:
        const TimeRange m_timeRange;
    };

}