#pragma once

// test case and development helpers to serialize and deserialize bento 
// data structures

#include <databento/symbology.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/map.hpp>

namespace bentoclient {
    namespace details {
        // This is a workaround for the fact that date::year_month_day is not
        // trivially serializable.
        /// @brief A serialization helper for test cases
        struct that_good_awful_ymd_shadow {
            unsigned short y_;
            unsigned char m_;
            unsigned char d_;
        };
        static_assert(sizeof(that_good_awful_ymd_shadow) == sizeof(date::year_month_day),
            "Size of shadow struct does not match size of date::year_month_day");
    }
}

namespace boost {
namespace serialization {

template <typename Archive>
void serialize(Archive& ar, date::year_month_day& date, const unsigned int version) {
    bentoclient::details::that_good_awful_ymd_shadow& shadow = 
        reinterpret_cast<bentoclient::details::that_good_awful_ymd_shadow&>(date);
    ar & shadow.y_;
    ar & shadow.m_;
    ar & shadow.d_;
}

template <typename Archive>
void serialize(Archive& ar, databento::MappingInterval& mappingInterval, const unsigned int version) {
    ar & mappingInterval.start_date;
    ar & mappingInterval.end_date;
    ar & mappingInterval.symbol;
}

template <typename Archive>
void serialize(Archive& ar, databento::SymbologyResolution& symres, const unsigned int version) {
    ar & symres.mappings;
    ar & symres.partial;
    ar & symres.not_found;
    ar & symres.stype_in;
    ar & symres.stype_out;
}

template <typename Archive>
void serialize(Archive& ar, databento::RecordHeader& recordHeader, const unsigned int version) {
    ar & recordHeader.length;
    ar & recordHeader.rtype;
    ar & recordHeader.publisher_id;
    ar & recordHeader.instrument_id;
    ar & recordHeader.ts_event;
}
template <typename Archive>
void serialize(Archive& ar, databento::UnixNanos& unixNanos, const unsigned int version) {
    uint64_t& repr_ = reinterpret_cast<uint64_t&>(unixNanos);
    ar & repr_;
}
template <typename Archive>
void serialize(Archive& ar, databento::TimeDeltaNanos& duration, const unsigned int version) {
    int32_t& repr_ = reinterpret_cast<int32_t&>(duration);
    ar & repr_;
}
template <typename Archive>
void serialize(Archive& ar, databento::FlagSet& flagSet, const unsigned int version) {
    std::uint8_t& repr_ = reinterpret_cast<std::uint8_t&>(flagSet);
    ar & repr_;
}
template <typename Archive>
void serialize(Archive& ar, databento::TradeMsg& tradeMsg, const unsigned int version) {
    ar & tradeMsg.hd;
    ar & tradeMsg.price;
    ar & tradeMsg.size;
    ar & tradeMsg.action;
    ar & tradeMsg.side;
    ar & tradeMsg.flags;
    ar & tradeMsg.depth;
    ar & tradeMsg.ts_recv;
    ar & tradeMsg.ts_in_delta;
    ar & tradeMsg.sequence;
}
template <typename Archive>
void serialize(Archive& ar, databento::ConsolidatedBidAskPair& bidAskPair, const unsigned int version) {
    ar & bidAskPair.bid_px;
    ar & bidAskPair.ask_px;
    ar & bidAskPair.bid_sz;
    ar & bidAskPair.ask_sz;
    ar & bidAskPair.bid_pb;
    ar & bidAskPair.reserved1;
    ar & bidAskPair.ask_pb;
    ar & bidAskPair.reserved2;
}
template <typename Archive>
void serialize(Archive& ar, databento::CbboMsg& cbboMsg, const unsigned int version) {
    ar & cbboMsg.hd;
    ar & cbboMsg.price;
    ar & cbboMsg.size;
    ar & cbboMsg.reserved1;
    ar & cbboMsg.side;
    ar & cbboMsg.flags;
    ar & cbboMsg.reserved2;
    ar & cbboMsg.ts_recv;
    ar & cbboMsg.reserved3;
    ar & cbboMsg.reserved4;
    ar & cbboMsg.levels;  
}
}   // namespace serialization
}   // namespace boost
