#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "bentoclient/marketenvironmentextended.hpp"

namespace bc = bentoclient;

TEST_CASE( "Marketenvironment extended yield curve loading", "[yieldcurve]" ) {
    bc::MarketEnvironmentExtended me(0.04, bc::DateUtils::m_nasdaqClose,
        "./tests/data/TSY.2024-12-31.csv");
    bc::Timestamp valuationTime = bc::DateUtils::makeTimestampZulu("2024-02-03");
    bc::Timestamp expiryTime = bc::DateUtils::makeTimestampZulu("2024-10-03");
    double fRate = me.getRiskFreeRate(valuationTime, expiryTime);
    REQUIRE(fRate == Catch::Approx(0.051530415772052848).epsilon(1e-9));
    expiryTime = bc::DateUtils::makeTimestampZulu("2024-02-04");
    fRate = me.getRiskFreeRate(valuationTime, expiryTime);
    REQUIRE(fRate == Catch::Approx(0.054160008807484304).epsilon(1e-9));
    valuationTime = bc::DateUtils::makeTimestampZulu("2025-02-03");
    try {
        fRate = me.getRiskFreeRate(valuationTime, expiryTime);
        REQUIRE(false);
    } catch (const std::exception& e)
    {
        REQUIRE(e.what() == std::string("Expiry time 2024-02-04 00:00:00.000000000Z"
            " not after valuation time 2025-02-03 00:00:00.000000000Z"));
    }
    expiryTime = bc::DateUtils::makeTimestampZulu("2025-06-04");
    // takes last yield curve in me
    fRate = me.getRiskFreeRate(valuationTime, expiryTime);
    REQUIRE(fRate == Catch::Approx(0.042740051472385091).epsilon(1e-9));
    // takes first yield curve in me
    valuationTime = bc::DateUtils::makeTimestampZulu("2023-02-03");
    expiryTime = bc::DateUtils::makeTimestampZulu("2023-02-04");
    fRate = me.getRiskFreeRate(valuationTime, expiryTime);
    REQUIRE(fRate == Catch::Approx(0.054743893591701363).epsilon(1e-9));
    // test new data format having the 1.5 Month!
    bc::MarketEnvironmentExtended me2(0.04, bc::DateUtils::m_nasdaqClose,
        "./tests/data/TSY.2025-06-06.csv");
    // see that we pick the 1.5 months!
    valuationTime = bc::DateUtils::makeTimestampZulu("2025-02-03");
    expiryTime = bc::DateUtils::makeTimestampZulu("2025-03-18");
    fRate = me2.getRiskFreeRate(valuationTime, expiryTime);
    // in this case, since 1.5 Month is filled only from 2025-02-18, get
    // a result based on 1 Month rate.
    REQUIRE(fRate == Catch::Approx(0.043229419944815856).epsilon(1e-9));
    valuationTime = bc::DateUtils::makeTimestampZulu("2025-02-18");
    expiryTime = bc::DateUtils::makeTimestampZulu("2025-04-02");
    fRate = me2.getRiskFreeRate(valuationTime, expiryTime);
    // Now, result on 1.5 Month rate.
    REQUIRE(fRate == Catch::Approx(0.043620828527698212).epsilon(1e-9));
}