#include <catch2/catch_test_macros.hpp>
#include "bentoclient/csvfromoptionchain.hpp"
#include "bentoclient/optionrecordgapfiller.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "dataloader.hpp"
#include "persistercsvinterceptor.hpp"

namespace bc = bentoclient;

static bc::OptionChain fillChain(std::shared_ptr<bc::MarketEnvironment> marketEnvironment)
{
    std::string sSymbol("SPY");
    std::string sDate("2025-04-02");
    std::string sExpiryDate("2025-04-04");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChain(
            "SPY_symbology_2025-04-02.txt",
            sSymbol, sDate, sExpiryDate,
            "SPY_cbbos_2025-04-02_17-30.txt");

    REQUIRE( optionChain.getPuts().size() == 193 );
    REQUIRE( optionChain.getCalls().size() == 193 );
    bc::OptionRecordGapFiller gapFiller(marketEnvironment);
    bc::OptionChain filled = gapFiller.fillGaps(optionChain);
    return filled;
}



TEST_CASE( "PersisterCSV output interceptor test", "[persistercsv]" ) {
    std::list<std::string> capturedCsv, capturedPath;

    bc::PersisterCSV::Outputter capturingOutputter = 
        bentotests::createStringCaptureOutputter(capturedPath, capturedCsv);

    double fRiskFreeRate = 0.04;
    auto marketEnvironment(std::make_shared<bc::MarketEnvironment>(fRiskFreeRate, bc::DateUtils::m_nasdaqClose));
    bc::OptionChain filledChain(fillChain(marketEnvironment));
    bc::OptionChain filledChainCopy(filledChain);
    std::string basePath("basePath");
    bc::PersisterCSV persisterCsv(basePath, 
        true, bc::PersisterCSV::CSVFormat::SideBySide);
    persisterCsv.setOutputter(std::move(capturingOutputter));
    // from the filled chain, run the csv conversion
    bc::CSVFromOptionChain toCSV(marketEnvironment);
    persisterCsv.persist(std::move(filledChain),  marketEnvironment);
    REQUIRE(capturedPath.size() == 1);
    REQUIRE(capturedPath.front() == basePath + "/" 
        + filledChainCopy.getValuationDate() + "/spy_chain_2025-04-02_2025-04-04_n193.csv");
    REQUIRE(capturedCsv.size() == 1);
    std::vector<std::string> lines = bc::AppUtils::splitByLinefeed(capturedCsv.front());
    std::string expectedHeaders = "Symbol,Date,Time,Rate,Strike,C_bid,C_mid,C_ask,P_bid,P_mid,P_ask,"
        "ExpDate,C_BidSize,C_AskSize,C_RecvTime,C_LastTrade,C_LastTradeTime,C_LastTradeSize,C_Comment,"
        "P_BidSize,P_AskSize,P_RecvTime,P_LastTrade,P_LastTradeTime,P_LastTradeSize,P_Comment,Precision";
    REQUIRE(lines.at(0) == expectedHeaders);
    std::string expectedFirstLine = "SPY,2025-04-02,13:30:09,566.05,375,190.99,191.10,191.21,0.00,0.01,0.01,"
        "2025-04-04,72,64,2025-04-02 13:30:09,186.62,2025-04-02 11:01:46,1,,"
        "1,1686,2025-04-02 13:30:09,nan,{null},0,spread-fit,0.0465";
    REQUIRE(lines.at(1) == expectedFirstLine);
    std::string expectedLastLine = "SPY,2025-04-02,13:30:09,566.05,700,0.00,0.01,0.01,"
        "133.89,134.01,134.14,2025-04-04,1,5030,2025-04-02 13:30:09,nan,{null},0,spread-fit,"
        "75,12,2025-04-02 13:30:09,nan,{null},0,,0.0465";
    REQUIRE(lines.at(193) == expectedLastLine);

}