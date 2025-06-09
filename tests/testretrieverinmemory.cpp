#include <catch2/catch_test_macros.hpp>
#include "bentoclient/optionchain.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "bentoclient/retrieverinmemory.hpp"
#include "dataloader.hpp"
#include <cmath>

namespace {
    bentoclient::OptionChain shiftChainTime(
        const bentoclient::OptionChain& source,
        bentoclient::Timestamp targetChainTime)
    {
        bentoclient::OptionChain shiftedChain(source);
        std::function<int(const bentoclient::OptionChain::Record&)> shifter =
            [targetChainTime](const bentoclient::OptionChain::Record& record)
        {
            auto& nonconstRecord = const_cast<bentoclient::OptionChain::Record&>(record);
            nonconstRecord.m_recvTime = targetChainTime;
            return 0;
        };
        bentoclient::OptionChain::Util::onAllRecords(shiftedChain, shifter);
        return shiftedChain;
    }
}
TEST_CASE( "Retriever in memory test", "[retriever]" ) {
    
    auto timeRange = std::chrono::minutes(10);
    bentoclient::RetrieverInMemory retriever(timeRange);
    std::string sSymbol("SPY");
    std::string sDate("2025-04-02");
    std::string sExpiryDate("2025-04-04");

    bentoclient::Timestamp testTime = bentoclient::DateUtils::makeTimestamp(2025, 04, 02, 10, 30, 00);

    // Test the main retriever function look up nearby chains. 
       
    try
    {
        const auto& chain = retriever.getRawOptionChain(sSymbol, testTime, sExpiryDate);
        REQUIRE(false);
    }
    catch(const std::invalid_argument& e)
    {
        std::string cause(e.what());
        REQUIRE(cause == std::string("No option chain in acceptable time range"
            " for 2025-04-02 14:30:00.000000000 of SPY EXP 2025-04-04"));
    }
    
    // load option chain
    bentoclient::OptionChain optionChain =
        bentotests::DataLoader().buildOptionChain(
            "SPY_symbology_2025-04-02.txt",
            sSymbol, sDate, sExpiryDate,
            "SPY_cbbos_2025-04-02_17-30.txt");
    REQUIRE( optionChain.getPuts().size() == 193 );
    REQUIRE( optionChain.getCalls().size() == 193 );
    auto shifted1 = shiftChainTime(optionChain, testTime + std::chrono::nanoseconds(789));
    retriever.submitOptionChain(std::move(shifted1));
    auto& retrieved1 = retriever.getRawOptionChain(sSymbol, testTime, sExpiryDate);
    bentoclient::Timestamp chainTime = retrieved1.getChainTime();
    // verify that the test time is no longer exactly same as chain time
    // which proves the time lenient lookup works.
    REQUIRE(chainTime != testTime);
    // testTime > chainTime, so the uint64_t overflows, wchich casted is negative
    REQUIRE(std::abs(static_cast<int>((chainTime - testTime).count())) < 1000);
    // but lookup time outside leniency range does not  find  chain
    auto testTime2 = testTime - (timeRange + std::chrono::seconds(1));
    try
    {
        auto& retrieved2 = retriever.getRawOptionChain(sSymbol, testTime2, sExpiryDate);
        REQUIRE(false);
    }
    catch(const std::invalid_argument& e)
    {
        std::string cause(e.what());
        REQUIRE(cause == std::string("No option chain in acceptable time range"
            " for 2025-04-02 14:19:59.000000000 of SPY EXP 2025-04-04"));
    }

    // test the time lenient lookup method
    bentoclient::Retriever::TimeToChainMap ttm;

    // the case of empty TimeToChainMap shouldn't occur in production
    auto pair = retriever.getNextInTimeRange(testTime, ttm);
    REQUIRE(pair == std::make_pair(bentoclient::Timestamp{}, false));

    auto stepping = std::chrono::minutes(33);
    REQUIRE(stepping/2 > timeRange);
    for (bentoclient::Timestamp t = testTime + std::chrono::minutes(1); 
            t < testTime + std::chrono::hours(10); t += stepping)
    {
        ttm.emplace(t, std::move(shiftChainTime(optionChain, t)));
    }

    std::vector<bentoclient::Timestamp> tv = bentoclient::AppUtils::keyVector(ttm);

    // for time below {lower}, return first timestamp
    auto pair2 = retriever.getNextInTimeRange(testTime, ttm);
    REQUIRE(pair2 == std::make_pair(tv[0], true));
    // too far below, still first timestap, but flagged false
    auto pair3 = retriever.getNextInTimeRange(testTime - timeRange, ttm);
    REQUIRE(pair3 == std::make_pair(tv[0], false));
    // slightly above first timestep, return that and true
    auto pair4 = retriever.getNextInTimeRange(tv[0] + std::chrono::minutes(1), ttm);
    REQUIRE(pair4 == std::make_pair(tv[0], true));
    // too far above first timestep, return that and false
    auto pair5 = retriever.getNextInTimeRange(tv[0] + std::chrono::seconds(1) + timeRange, ttm);
    REQUIRE(pair5 == std::make_pair(tv[0], false));
    // closer to second timestamp, return that and false
    auto pair6 = retriever.getNextInTimeRange(tv[0] + stepping/2 + std::chrono::minutes(1), ttm);
    REQUIRE(pair6 == std::make_pair(tv[1], false));
    // in range below second timestamp, return that and true
    auto pair7 = retriever.getNextInTimeRange(tv[1] - timeRange + std::chrono::seconds(1), ttm);
    REQUIRE(pair7 == std::make_pair(tv[1], true));
    // in range above last timestamp, return that and true
    auto pair8 = retriever.getNextInTimeRange(tv.back() + std::chrono::minutes(1), ttm);
    REQUIRE(pair8 == std::make_pair(tv.back(), true));
    // out of range above last timestamp, return that and false
    auto pair9 = retriever.getNextInTimeRange(tv.back() + timeRange + std::chrono::seconds(1), ttm);
    REQUIRE(pair9 == std::make_pair(tv.back(), false));
}