#include <catch2/catch_test_macros.hpp>
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/osioption.hpp"
#include "bentoclient/apputils.hpp"
#include "dataloader.hpp"

TEST_CASE( "SymbologyResolution Insert", "[syminsert]" ) {
    // Test the LiveClient class
    bentoclient::OptionInstruments instruments;
    databento::SymbologyResolution resolution = bentotests::DataLoader().getSymbologyResolution("SPY_symbology_2024-06-10.txt");
    REQUIRE( resolution.mappings.size() == 9326 );
    instruments.insert(resolution);
    REQUIRE( instruments.getUnmapped() != nullptr );
    REQUIRE( instruments.getUnmapped()->m_osiIdentifiers.size() == 0 );
    REQUIRE( instruments.getUnmapped()->m_invalidOsiIdentifiers.size() == 0 );
    REQUIRE( instruments.getUnmapped()->m_mappings.size() == 0 );
    REQUIRE( instruments.getMappings().size() == 1 );
    REQUIRE( bentoclient::AppUtils::joinList(instruments.getUnderliers(),",") == "SPY" );
    std::string valuationDates = bentoclient::AppUtils::joinList(
        instruments.getValuationDates("SPY"),",");
    REQUIRE( valuationDates == "2024-06-10" );
    std::string expiryDates = bentoclient::AppUtils::joinList(
        instruments.getExpiryDates("SPY", valuationDates),",");
    std::string expectedExpiryDates("2024-06-07,2024-06-10,2024-06-11,2024-06-12,2024-06-13,2024-06-14,"
        "2024-06-17,2024-06-18,2024-06-20,2024-06-21,2024-06-24,2024-06-28,2024-07-05,2024-07-12,"
        "2024-07-19,2024-07-26,2024-07-31,2024-08-16,2024-08-30,2024-09-20,2024-09-30,2024-10-18,"
        "2024-10-31,2024-11-15,2024-11-29,2024-12-20,2024-12-31,2025-01-17,2025-01-31,2025-03-21,"
        "2025-03-31,2025-06-20,2025-09-19,2025-12-19,2026-01-16,2026-06-18,2026-12-18");
    REQUIRE( expiryDates == expectedExpiryDates );
    std::string putStrikeKeys = bentoclient::AppUtils::joinList(
        instruments.getStrikeKeys("SPY",
        valuationDates, "2024-06-17", true), ",");
    std::string expectedPutStrikeKeys("00448000,00449000,00450000,00451000,00452000,"
        "00453000,00454000,00455000,00456000,00457000,00458000,00459000,00460000,"
        "00461000,00462000,00463000,00464000,00465000,00466000,00467000,00468000,"
        "00469000,00470000,00471000,00472000,00473000,00474000,00475000,00476000,"
        "00477000,00478000,00479000,00480000,00481000,00482000,00483000,00484000,"
        "00485000,00486000,00487000,00488000,00489000,00490000,00491000,00492000,"
        "00493000,00494000,00495000,00496000,00497000,00498000,00499000,00500000,"
        "00505000,00506000,00507000,00508000,00509000,00510000,00511000,00512000,"
        "00513000,00514000,00515000,00516000,00517000,00518000,00519000,00520000,"
        "00521000,00522000,00523000,00524000,00525000,00526000,00527000,00528000,"
        "00529000,00530000,00531000,00532000,00533000,00534000,00535000,00536000,"
        "00537000,00538000,00539000,00540000,00541000,00542000,00543000,00544000,"
        "00545000,00546000,00547000,00548000,00549000,00550000,00551000,00552000,"
        "00553000,00554000,00555000,00560000,00565000,00570000,00575000,00580000,"
        "00585000,00590000,00595000,00600000,00605000,00610000");
    REQUIRE( putStrikeKeys == expectedPutStrikeKeys );
    std::string putStrikes = bentoclient::AppUtils::joinList(
        instruments.getStrikes("SPY",
        valuationDates, "2024-06-17", true), ",");
    std::string expectedPutStrikes("448,449,450,451,452,453,454,455,456,457,458,"
        "459,460,461,462,463,464,465,466,467,468,469,470,471,472,473,474,475,476,"
        "477,478,479,480,481,482,483,484,485,486,487,488,489,490,491,492,493,494,"
        "495,496,497,498,499,500,505,506,507,508,509,510,511,512,513,514,515,516,"
        "517,518,519,520,521,522,523,524,525,526,527,528,529,530,531,532,533,534,"
        "535,536,537,538,539,540,541,542,543,544,545,546,547,548,549,550,551,552,"
        "553,554,555,560,565,570,575,580,585,590,595,600,605,610");
    REQUIRE( putStrikes == expectedPutStrikes );
    const auto& sampleMapping = instruments.getMappings().at("SPY")
        .at("2024-06-10").at("2024-06-17")->first.at("00531000");
    REQUIRE( sampleMapping.first == "SPY   240617P00531000" );
    REQUIRE( sampleMapping.second == "1375732232" );
    REQUIRE( instruments.contains("SPY", "2024-06-10", "2024-06-17") == true );
    REQUIRE( instruments.contains("SPY", "2024-06-10", "2024-12-31") == true );
    // check the subset contains only the requested Option Chain
    bentoclient::OptionInstruments optionInstrumentsSub =
        instruments.get("SPY", "2024-06-10", "2024-06-17");
    REQUIRE( optionInstrumentsSub.contains("SPY", "2024-06-10", "2024-06-17") == true );
    REQUIRE( optionInstrumentsSub.contains("SPY", "2024-06-10", "2024-12-31") == false );
    // check that the option chain wasn't copied
    const auto& sampleMapping2 = optionInstrumentsSub.getStrikeKeyPutCallMap(
        "SPY", "2024-06-10", "2024-06-17")->first.at("00531000");
    REQUIRE( &sampleMapping == &sampleMapping2 );
    // get the symbol mapping
    std::map<std::string, std::string> osiToInstrumentId =
        optionInstrumentsSub.getOsiToInstrumentIdMap("SPY", "2024-06-10", "2024-06-17");
    REQUIRE( osiToInstrumentId.size() == 230 );
    REQUIRE( osiToInstrumentId["SPY   240617P00531000"] == "1375732232" );
    // check that the object for a single option chain returns the right default data
    std::map<std::string, std::string> osiToInstrumentId2 =
        optionInstrumentsSub.getOsiToInstrumentIdMap();
    REQUIRE( osiToInstrumentId == osiToInstrumentId2 );
}

TEST_CASE( "Get next expiry dates for symbol", "[chainexpiry]" ) {
    bentoclient::OptionInstruments optionInstruments =
        bentotests::DataLoader().getOptionInstruments("SPY_symbology_2024-06-10.txt");
    std::string sSymbol("SPY");
    std::string sDate("2024-06-10");
    int nDte = 10;
    std::list<std::string> expiryDates = optionInstruments.getExpiryDatesForDTE(
        sSymbol, sDate, nDte);
    std::string datesStr = bentoclient::AppUtils::joinList(expiryDates);
    REQUIRE(datesStr == "2024-06-10,2024-06-11,2024-06-12,2024-06-13,2024-06-14,2024-06-17,"
        "2024-06-18,2024-06-20");
    std::list<std::string> nextDate = optionInstruments.getNextExpiryDate(sSymbol, sDate);
    std::string nextStr = bentoclient::AppUtils::joinList(nextDate);
    REQUIRE(nextStr == "2024-06-10,2024-06-11");
}

TEST_CASE( "Time series trade range samples", "[tsrangetrade]" ) {
    // Test the LiveClient class
    bentoclient::OptionInstruments instruments;
    databento::SymbologyResolution resolution = 
        bentotests::DataLoader().getSymbologyResolution("SPY_symbology_2024-06-10.txt");
    REQUIRE( resolution.mappings.size() == 9326 );
    instruments.insert(resolution);
    // the instruments the trade data is for
    std::string sSymbol("SPY");
    std::string sDate("2024-06-10");
    std::string sExpiryDate("2024-06-17");
    bentoclient::OptionInstruments testInstruments =
        instruments.get(sSymbol, sDate, sExpiryDate);
    // bento API call producing the trades series
    // m_clientPtr->TimeseriesGetRange(
    //     "OPRA.PILLAR", {sDate + "T17:30", sDate + "T17:30:10"}, symbols,
    //     db::Schema::Trades, db::SType::RawSymbol, db::SType::InstrumentId, {}, dump_symbols,
    //     push_trades);
    std::list<databento::TradeMsg> trades = bentotests::DataLoader().getTradeMessages("SPY_tradeMsg_2024-06-10_17-30.txt");
    REQUIRE( trades.size() == 9 );
    // instrument ID to OSI mapping
    std::map<std::string, std::string> instrumentIdToOsi =
        testInstruments.getInstrumentIdToOsiMap(sSymbol, sDate, sExpiryDate);
    for (auto trade : trades) {
        auto it = instrumentIdToOsi.find(std::to_string(trade.hd.instrument_id));
        if ( it != instrumentIdToOsi.end() ) {
            std::string osiIdentifier = it->second;
            try {
                // check the OSI identifier is valid
                bentoclient::OsiOption osiOption(osiIdentifier);
                REQUIRE( osiOption.getExpiryDate() == sExpiryDate );
            } catch (const std::invalid_argument& e) {
                // invalid OSI identifier
                REQUIRE( false );
            }
        } else {
            // instrument ID not found in the mapping
            REQUIRE( false );
        }
    }
}

TEST_CASE( "Time series cbbo1s range samples", "[tsrangecbbo]" ) {
    // Test the LiveClient class
    std::string sSymbol("SPY");
    std::string sDate("2025-04-02");
    std::string sExpiryDate("2025-04-04");
    bentoclient::OptionInstruments testInstruments = 
        bentotests::DataLoader().getOptionInstruments("SPY_symbology_2025-04-02.txt", 
        sSymbol, sDate, sExpiryDate);
    REQUIRE(testInstruments.getStrikeKeyPutCallMap().first.size() == 193);
    // bento API call producing the trades series
    // m_clientPtr->TimeseriesGetRange(
    //     "OPRA.PILLAR", {sDate + "T17:30", sDate + "T17:30:10"}, symbols,
    //     db::Schema::Cbbo1s, db::SType::InstrumentId, db::SType::InstrumentId, {}, dump_symbols,
    //     push_trades);
    std::list<databento::CbboMsg> trades = 
        bentotests::DataLoader().getCbboMessages("SPY_cbbos_2025-04-02_17-30.txt");
    REQUIRE( trades.size() == 3377 );
    // instrument ID to OSI mapping
    std::map<std::string, std::string> instrumentIdToOsi =
        testInstruments.getInstrumentIdToOsiMap();
    for (auto trade : trades) {
        auto it = instrumentIdToOsi.find(std::to_string(trade.hd.instrument_id));
        if ( it != instrumentIdToOsi.end() ) {
            std::string osiIdentifier = it->second;
            try {
                // check the OSI identifier is valid
                bentoclient::OsiOption osiOption(osiIdentifier);
                //std::cout << osiOption << std::endl;
                REQUIRE( osiOption.getExpiryDate() == sExpiryDate );
            } catch (const std::invalid_argument& e) {
                // invalid OSI identifier
                REQUIRE( false );
            }
        } else {
            // instrument ID not found in the mapping
            REQUIRE( false );
        }
    }
}
