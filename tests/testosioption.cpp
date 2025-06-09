#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <bentoclient/osioption.hpp>

TEST_CASE( "OSI Option", "[osiopt]" ) {
    // Test the OsiOption class
    std::string sOsiIdentifier0 = "SPY 240610X00123000";
    try {
        bentoclient::OsiOption osi0(sOsiIdentifier0);
        REQUIRE( false ); // Should not reach here
    } catch (const std::invalid_argument& e) {
        REQUIRE( std::string(e.what()) == "Invalid OSI identifier format: " + sOsiIdentifier0 );
    }
    std::string sOsiIdentifier1 = "SPY 240610C00123000";
    bentoclient::OsiOption osi1(sOsiIdentifier1);
    REQUIRE( osi1.getOsiIdentifier() == sOsiIdentifier1);
    REQUIRE( osi1.getUnderlier() == "SPY");
    REQUIRE( osi1.getExpiryDate() == "2024-06-10");
    REQUIRE( osi1.getType() == "C");
    REQUIRE( osi1.getStrikeDollars() == "00123");
    REQUIRE( osi1.getStrikeDecimal() == "000");
    REQUIRE( osi1.getStrike() == "123");
    REQUIRE( osi1.isCall() == true);
    REQUIRE( osi1.isPut() == false);
    std::string sOsiIdentifier2 = "SPY 240610P00123400";
    bentoclient::OsiOption osi2(sOsiIdentifier2);
    REQUIRE( osi2.getStrike() == "123.4");
    REQUIRE( osi2.isCall() == false);
    REQUIRE( osi2.isPut() == true);
    std::string sOsiIdentifier3 = "SPY 240610P00120040";
    bentoclient::OsiOption osi3(sOsiIdentifier3);
    REQUIRE( osi3.getStrike() == "120.04");
    std::string sOsiIdentifier4 = "SPY 240610P01120004";
    bentoclient::OsiOption osi4(sOsiIdentifier4);
    REQUIRE( osi4.getStrike() == "1120.004");
    // checks that incorrect format having dollar strike zero 
    // leads to predicted behaviour
    std::string sOsiIdentifier5 = "SPY 240610P00000004";
    bentoclient::OsiOption osi5(sOsiIdentifier5);
    REQUIRE( osi5.getStrike() == ".004");
    std::string sOsiIdentifier6 = "SPY 240610P00000000";
    bentoclient::OsiOption osi6(sOsiIdentifier6);
    REQUIRE( osi6.getStrike().empty());
    std::string sStrikeKey = bentoclient::OsiOption::toStrikeKey(1.0);
    REQUIRE( sStrikeKey == "00001000");
    sStrikeKey = bentoclient::OsiOption::toStrikeKey(10.01);
    REQUIRE( sStrikeKey == "00010010");
    sStrikeKey = bentoclient::OsiOption::toStrikeKey(10.0001);
    REQUIRE( sStrikeKey == "00010000");
    sStrikeKey = bentoclient::OsiOption::toStrikeKey(100000.0001);
    REQUIRE( sStrikeKey == "00000000");

}
