#include "bentoclient/retriever.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/optionrecordgapfiller.hpp"
#include "bentoclient/marketenvironmentextended.hpp"

using namespace bentoclient;

Retriever::Retriever(TimeRange timeRange) :
    m_timeRange(timeRange)
{
}

OptionChain Retriever::getOptionChain(
    const std::string& symbol,
    Timestamp dateTime,
    const std::string& expiryDate)
{
    const OptionChain& rawOptionChain = getRawOptionChain(symbol, dateTime, expiryDate);
    std::shared_ptr<MarketEnvironment> marketEnvironment = getMarketEnvironment(symbol);
    double fRiskFreeRate = marketEnvironment->getRiskFreeRate(
        rawOptionChain.getChainTime(),
        rawOptionChain.getExpiryTime(marketEnvironment->getExchangeClose()));
    OptionRecordGapFiller gapFiller(marketEnvironment);
    return gapFiller.fillGaps(rawOptionChain);
}


std::pair<Timestamp, bool> Retriever::getNextInTimeRange(Timestamp at,
    const TimeToChainMap& timeToChainMap) const
{
    return MarketEnvironmentExtended::getNextInTimeRange(at, timeToChainMap, m_timeRange);
}
