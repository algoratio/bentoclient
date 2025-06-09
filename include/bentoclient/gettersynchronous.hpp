#pragma once

#include "bentoclient/getter.hpp"
#include <databento/historical.hpp>
#include <chrono>

namespace bentoclient
{
    /// @brief Implements a synchronous getter for databento
    class GetterSynchronous : public Getter
    {
    public:
        GetterSynchronous(std::unique_ptr<databento::Historical>&& clientPtr);

        databento::SymbologyResolution getSymbologyResolution(
            const std::string& dataSet,
            const std::string& sUnderlier, const std::string& sDate) override;

        std::list<databento::CbboMsg> getCbboTimeseriesRange(
            const std::vector<std::string>& instrumentIds,
            const std::string& dataSet,
            databento::Schema schema,
            Timestamp at,
            TimeRange timeRange) override;
    private:
        /// @brief underlying historical client
        std::unique_ptr<databento::Historical> m_clientPtr;
    };
}