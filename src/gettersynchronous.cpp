#include "bentoclient/gettersynchronous.hpp"

using namespace bentoclient;


GetterSynchronous::GetterSynchronous(std::unique_ptr<databento::Historical>&& clientPtr) :
    m_clientPtr(std::move(clientPtr))
    {}


databento::SymbologyResolution GetterSynchronous::getSymbologyResolution(
    const std::string& dataSet,
    const std::string& sUnderlier, const std::string& sDate)
{
    return m_clientPtr->SymbologyResolve(dataSet, {sUnderlier + ".OPT"}, 
        databento::SType::Parent, databento::SType::InstrumentId,
        // the fixed time range means to catch symbology of a single trade day,
        // valid in its instrument mappings for that trade day.
        {sDate + "T01:00", sDate + "T23:30"});
}

std::list<databento::CbboMsg> GetterSynchronous::getCbboTimeseriesRange(
    const std::vector<std::string>& instrumentIds,
    const std::string& dataSet,
    databento::Schema schema,
    Timestamp at,
    TimeRange timeRange)
{
    std::list<databento::CbboMsg> cbboMsgs;
    // Instrument collections, like an option chain, will have timestamp of newest valid 
    // trade data. To meet, but not exceed desired {at}, add a little and then take the
    // sample time range as a lookback.
    TimeRange lookAhead = std::chrono::seconds(2);
    Timestamp to = at + lookAhead;
    Timestamp from = to - timeRange;
    auto push_cbbos = [&cbboMsgs](const databento::Record& record) {
        const auto& cbbo_msg = record.Get<databento::CbboMsg>();
        cbboMsgs.push_back(cbbo_msg);
        return databento::KeepGoing::Continue;
    };
    auto dump_symbols = [](const databento::Metadata& metadata) {    };
    m_clientPtr->TimeseriesGetRange(
        dataSet, {from, to}, instrumentIds,
        schema, databento::SType::InstrumentId, databento::SType::InstrumentId, 
        {}, dump_symbols,
        push_cbbos);
    return cbboMsgs;
}


