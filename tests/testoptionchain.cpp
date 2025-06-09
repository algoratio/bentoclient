#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "bentoclient/optionchain.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/dateutils.hpp"
#include "dataloader.hpp"
#include <algorithm>

namespace bc = bentoclient;

TEST_CASE( "Build Option Chain", "[buildoptionchain]" ) {
    // Check operator == for Records
    bc::OptionChain::Record rA{}, rB{};
    REQUIRE(rA == rB);
    // load option chain
    std::string sSymbol("SPY");
    std::string sDate("2025-04-02");
    std::string sExpiryDate("2025-04-04");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChain(
            "SPY_symbology_2025-04-02.txt",
            sSymbol, sDate, sExpiryDate,
            "SPY_cbbos_2025-04-02_17-30.txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 3 );

    REQUIRE( optionChain.getPuts().size() == 193 );
    REQUIRE( optionChain.getCalls().size() == 193 );
    std::string sPutKeys(bc::AppUtils::joinKeys(optionChain.getPuts(),","));
    std::string sCallKeys(bc::AppUtils::joinKeys(optionChain.getCalls(),","));
    REQUIRE( sPutKeys == "00375000,00380000,00385000,00390000,00395000,00400000,00405000,"
        "00410000,00415000,00420000,00425000,00430000,00435000,00440000,00445000,00450000,"
        "00455000,00460000,00465000,00469000,00470000,00471000,00472000,00473000,00474000,"
        "00475000,00476000,00477000,00478000,00479000,00480000,00481000,00482000,00483000,"
        "00484000,00485000,00486000,00487000,00488000,00489000,00490000,00491000,00492000,"
        "00493000,00494000,00495000,00496000,00497000,00498000,00499000,00500000,00505000,"
        "00510000,00511000,00512000,00513000,00514000,00515000,00516000,00517000,00518000,"
        "00519000,00520000,00521000,00522000,00523000,00524000,00525000,00526000,00527000,"
        "00528000,00529000,00530000,00531000,00532000,00533000,00534000,00535000,00536000,"
        "00537000,00538000,00539000,00540000,00541000,00542000,00543000,00544000,00545000,"
        "00546000,00547000,00547500,00548000,00549000,00550000,00551000,00552000,00552500,"
        "00553000,00554000,00555000,00556000,00557000,00557500,00558000,00559000,00560000,"
        "00561000,00562000,00563000,00564000,00565000,00566000,00567000,00568000,00569000,"
        "00570000,00571000,00572000,00573000,00574000,00575000,00576000,00577000,00578000,"
        "00579000,00580000,00581000,00582000,00583000,00584000,00585000,00586000,00587000,"
        "00588000,00589000,00590000,00591000,00592000,00592500,00593000,00594000,00595000,"
        "00596000,00597000,00597500,00598000,00599000,00600000,00601000,00602000,00602500,"
        "00603000,00604000,00605000,00606000,00607000,00607500,00608000,00609000,00610000,"
        "00611000,00612000,00612500,00613000,00614000,00615000,00616000,00617000,00618000,"
        "00619000,00620000,00621000,00622000,00623000,00624000,00625000,00626000,00627000,"
        "00630000,00635000,00640000,00645000,00650000,00655000,00660000,00665000,00670000,"
        "00675000,00680000,00685000,00690000,00695000,00700000");
    REQUIRE( sCallKeys == sPutKeys );
    auto counter = [](const std::map<std::string, bc::OptionChain::Record>& map,
        const std::string& comment) {
        return std::count_if(map.begin(), map.end(),
            [&comment](const auto& pair) {
                return pair.second.m_comment == comment;
            });
    };
    const bc::OptionChain::Record& callRecord615 = optionChain.getCalls().at("00615000");
    const bc::OptionChain::Record& callRecord620 = optionChain.getCalls().at("00620000");
    const bc::OptionChain::Record& callRecord625 = optionChain.getCalls().at("00625000");
    REQUIRE( callRecord615.anyBidAskValid() == false);
    REQUIRE( callRecord620.anyBidAskValid() == false);
    REQUIRE( callRecord625.anyBidAskValid() == false);
    // check all receive times
    std::function<bc::Timestamp(const bc::OptionChain::Record&)> getRecvTime =
    [](const bc::OptionChain::Record& record) {
        return record.m_recvTime;
    };
    std::list<bc::Timestamp> recvTimes =
        bc::OptionChain::Util::onAllRecords(optionChain, getRecvTime);
    bc::Timestamp minRecvTime = bc::DateUtils::makeTimestampZulu(
        2025, 4, 2, 17, 30, 0);
    bc::Timestamp maxRecvTime = bc::DateUtils::makeTimestampZulu(
        2025, 4, 2, 17, 30, 10);
    for (auto recvTime : recvTimes) {
        if (recvTime == bc::Timestamp{})
            continue;
        REQUIRE( recvTime >= minRecvTime );
        REQUIRE( recvTime <= maxRecvTime );
    }
    bc::Timestamp chainTime = optionChain.getChainTime();
    REQUIRE( chainTime >= minRecvTime );
    REQUIRE( chainTime <= maxRecvTime );
    // ny time is 4 hours behind zulu time when daylight saving time is in effect
    bc::Timestamp nyTime = bc::DateUtils::makeTimestamp(
        2025, 4, 4, 16, 0, 0);
    REQUIRE( nyTime == bc::DateUtils::makeTimestampZulu(
        2025, 4, 4, 20, 0, 0) );
    REQUIRE( optionChain.getExpiryTime(bc::DateUtils::m_nasdaqClose) == nyTime );
    // however, the expiry time is 5 hours behind zulu time when daylight saving is in effect
    bc::Timestamp nyTime2 = bc::DateUtils::makeTimestamp(
        2025, 2, 4, 16, 0, 0);
    REQUIRE( nyTime2 == bc::DateUtils::makeTimestampZulu(
        2025, 2, 4, 21, 0, 0) );
    double fContinuousRate = 0.05;
    double discountFactor = bc::OptionChain::Util::getDiscountFactor(
        optionChain, fContinuousRate, bc::DateUtils::m_nasdaqClose);
    REQUIRE( discountFactor == Catch::Approx(0.99971200468240784) );
    std::function<double(const bc::OptionChain::RecordMap::value_type&, 
        const bc::OptionChain::RecordMap::value_type&)> getParityRate =
        bc::OptionChain::Util::PutCallParityRate(discountFactor);
    auto parityRates = bc::OptionChain::Util::onAllPutCallRecords(
        optionChain, getParityRate);
    // verify that, judging from parity rates, the data comes from the same time period
    // specifically, the parity rate should fall about linearly with rising strikes
    for (auto parityRate : parityRates) {
        REQUIRE( parityRate.second >= 565.81 );
        REQUIRE( parityRate.second  <= 566.04 );
    }
    double avg4Parity = optionChain.getParityRate(fContinuousRate, bc::DateUtils::m_nasdaqClose);
    REQUIRE( avg4Parity == Catch::Approx(566.02214223079227) );
    double parityVariance = optionChain.getParityRateQualityScore(
        fContinuousRate, bc::DateUtils::m_nasdaqClose);
    REQUIRE( parityVariance == Catch::Approx(0.0019996671612625434) ); 
}


TEST_CASE( "Build BNO Option Chain from cbboMap", "[bnomapbuild]" ) {
    // load example of a bad option chain with very spotty data.
    std::string sSymbol("BNO");
    std::string sDate("2025-04-28");
    std::string sExpiryDate("2025-05-16");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChainFromCbboMap(
            sSymbol + "_symbologyResolution_" + sDate + ".txt",
            sSymbol, sDate, sExpiryDate,
            sSymbol + "_cbboMap_" + sDate + "_exp_" + sExpiryDate + ".txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 0 );
}

TEST_CASE( "Build QQQ Option Chain exp 2025-04-28 from cbboMap", "[qqq0428mapbuild]" ) {
    // load option chain
    std::string sSymbol("QQQ");
    std::string sDate("2025-04-28");
    std::string sExpiryDate("2025-04-28");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChainFromCbboMap(
            sSymbol + "_symbologyResolution_" + sDate + ".txt",
            sSymbol, sDate, sExpiryDate,
            sSymbol + "_cbboMap_" + sDate + "_exp_" + sExpiryDate + ".txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 0 );
    
}

TEST_CASE( "Build QQQ Option Chain exp 2025-04-29 from cbboMap", "[qqq0429mapbuild]" ) {
    // load option chain
    std::string sSymbol("QQQ");
    std::string sDate("2025-04-28");
    std::string sExpiryDate("2025-04-29");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChainFromCbboMap(
            sSymbol + "_symbologyResolution_" + sDate + ".txt",
            sSymbol, sDate, sExpiryDate,
            sSymbol + "_cbboMap_" + sDate + "_exp_" + sExpiryDate + ".txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 0 );
    
}

TEST_CASE( "Build QQQ Option Chain exp 2025-04-30 from cbboMap", "[qqq0430mapbuild]" ) {
    // load option chain
    std::string sSymbol("QQQ");
    std::string sDate("2025-04-28");
    std::string sExpiryDate("2025-04-30");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChainFromCbboMap(
            sSymbol + "_symbologyResolution_" + sDate + ".txt",
            sSymbol, sDate, sExpiryDate,
            sSymbol + "_cbboMap_" + sDate + "_exp_" + sExpiryDate + ".txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 0 );
    
}

TEST_CASE( "Build QQQ Option Chain exp 2025-05-01 from cbboMap", "[qqq0501mapbuild]" ) {
    // load option chain
    std::string sSymbol("QQQ");
    std::string sDate("2025-04-28");
    std::string sExpiryDate("2025-05-01");
    bc::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChainFromCbboMap(
            sSymbol + "_symbologyResolution_" + sDate + ".txt",
            sSymbol, sDate, sExpiryDate,
            sSymbol + "_cbboMap_" + sDate + "_exp_" + sExpiryDate + ".txt");
    REQUIRE( optionChain.getUnderlier() == sSymbol );
    REQUIRE( optionChain.getValuationDate() == sDate );
    REQUIRE( optionChain.getExpiryDate() == sExpiryDate );
    REQUIRE( optionChain.getMissingInstrumentIdToOsiMap().size() == 0 );
    
}
