#include "bentoclient/csvfromoptionchain.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/osioption.hpp"
#include "bentoclient/datagrid.hpp"
#include <boost/log/trivial.hpp>

using namespace bentoclient;

const std::string CSVFromOptionChain::HeaderCols::m_symbol("Symbol");
const std::string CSVFromOptionChain::HeaderCols::m_date("Date");
const std::string CSVFromOptionChain::HeaderCols::m_time("Time");
const std::string CSVFromOptionChain::HeaderCols::m_rate("Rate");
const std::string CSVFromOptionChain::HeaderCols::m_type("Type");
const std::string CSVFromOptionChain::HeaderCols::m_strike("Strike");
const std::string CSVFromOptionChain::HeaderCols::m_bid("bid");
const std::string CSVFromOptionChain::HeaderCols::m_mid("mid");
const std::string CSVFromOptionChain::HeaderCols::m_ask("ask");
const std::string CSVFromOptionChain::HeaderCols::m_prefix_call_stacked("C_");
const std::string CSVFromOptionChain::HeaderCols::m_prefix_put_stacked("P_");
const std::string CSVFromOptionChain::HeaderCols::m_expDate("ExpDate");
const std::string CSVFromOptionChain::HeaderCols::m_recvTime("RecvTime");
const std::string CSVFromOptionChain::HeaderCols::m_lastTrade("LastTrade");
const std::string CSVFromOptionChain::HeaderCols::m_lastTradeTime("LastTradeTime");
const std::string CSVFromOptionChain::HeaderCols::m_lastTradeSize("LastTradeSize");
const std::string CSVFromOptionChain::HeaderCols::m_bidSize("BidSize");
const std::string CSVFromOptionChain::HeaderCols::m_askSize("AskSize");
const std::string CSVFromOptionChain::HeaderCols::m_precision("Precision");
const std::string CSVFromOptionChain::HeaderCols::m_comment("Comment");

const std::string CSVFromOptionChain::Types::m_put("Put");
const std::string CSVFromOptionChain::Types::m_call("Call");

class CSVFromOptionChain::Algos{
public:
    typedef const std::function<void(std::string &&, 
        const OptionChain::Record&, const OptionChain::Record&)>
            SideBySideFunctor;
    static void sideBySide(const OptionChain& optionChain, SideBySideFunctor rowFunctor)
    {
        auto& puts = optionChain.getPuts();
        auto& calls = optionChain.getCalls();
        for (auto putIt = puts.begin(); putIt != puts.end(); ++putIt)
        {
            auto callIt = calls.find(putIt->first);
            if (callIt != calls.end())
            {
                rowFunctor(OsiOption::fromStrikeKeyAsString(putIt->first),
                    putIt->second, callIt->second);
            }
        }
    }
    typedef const std::function<void(std::string&&, const std::string, const OptionChain::Record&)> 
        StackedFunctor;
    static void stacked(const std::map<std::string, OptionChain::Record>& recordMap, 
        const std::string& type, StackedFunctor rowFunctor)
    {
        for (auto it = recordMap.begin(); it != recordMap.end(); ++it)
        {
            rowFunctor(OsiOption::fromStrikeKeyAsString(it->first), type, it->second);
        }
    }
};

std::list<std::string> CSVFromOptionChain::HeaderCols::getSideBySideCols()
{
    return std::list<std::string>(
    {
        m_symbol,
        m_date,
        m_time,
        m_rate,
        m_strike,
        pcs(m_bid),
        pcs(m_mid),
        pcs(m_ask),
        pps(m_bid),
        pps(m_mid),
        pps(m_ask),
        m_expDate,
        pcs(m_bidSize),
        pcs(m_askSize),
        pcs(m_recvTime),
        pcs(m_lastTrade),
        pcs(m_lastTradeTime),
        pcs(m_lastTradeSize),
        pcs(m_comment),
        pps(m_bidSize),
        pps(m_askSize),
        pps(m_recvTime),
        pps(m_lastTrade),
        pps(m_lastTradeTime),
        pps(m_lastTradeSize),
        pps(m_comment),
        m_precision
    }
    );

}

std::list<std::string> CSVFromOptionChain::HeaderCols::getStackedCols()
{
    return std::list<std::string>(
    {
        m_symbol,
        m_date,
        m_time,
        m_rate,
        m_type,
        m_strike,
        m_bid,
        m_mid,
        m_ask,
        m_expDate,
        m_bidSize,
        m_askSize,
        m_recvTime,
        m_lastTrade,
        m_lastTradeTime,
        m_lastTradeSize,
        m_comment,
        m_precision
    }
    );
}

std::list<std::string> CSVFromOptionChain::HeaderCols::capitalizeFirst(const std::list<std::string>& cols)
{
    std::list<std::string> caps;
    for (const auto& col: cols)
    {
        std::string cap(col);
        if (!cap.empty())
        {
            cap[0] = std::toupper(cap[0]);
        }
        caps.emplace_back(cap);
    }
    return caps;
}

CSVFromOptionChain::CSVFromOptionChain(std::shared_ptr<MarketEnvironment> marketEnvironment) :
    m_marketEnvironment(marketEnvironment)
{}


void CSVFromOptionChain::sideBySide(std::ostream& ostr, const OptionChain& optionChain) const
{
    double fPcpRate = getPcpRate(optionChain);
    double fPrecision = computePrecision(optionChain);
    DataGrid grid;
    Algos::SideBySideFunctor rowFunctor = [&grid](std::string&& strike, 
        const OptionChain::Record& put, const OptionChain::Record& call)
    {
        grid.addRow(
            std::move(strike),
            call.getBidPrice(),
            call.getMidPrice(),
            call.getAskPrice(),
            put.getBidPrice(),
            put.getMidPrice(),
            put.getAskPrice(),
            std::get<1>(call.m_bidPrice),
            std::get<1>(call.m_askPrice),
            call.getRecvTime(),
            call.getTradePrice(),
            call.getTradeTime(),
            std::get<1>(call.m_price),
            call.m_comment,
            std::get<1>(put.m_bidPrice),
            std::get<1>(put.m_askPrice),
            put.getRecvTime(),
            put.getTradePrice(),
            put.getTradeTime(),
            std::get<1>(put.m_price),
            put.m_comment
            );
    };
    Algos::sideBySide(optionChain, rowFunctor);
    // set timestamp formatting for trades
    grid.setFormat(DataGrid::FormatFunctor(
        [this](){
            return std::make_unique<DataGrid::TimestampFormatSeconds>(
                DataGrid::m_defaultTimestampFormat, 
                this->m_marketEnvironment->getExchangeClose().m_timeZone);
        }
    ));
    grid.insertColumn(7, optionChain.getExpiryTime(m_marketEnvironment->getExchangeClose()));
    grid.setFormat(7, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultDateFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, fPcpRate);
    grid.insertColumn(0, optionChain.getChainTime());
    grid.setFormat(0, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultTimeFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, optionChain.getChainTime());
    grid.setFormat(0, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultDateFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, optionChain.getUnderlier());
    grid.setFormat(DataGrid::FormatFunctor([]{return std::make_unique<DataGrid::DoubleFormat>();}));
    std::size_t precPos = grid.addColumn(fPrecision);
    grid.setFormat(precPos, std::make_unique<DataGrid::DoubleFormat>(".4f"));
    grid.setColNames(HeaderCols::capitalizeFirst(HeaderCols::getSideBySideCols()));
    grid.serializeHeaderRow(ostr);
    grid.serializeStringGrid(ostr);
}

void CSVFromOptionChain::stacked(std::ostream& ostr, const OptionChain& optionChain) const
{
    double fPcpRate = getPcpRate(optionChain);
    double fPrecision = computePrecision(optionChain);
    DataGrid grid;
    Algos::StackedFunctor rowFunctor = [&grid](std::string&& strike, 
        const std::string& type, const OptionChain::Record& rec)
    {
        grid.addRow(
            type,
            std::move(strike),
            rec.getBidPrice(),
            rec.getMidPrice(),
            rec.getAskPrice(),
            std::get<1>(rec.m_bidPrice),
            std::get<1>(rec.m_askPrice),
            rec.getRecvTime(),
            rec.getTradePrice(),
            rec.getTradeTime(),
            std::get<1>(rec.m_price),
            rec.m_comment
            );
    };
    Algos::stacked(optionChain.getPuts(), Types::m_put, rowFunctor);
    Algos::stacked(optionChain.getCalls(), Types::m_call, rowFunctor);
    // set timestamp formatting for trades
    grid.setFormat(DataGrid::FormatFunctor(
        [this](){
            return std::make_unique<DataGrid::TimestampFormatSeconds>(
                DataGrid::m_defaultTimestampFormat, 
                this->m_marketEnvironment->getExchangeClose().m_timeZone);
        }
    ));
    grid.insertColumn(5, optionChain.getExpiryTime(m_marketEnvironment->getExchangeClose()));
    grid.setFormat(5, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultDateFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, fPcpRate);
    grid.insertColumn(0, optionChain.getChainTime());
    grid.setFormat(0, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultTimeFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, optionChain.getChainTime());
    grid.setFormat(0, std::make_unique<DataGrid::TimestampFormatSeconds>(
        DataGrid::m_defaultDateFormat, m_marketEnvironment->getExchangeClose().m_timeZone));
    grid.insertColumn(0, optionChain.getUnderlier());
    grid.setFormat(DataGrid::FormatFunctor([]{return std::make_unique<DataGrid::DoubleFormat>();}));
    std::size_t precPos = grid.addColumn(fPrecision);
    grid.setFormat(precPos, std::make_unique<DataGrid::DoubleFormat>(".4f"));
    grid.setColNames(HeaderCols::capitalizeFirst(HeaderCols::getStackedCols()));
    grid.serializeHeaderRow(ostr);
    grid.serializeStringGrid(ostr);
}

double CSVFromOptionChain::getPcpRate(const OptionChain& optionChain) const
{   
    return optionChain.getParityRate(
        m_marketEnvironment->getRiskFreeRate(
            optionChain.getChainTime(), 
            optionChain.getExpiryTime(m_marketEnvironment->getExchangeClose())
        ),
        m_marketEnvironment->getExchangeClose()
    );
}

double CSVFromOptionChain::computePrecision(const OptionChain& optionChain) const
{
    double fPrecision = std::nan("0xbad");
    try {
        double fRiskFreeRate = m_marketEnvironment->getRiskFreeRate(
            optionChain.getChainTime(), 
            optionChain.getExpiryTime(m_marketEnvironment->getExchangeClose())
        );
        fPrecision = std::sqrt(optionChain.getParityRateQualityScore(fRiskFreeRate, 
            m_marketEnvironment->getExchangeClose()));
    } catch (const std::exception& e)
    {
        BOOST_LOG_TRIVIAL(warning) << "Failed to compute precision for symbol " << optionChain.getUnderlier() << " at " 
            << optionChain.getValuationDate() << " for expiry " << optionChain.getExpiryDate() 
            << ", due to cause: " << e.what();
    }
    return fPrecision;
}
