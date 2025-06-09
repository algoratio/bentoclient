#include "dataloader.hpp"
#include <fstream>
#include "bentoclient/bentoserializer.hpp"
#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/optionchain.hpp"

using namespace bentoclient;
using namespace bentotests;

databento::SymbologyResolution DataLoader::getSymbologyResolution(const std::string& fileName) const
{
    // m_clientPtr->SymbologyResolve("OPRA.PILLAR", {underlier + ".OPT"}, db::SType::Parent, db::SType::InstrumentId,
    //     {date + "T01:00", date + "T23:30"});
    //     std::ofstream ofs(underlier + "_symbology_" + date + ".txt");
    //     boost::archive::text_oarchive oa(ofs);
    //     oa << resolution;
    databento::SymbologyResolution resolution;
    std::ifstream istr(pathName(fileName));
    boost::archive::text_iarchive ia(istr);
    ia >> resolution;    
    return resolution;
}

std::list<databento::TradeMsg> DataLoader::getTradeMessages(const std::string& fileName) const
{
    std::list<databento::TradeMsg> trades;
    std::ifstream istr2(pathName(fileName));
    boost::archive::text_iarchive ia2(istr2);
    ia2 >> trades;
    return trades;
}

std::list<databento::CbboMsg> DataLoader::getCbboMessages(const std::string& fileName) const
{
    std::list<databento::CbboMsg> trades;
    std::ifstream istr2(pathName(fileName));
    boost::archive::text_iarchive ia2(istr2);
    ia2 >> trades;
    return trades;
}

std::map<std::string, std::list<databento::CbboMsg>> DataLoader::getMappedCbboMessages(const std::string& fileName) const
{
    // Files intercepted from "#if STREAM_DEBUG" in requestersynchronous
    std::map<std::string, std::list<databento::CbboMsg>> cbbos;
    std::ifstream ifs(pathName(fileName));
    boost::archive::text_iarchive ia(ifs);
    ia >> cbbos;
    return cbbos;
}


OptionInstruments DataLoader::getOptionInstruments(
    const std::string& symbologyFile
) const
{
    OptionInstruments instruments;
    databento::SymbologyResolution resolution = getSymbologyResolution(symbologyFile);
    instruments.insert(resolution);
    return instruments;
}

OptionInstruments DataLoader::getOptionInstruments(
    const std::string& symbologyFile,
    const std::string& sSymbol,
    const std::string& sDate,
    const std::string& sExpiryDate) const
{
    OptionInstruments instruments;
    databento::SymbologyResolution resolution = getSymbologyResolution(symbologyFile);
    instruments.insert(resolution);
    // the instruments the trade data is for
    OptionInstruments testInstruments =
        instruments.get(sSymbol, sDate, sExpiryDate);
    return testInstruments;
}

bentoclient::OptionChain DataLoader::buildOptionChain(
    const std::string& symbologyFile,
    const std::string& sSymbol,
    const std::string& sDate,
    const std::string& sExpiryDate,
    const std::string& sCbboMessages        
) const
{
    OptionInstruments testInstruments = getOptionInstruments(
        symbologyFile,
        sSymbol,
        sDate,
        sExpiryDate);
    // bento API call producing the trades series
    // m_clientPtr->TimeseriesGetRange(
    //     "OPRA.PILLAR", {sDate + "T17:30", sDate + "T17:30:10"}, symbols,
    //     db::Schema::Cbbo1s, db::SType::InstrumentId, db::SType::InstrumentId, {}, dump_symbols,
    //     push_trades);
    std::list<databento::CbboMsg> cbboMsgs = getCbboMessages(sCbboMessages);
    std::map<std::string, std::string> idToOsi = testInstruments.getInstrumentIdToOsiMap();
    OptionChain::InstrumentIdToCbboMap cbboMap = OptionChain::mapCbboMsgsToInstruments(
        std::move(cbboMsgs), idToOsi);
    OptionChain::RecordTimeline timeline = OptionChain::buildRecordTimeline(
        cbboMap, idToOsi, std::chrono::seconds(2));
    OptionChain::PutCallRecordMap putCallRecords = OptionChain::mapLatestBestInTimelineToRecord(timeline);
    return OptionChain::build(std::move(putCallRecords), testInstruments);
}

bentoclient::OptionChain DataLoader::buildOptionChainFromCbboMap(
    const std::string& symbologyFile,
    const std::string& sSymbol,
    const std::string& sDate,
    const std::string& sExpiryDate,
    const std::string& sCbboMap        
) const
{
    OptionInstruments testInstruments = getOptionInstruments(
        symbologyFile,
        sSymbol,
        sDate,
        sExpiryDate);
    std::map<std::string, std::string> idToOsi = testInstruments.getInstrumentIdToOsiMap();
    OptionChain::InstrumentIdToCbboMap cbboMap = getMappedCbboMessages(sCbboMap);
    OptionChain::RecordTimeline timeline = OptionChain::buildRecordTimeline(
        cbboMap, idToOsi, std::chrono::seconds(2));
    OptionChain::PutCallRecordMap putCallRecords = OptionChain::mapLatestBestInTimelineToRecord(timeline);
    return OptionChain::build(std::move(putCallRecords), testInstruments);
}

