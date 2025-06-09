#pragma once
#include "bentoclient/marketenvironment.hpp"
#include <list>
#include <memory>

namespace bentoclient 
{
    class OptionChain;
    /// @brief Utility for the patching of gaps in raw option chains built from databento streams
    class OptionRecordGapFiller
    {
        class Algos;
    public:
        /// @brief Constructs an option record gap filler for a market environment for parity computations
        /// @param marketEnvironment 
        OptionRecordGapFiller(std::shared_ptr<MarketEnvironment> marketEnvironment) :
            m_marketEnvironment(marketEnvironment),
            m_orphanedPuts{},
            m_orphanedCalls{}
        {}
        OptionRecordGapFiller() = delete;
        OptionRecordGapFiller(const OptionRecordGapFiller&) = delete;
        OptionRecordGapFiller& operator = (const OptionRecordGapFiller&) = delete;
        OptionRecordGapFiller(OptionRecordGapFiller&&) = default;
        OptionRecordGapFiller& operator = (OptionRecordGapFiller&&) = default;


        OptionChain fillGaps(const OptionChain& optionChain);

        /// @brief Strike keys for any calls that had no matching put
        const std::list<std::string>& getOrphanedCalls() const 
        {
            return m_orphanedCalls;
        }
        /// @brief Strike keys for any puts that had no matching calls
        const std::list<std::string>& getOrphanedPuts() const
        {
            return m_orphanedPuts;
        }
    private:
        std::shared_ptr<MarketEnvironment> m_marketEnvironment;
        std::list<std::string> m_orphanedPuts;
        std::list<std::string> m_orphanedCalls;
    public:
        static const std::string m_pcpFitComment;
        static const std::string m_spreadFitComment;
        static const std::string m_linInterpolComment;
        static const std::string m_logExtrapolateComment;
    };
}