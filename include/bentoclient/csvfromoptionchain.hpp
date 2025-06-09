#pragma once
#include "bentoclient/marketenvironment.hpp"
#include <list>
#include <memory>


namespace bentoclient
{
    class OptionChain;
    class MarketEnvironment;
    /// @brief Converts an option chain's record data to CSV
    class CSVFromOptionChain
    {
        class Algos;
    public:
        /// @brief CSV header columns
        struct HeaderCols
        {
            // column names
            static const std::string m_symbol;
            static const std::string m_date;
            static const std::string m_time;
            static const std::string m_rate;
            static const std::string m_type;
            static const std::string m_strike;
            static const std::string m_bid;
            static const std::string m_mid;
            static const std::string m_ask;
            static const std::string m_expDate;
            static const std::string m_recvTime;
            static const std::string m_lastTrade;
            static const std::string m_lastTradeTime;
            static const std::string m_lastTradeSize;
            static const std::string m_bidSize;
            static const std::string m_askSize;
            static const std::string m_precision;
            static const std::string m_comment;
            // column name prefixes
            static const std::string m_prefix_call_stacked;
            static const std::string m_prefix_put_stacked;

            /// @brief derives a list of columns for side by side display of puts and calls
            static std::list<std::string> getSideBySideCols();
            /// @brief derives a list of columns for stacked display of puts and calls
            static std::list<std::string> getStackedCols();

            /// @brief capitalizes the first letter of header columns
            static std::list<std::string> capitalizeFirst(const std::list<std::string>& cols);

            /// @brief prefixes header columns for calls in side by side display
            static std::string pcs(const std::string& str) { return m_prefix_call_stacked + str; }
            /// @brief prefixes header columns for puts in side by side display
            static std::string pps(const std::string& str) { return m_prefix_put_stacked + str; }
        };
        /// @brief Option type markers for stacked display of puts and calls
        struct Types
        {
            static const std::string m_put;
            static const std::string m_call;
        };
    public:
        CSVFromOptionChain() = delete;
        /// @brief Constructs CSV converter with put-call-parity computation parameters
        /// @param marketEnvironment The market environment for put-call-parity
        CSVFromOptionChain(std::shared_ptr<MarketEnvironment> marketEnvironment);
        ~CSVFromOptionChain() = default;

        /// @brief Writes a side by side CSV representation of an option chain to output stream
        /// @param ostr Stream to write CSV to
        /// @param optionChain Option chain to convert to CSV
        void sideBySide(std::ostream& ostr, const OptionChain& optionChain) const;
        /// @brief Writes a stacked put and call CSV representation of an option chain to output stream 
        /// @param ostr Stream to write CSV to
        /// @param optionChain Option chain to convert to CSV
        void stacked(std::ostream& ostr, const OptionChain& optionChain) const;
    private:
        /// @brief Compute the put-call-parity rate (price of underlier)
        double getPcpRate(const OptionChain& optionChain) const;
        /// @brief computes the precision column estimating the variability in put-call-parity across records
        double computePrecision(const OptionChain& optionChain) const;
    private:
        /// @brief Market environment for put-call-parity
        std::shared_ptr<MarketEnvironment> m_marketEnvironment;
    };
}