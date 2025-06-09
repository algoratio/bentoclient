#pragma once
#include "bentoclient/dateutils.hpp"

namespace bentoclient
{
    /// @brief MarketEnvironment holds market data for option chain computations
    /// including the risk free rate and information on stock exchanges.
    /// @details Market environments are shared across threads and must remain immutable
    class MarketEnvironment
    {
    public:
        /// @brief Constructs a base market environment
        /// @param fRiskFreeRate Risk free interest rate
        /// @param exchangeClose Information on closing time of exchange
        MarketEnvironment(double fRiskFreeRate,
            const DateUtils::ExchangeClose& exchangeClose) :
        m_fRiskFreeRate(fRiskFreeRate),
        m_exchangeClose(exchangeClose)
        {}
        MarketEnvironment() = delete;
        // ensure immutable state to avoid synchronization needs
        MarketEnvironment(const MarketEnvironment&) = delete;
        MarketEnvironment& operator = (const MarketEnvironment&) = delete;
        MarketEnvironment(MarketEnvironment&&) = delete;
        MarketEnvironment& operator = (MarketEnvironment&&) = delete;
        virtual ~MarketEnvironment() {}

        /// @brief Returns the risk free rate depending on valuation time and expiry time
        /// @param valuationTime Valuation time of option
        /// @param expiryTime Expiry time of option
        /// @return A continuously compounded rate for the relevant time period
        virtual double getRiskFreeRate(Timestamp valuationTime, Timestamp expiryTime) const
        {
            // sublass to integrate a yield curve
            /* No synchronization in this method, since all class data is const */
            return m_fRiskFreeRate;
        }

        /// @brief Information on closing time and time zone of the underlier's exchange
        /// @return Exchange close reference
        const DateUtils::ExchangeClose& getExchangeClose() const
        {
            /* No synchronization in this method, since all class data is const */
            return m_exchangeClose;
        }
    protected:
        /// @brief The default risk free rates for all bond maturities
        const double m_fRiskFreeRate;
    private:
        /// @brief Exchange for the underlier
        const DateUtils::ExchangeClose m_exchangeClose;
    };

}
