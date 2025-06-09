#include <catch2/catch_test_macros.hpp>
#include "bentoclient/optionchain.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "bentoclient/retrieverinmemory.hpp"
#include "bentoclient/requestersynchronous.hpp"
#include "bentoclient/getter.hpp"
#include "bentoclient/persistercsv.hpp"
#include "bentoclient/dateutils.hpp"
#include "persistercsvinterceptor.hpp"
#include "dataloader.hpp"

namespace
{
    class GetterMockup : public bentoclient::Getter
    {
    public:
        GetterMockup(
            databento::SymbologyResolution&& symbologyResolution,
            std::list<databento::CbboMsg>&& cbboMsgs) :
            m_symbologyResolution(std::move(symbologyResolution)),
            m_cbboMsgs(std::move(cbboMsgs))
        {}
        databento::SymbologyResolution getSymbologyResolution(const std::string& sDataset,
            const std::string& sUnderlier, const std::string& sDate) override
        {
            return m_symbologyResolution;
        }

        std::list<databento::CbboMsg> getCbboTimeseriesRange(
            const std::vector<std::string>& instrumentIds,
            const std::string& sDataset,
            databento::Schema schema,
            bentoclient::Timestamp at,
            bentoclient::TimeRange timeRange) override
        {
            if (schema == databento::Schema::Cbbo1S)
                return m_cbboMsgs;
            return std::list<databento::CbboMsg>();
        }
    private:
        databento::SymbologyResolution m_symbologyResolution;
        std::list<databento::CbboMsg> m_cbboMsgs;

    };
}

TEST_CASE( "RequesterSynchronous test", "[requestersync]" ) {
    // Test the LiveClient class
    std::string sSymbol("SPY");
    std::string sDate("2025-04-02");
    //std::string sExpiryDate("2025-04-04");
    databento::SymbologyResolution symbologyResolution = 
        bentotests::DataLoader().getSymbologyResolution("SPY_symbology_2025-04-02.txt");
    std::list<databento::CbboMsg> cbboMsgs = 
        bentotests::DataLoader().getCbboMessages("SPY_cbbos_2025-04-02_17-30.txt");
    REQUIRE( cbboMsgs.size() == 3377 );
    
    std::list<std::string> capturedCsv, capturedPath;
    std::list<std::string> missingChainTxt, missingChainFile;

    std::unique_ptr<bentoclient::Getter> getterMockup =
        std::make_unique<GetterMockup>(std::move(symbologyResolution),
        std::move(cbboMsgs));
    std::unique_ptr<bentoclient::Retriever> retrieverInMemory =
        std::make_unique<bentoclient::RetrieverInMemory>(std::chrono::minutes(10));
    std::string basePath("basePath");
    std::unique_ptr<bentoclient::PersisterCSV> persisterCsv = 
        std::make_unique<bentoclient::PersisterCSV>(basePath, true,
        bentoclient::PersisterCSV::CSVFormat::SideBySide);
    persisterCsv->setOutputter(bentotests::createStringCaptureOutputter(
        capturedPath, capturedCsv));
    persisterCsv->setMissingOutputter(bentotests::createStringCaptureOutputter(
        missingChainFile, missingChainTxt));
    bentoclient::TimeRange cbbo1sTimeRange = std::chrono::seconds(10);
    bentoclient::TimeRange cbbo1mTimeRange = std::chrono::minutes(60);
    std::uint64_t nSplitInstrumentIds = 100;
    const std::string& sDataset("OPRA.PILLAR");
    bentoclient::RequesterSynchronous requesterSynchronous(
        std::move(getterMockup),
        std::move(retrieverInMemory),
        std::move(persisterCsv),
        sDataset,
        cbbo1sTimeRange,
        cbbo1mTimeRange,
        nSplitInstrumentIds,
        0.04,
        std::string{}
    );
    bentoclient::Timestamp at = bentoclient::DateUtils::makeTimestamp(sDate, "17:30:00", 
        bentoclient::DateUtils::Timezone::m_UTC);
    int nDte = 2;
    requesterSynchronous.getOptionChains(sSymbol, at, nDte);
    // since the mockup data is only for one chain, the two first dates in the nDte range
    // produce empty, invalid chains. 
    REQUIRE(capturedCsv.size() == 1);
    REQUIRE(capturedPath.size() == 1);
    std::string expectedPath = basePath + "/"  + sDate + "/spy_chain_2025-04-02_2025-04-04_n193.csv";
    REQUIRE(capturedPath.front() == expectedPath);
    std::vector<std::string> csvLines = bentoclient::AppUtils::splitByLinefeed(capturedCsv.front());
    std::string expectedHeaders = "Symbol,Date,Time,Rate,Strike,C_bid,C_mid,C_ask,P_bid,P_mid,P_ask,"
        "ExpDate,C_BidSize,C_AskSize,C_RecvTime,C_LastTrade,C_LastTradeTime,C_LastTradeSize,C_Comment,"
        "P_BidSize,P_AskSize,P_RecvTime,P_LastTrade,P_LastTradeTime,P_LastTradeSize,P_Comment,Precision";
    REQUIRE(csvLines.at(0) == expectedHeaders);
    std::string expectedFirstLine = "SPY,2025-04-02,13:30:09,566.05,375,190.99,191.10,191.21,0.00,0.01,0.01,"
        "2025-04-04,72,64,2025-04-02 13:30:09,186.62,2025-04-02 11:01:46,1,,"
        "1,1686,2025-04-02 13:30:09,nan,{null},0,spread-fit,0.0465";
    REQUIRE(csvLines.at(1) == expectedFirstLine);
    std::string expectedLastLine = "SPY,2025-04-02,13:30:09,566.05,700,0.00,0.01,0.01,"
        "133.89,134.01,134.14,2025-04-04,1,5030,2025-04-02 13:30:09,nan,{null},0,spread-fit,"
        "75,12,2025-04-02 13:30:09,nan,{null},0,,0.0465";
    REQUIRE(csvLines.at(193) == expectedLastLine);
    // Capture of missing messages
    REQUIRE(missingChainFile.size() == 1);
    REQUIRE(missingChainTxt.size() == 1);
    REQUIRE(missingChainFile.front() == "basePath/2025-04-02/spy_missing_2025-04-02_17-30-00.000000000.txt");
    REQUIRE(missingChainTxt.front() == "2025-04-02 17:30:00.000000000 EXP 2025-04-02\n"
        "2025-04-02 17:30:00.000000000 EXP 2025-04-03\n");
}

