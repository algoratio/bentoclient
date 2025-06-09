#pragma once
#include <databento/record.hpp>
#include <databento/symbology.hpp>
#include <databento/enums.hpp>
#include "bentoclient/clienttypes.hpp"
namespace bentoclient
{
    /// @brief Interface for getting data from bento
    class Getter
    {
    public:
        Getter() = default;
        Getter(const Getter&) = delete;
        Getter& operator = (const Getter&) = delete;
        Getter(Getter&&) = default;
        Getter& operator = (Getter&&) = default;
        virtual ~Getter() = default;

        /// @brief Get an options symbology resolution for an underlier
        /// @param dataSet Data set such as OPRA.PILLAR
        /// @param sUnderlier Underlier, such as SPY
        /// @param sDate Valuation date of options
        /// @return Symbology for underlier and valuation date (valid on that date only)
        virtual databento::SymbologyResolution getSymbologyResolution(
            const std::string& dataSet,
            const std::string& sUnderlier, const std::string& sDate) = 0;

        /// @brief Get a CBBO timeseries for a set of instruments
        /// @param instrumentIds A vector of instrument IDs
        /// @param dataSet A dataset, such as OPRA.PILLAR
        /// @param schema Schema, such as Cbbo1S
        /// @param at Timestamp to serve as historic "now"
        /// @param timeRange Time range before "now" to get data for
        /// @return The timeseries matching the search criteria
        virtual std::list<databento::CbboMsg> getCbboTimeseriesRange(
            const std::vector<std::string>& instrumentIds,
            const std::string& dataSet,
            databento::Schema schema,
            bentoclient::Timestamp at,
            TimeRange timeRange) = 0;

        /// @brief brings timestamps into a string format for databento calls
        static std::string format(bentoclient::Timestamp timestamp);
        
    };
}