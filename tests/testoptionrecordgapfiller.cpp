#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "bentoclient/optionrecordgapfiller.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "dataloader.hpp"

namespace bc = bentoclient;
#define REQUIRE_MESSAGE(cond, msg) do { INFO(msg); REQUIRE(cond);} while((void)0,0)
namespace{
    void emptyRecordsUntil(const bc::OptionChain::RecordMap& constRecordMap, const std::string& untilKey)
    {
        auto& recordMap = const_cast<bc::OptionChain::RecordMap&>(constRecordMap);
        auto untilIt = recordMap.find(untilKey);
        if (untilIt != recordMap.end())
        {
            for (auto it = recordMap.begin(); it != untilIt; ++it)
                it->second = bc::OptionChain::Record();
        }
    }
    void emptyRecordsBetween(const bc::OptionChain::RecordMap& constRecordMap, const std::string& fromKey, const std::string& untilKey)
    {
        auto& recordMap = const_cast<bc::OptionChain::RecordMap&>(constRecordMap);
        auto fromIt = recordMap.find(fromKey);
        auto untilIt = recordMap.find(untilKey);
        if (untilIt != recordMap.end() && fromIt != recordMap.end())
        {
            for (auto it = fromIt; it != untilIt; ++it)
                it->second = bc::OptionChain::Record();
        }
    }
    void emptyRecordsFrom(const bc::OptionChain::RecordMap& constRecordMap, const std::string& fromKey)
    {
        auto& recordMap = const_cast<bc::OptionChain::RecordMap&>(constRecordMap);
        auto fromIt = recordMap.find(fromKey);
        if (fromIt != recordMap.end())
        {
            for (auto it = fromIt; it != recordMap.end(); ++it)
                it->second = bc::OptionChain::Record();
        }
    }
    void emptyRecordAt(const bc::OptionChain::RecordMap& constRecordMap, const std::string& atKey)
    {
        auto& recordMap = const_cast<bc::OptionChain::RecordMap&>(constRecordMap);
        auto it = recordMap.find(atKey);
        if (it != recordMap.end())
            it->second = bc::OptionChain::Record();
    }

    struct RecordCmp
    {
        RecordCmp(double ask, double bid, const std::string& comment) :
            m_ask(ask),
            m_bid(bid),
            m_comment(comment)
        {}
        RecordCmp(const bc::OptionChain::Record& record) :
            m_ask(std::get<0>(record.m_askPrice)),
            m_bid(std::get<0>(record.m_bidPrice)),
            m_comment(record.m_comment)
        {}

        bool operator == (const RecordCmp& other) const
        {
            return this == &other
                || (m_ask == Catch::Approx(other.m_ask).epsilon(1e-6)
                &&  m_bid == Catch::Approx(other.m_bid).epsilon(1e-6)
                && m_comment == other.m_comment);
        }
        bool operator != (const RecordCmp& other) const
        {
            return !operator == (other);
        }

        double m_ask;
        double m_bid;
        std::string m_comment;
    };
    typedef std::map<std::string, RecordCmp> RecordCmpMap;
    std::pair<std::string, bool> checkRecords(const bc::OptionChain::RecordMap& recordMap,
        const RecordCmpMap& recordCmpMap)
    {
        for (auto it = recordCmpMap.begin(); it != recordCmpMap.end(); ++it)
        {
            auto recordIt = recordMap.find(it->first);
            if (recordIt == recordMap.end())
                return {it->first, false};
            RecordCmp cmp(recordIt->second);
            if (cmp != it->second)
                return {it->first, false};
        }
        return {std::string{}, true};
    }
    std::pair<std::string, bool> checkEmpty(const bc::OptionChain::RecordMap& recordMap, std::list<std::string> keys)
    {
        for (auto& key : keys)
        {
            auto it = recordMap.find(key);
            if (it == recordMap.end())
                return {key, false};
            if (!it->second.empty())
                return {key, false};
        }
        return {std::string{}, true};
    }
}


TEST_CASE( "Record Gap Filler SPY 2025-04-02", "[gapfillerspy0402]" ) {
    // load option chain
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
    // GAP FILLER
    bc::OptionChain gapChain(optionChain);
    // Mark a call at strike 500 as lacking data to trigger gap interpolation
    std::string testKey = "00500000";
    std::string otmPutKey = "00375000";
    std::string otmCallKey = "00700000";
    bc::OptionChain::Record orec = gapChain.getCalls().at(testKey);
    bc::OptionChain::Record ootmput = gapChain.getPuts().at(otmPutKey);
    bc::OptionChain::Record ootmcall = gapChain.getCalls().at(otmCallKey);
    std::get<1>(const_cast<bc::OptionChain::Record&>(
        gapChain.getCalls().at(testKey)).m_bidPrice) = 0;
    double fRiskFreeRate = 0.04;
    auto marketEnvironment(std::make_shared<bc::MarketEnvironment>(fRiskFreeRate, bc::DateUtils::m_nasdaqClose));
    bc::OptionRecordGapFiller gapFiller(marketEnvironment);
    bc::OptionChain filled = gapFiller.fillGaps(gapChain);
    // because of the invalid bid price, the strike 500 call option should now have estimated
    // bid and ask that should be close to original values.
    bc::OptionChain::Record nrec = filled.getCalls().at(testKey);
    // and the time should come from previous record
    std::string prevKey = "00499000";    
    bc::OptionChain::Record porec = filled.getCalls().at(prevKey);
    // check the modified record has the time stamp from previous record
    REQUIRE(porec.m_recvTime == nrec.m_recvTime);
    // check the original optionChain didin't change
    REQUIRE(orec.m_bidPrice == optionChain.getCalls().at(testKey).m_bidPrice);
    REQUIRE(orec.m_askPrice == optionChain.getCalls().at(testKey).m_askPrice);
    // check the approximation bid is different from the original, but ask remained same
    REQUIRE(nrec.m_bidPrice != orec.m_bidPrice);
    REQUIRE(nrec.m_askPrice == orec.m_askPrice);
    REQUIRE(nrec.m_comment == bc::OptionRecordGapFiller::m_spreadFitComment);
    // check the approximation is 0.1% close to the original
    REQUIRE(std::get<0>(nrec.m_bidPrice) == Catch::Approx(std::get<0>(orec.m_bidPrice)).epsilon(1e-3));
    // verify that the otm put wasn't valid before and is valid now
    REQUIRE(!ootmput.bidAskValid());
    REQUIRE(filled.getPuts().at(otmPutKey).bidAskValid());
    // verify that the otm call wasn't valid before and is valid now
    REQUIRE(!ootmcall.bidAskValid());
    REQUIRE(filled.getCalls().at(otmCallKey).bidAskValid());
    // check invalidating the put side
    bc::OptionChain gapChain2(optionChain);
    bc::OptionChain::Record orec2 = gapChain2.getPuts().at(testKey);
    std::get<1>(const_cast<bc::OptionChain::Record&>(
        gapChain2.getPuts().at(testKey)).m_bidPrice) = 0;
    bc::OptionChain filled2 = gapFiller.fillGaps(gapChain2);
    // check that the routine didn't touch the call side
    REQUIRE(orec.m_bidPrice == filled2.getCalls().at(testKey).m_bidPrice);
    REQUIRE(orec.m_askPrice == filled2.getCalls().at(testKey).m_askPrice);
    // check that it worked on the put side
    bc::OptionChain::Record nrec2 = filled2.getPuts().at(testKey);
    REQUIRE(orec2.m_bidPrice != nrec2.m_bidPrice);
    REQUIRE(orec2.m_askPrice == nrec2.m_askPrice);
    // and that the extimates are 1 cents precision
    REQUIRE(std::get<0>(orec2.m_bidPrice) == Catch::Approx(std::get<0>(nrec2.m_bidPrice)).epsilon(0).margin(0.01));


}

TEST_CASE( "Record Gap Filler BNO 2025-04-28", "[gapfillerbno0428]" ) {
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
    // GAP FILLER
    bc::OptionChain gapChain(optionChain);
    // kill OTM puts and calls
    emptyRecordsUntil(gapChain.getPuts(), "00022000");
    emptyRecordsFrom(gapChain.getCalls(), "00038000");
    double fRiskFreeRate = 0.04;
    auto marketEnvironment(std::make_shared<bc::MarketEnvironment>(fRiskFreeRate, bc::DateUtils::m_nasdaqClose));
    bc::OptionRecordGapFiller gapFiller(marketEnvironment);
    // gap filler should now resort to log-linear extrapolation.
    bc::OptionChain filled = gapFiller.fillGaps(gapChain);
    auto pair = checkRecords(filled.getPuts(),
    {
        { "00019000", {0.18783830512776567, 0.0, bc::OptionRecordGapFiller::m_logExtrapolateComment}},
        { "00020000", {0.20759932013042498, 0.0, bc::OptionRecordGapFiller::m_logExtrapolateComment}},
        { "00021000", {0.23466718356300154, 0.0, bc::OptionRecordGapFiller::m_logExtrapolateComment}},
        { "00022000", {0.75, 0.4812087912087924, bc::OptionRecordGapFiller::m_spreadFitComment}}
    }
    );
    REQUIRE_MESSAGE(pair.second == true, "Record mismatch at strike key: " + pair.first);
    pair = checkRecords(filled.getCalls(),
    {
        { "00037000", {0.75, 0.74, bc::OptionRecordGapFiller::m_spreadFitComment}},
        { "00038000", {0.13263823595627638, 0.12263823595627638, bc::OptionRecordGapFiller::m_logExtrapolateComment}},
        { "00039000", {0.10992748944636097, 0.099927489446360956, bc::OptionRecordGapFiller::m_logExtrapolateComment}},
        { "00040000", {0.091257679440882353, 0.081257679440882344, bc::OptionRecordGapFiller::m_logExtrapolateComment}}
    }
    );
    REQUIRE_MESSAGE(pair.second == true, "Record mismatch at strike key: " + pair.first);
    
    // Now add in a PCP call gap
    emptyRecordAt(gapChain.getCalls(), "00028000");
    filled = gapFiller.fillGaps(gapChain);
    pair = checkRecords(filled.getCalls(),
    {
        { "00027000", {1.35, 1.05, std::string{}}},
        { "00028000", {0.7864697802197782, 0.56146978021977811, bc::OptionRecordGapFiller::m_pcpFitComment}},
        { "00029000", {0.45, 0.3, std::string{}}},
    }
    );
    REQUIRE_MESSAGE(pair.second == true, "Record mismatch at strike key: " + pair.first);

    // Now add in a PCP put gap
    gapChain = optionChain;
    emptyRecordAt(gapChain.getPuts(), "00028000");
    filled = gapFiller.fillGaps(gapChain);
    pair = checkRecords(filled.getPuts(),
    {
        { "00027000", {0.95, 0.6, std::string{}}},
        { "00028000", {1.3169604739183456, 0.9669604739183455, bc::OptionRecordGapFiller::m_pcpFitComment}},
        { "00029000", {2.0, 1.65, std::string{}}},
    }
    );
    REQUIRE_MESSAGE(pair.second == true, "Record mismatch at strike key: " + pair.first);
}

TEST_CASE( "Record Gap Filler BNO 2025-04-29", "[gapfillerqqq0429]" ) {
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

    double fRiskFreeRate = 0.04;
    double parRate = optionChain.getParityRate(fRiskFreeRate, bc::DateUtils::m_nasdaqClose);
    REQUIRE(parRate == Catch::Approx(471.61153578408215).epsilon(1e-6));

    // now blank out options around the par rate
    bc::OptionChain gapChain(optionChain);
    std::string fromKey("00468000"), toKey("00474000");
    emptyRecordsBetween(gapChain.getPuts(), fromKey, toKey);
    emptyRecordsBetween(gapChain.getCalls(), fromKey, toKey);
    auto marketEnvironment(std::make_shared<bc::MarketEnvironment>(fRiskFreeRate, bc::DateUtils::m_nasdaqClose));
    bc::OptionRecordGapFiller gapFiller(marketEnvironment);
    // gap filler should now bail out because of a bad chain around parity
    bc::OptionChain filled = gapFiller.fillGaps(gapChain);
    std::list<std::string> keyList = {
        "00468000",
        "00469000",
        "00470000",
        "00471000",
        "00472000",
        "00473000"
    };
    auto pair = checkEmpty(gapChain.getPuts(), keyList);
    REQUIRE_MESSAGE(pair.second == true, "Nonempty value for key " + pair.first);
    pair = checkEmpty(gapChain.getCalls(), keyList);
    REQUIRE_MESSAGE(pair.second == true, "Nonempty value for key " + pair.first);
}
