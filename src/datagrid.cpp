#include "bentoclient/datagrid.hpp"
#include <sstream>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <chrono>

using namespace bentoclient;
const std::map<DataGrid::DataType, std::string> DataGrid::m_dataTypeToStringMap = {
    {DataGrid::DataType::GENERIC, "GENERIC"},
    {DataGrid::DataType::INT, "INT"},
    {DataGrid::DataType::UINT, "UINT"},
    {DataGrid::DataType::DOUBLE, "DOUBLE"},
    {DataGrid::DataType::STRING, "STRING"},
    {DataGrid::DataType::TIMESTAMP, "TIMESTAMP"}
};

const std::string DataGrid::Format::m_defaultNull("{null}");
const std::string DataGrid::m_defaultSeparator(",");
const std::string DataGrid::m_lineFeed("\n");
const std::string DataGrid::m_defaultIntFormat{};
const std::string DataGrid::m_defaultUIntFormat{};
const std::string DataGrid::m_defaultDoubleFormat{".2f"};
const std::string DataGrid::m_defaultStringFormat{};
const std::string DataGrid::m_defaultTimestampFormat("%Y-%m-%d %H:%M:%S");
const std::string DataGrid::m_defaultDateFormat("%Y-%m-%d");
const std::string DataGrid::m_defaultTimeFormat("%H:%M:%S");
const std::string DataGrid::m_defaultFormat{};

std::string DataGrid::Format::fmt(const std::string& format, IntType value)
{
    return fmt::format(format, value);
}
std::string DataGrid::Format::fmt(const std::string& format, UIntType value)
{
    return fmt::format(format, value);
}
std::string DataGrid::Format::fmt(const std::string& format, DoubleType value)
{
    return fmt::format(format, value);
}
std::string DataGrid::Format::fmt(const std::string& format, const std::string& value)
{
    return fmt::format(format, value);
}
std::string DataGrid::Format::fmt(const std::string& format, const Timestamp& value)
{
    return fmt::format(format, value);
}



std::string DataGrid::TimestampFormatSeconds::formatCell(const Cell& cell) const
{
    if (cell.getType() != getType())
    {
        throw std::runtime_error("DataGrid: TimestampFormatSeconds: cell type mismatch");
    }
    const GenericCell<Timestamp>& timestampCell = 
        static_cast<const GenericCell<Timestamp>&>(cell);
    // auto timePoint = timestampCell.get();
    // current version of fmt outputs fractional seconds
    // but we don't want that here.
    auto timePoint = timestampCell.get();
    if (timePoint == Timestamp{})
        return this->m_null;
    auto localTimePoint = DateUtils::convertZuluToTimestamp(timePoint, m_timeZone);
    auto timePointSeconds = std::chrono::time_point_cast<std::chrono::seconds>(localTimePoint);
    return fmt::format(m_fmt, timePointSeconds);
}

std::ostream& operator<<(std::ostream& os, const DataGrid::DataType& dataType)
{
    auto it = DataGrid::m_dataTypeToStringMap.find(dataType);
    if (it != DataGrid::m_dataTypeToStringMap.end()) {
        os << it->second;
    } else {
        os << "UNKNOWN";
    }
    return os;
}

void DataGrid::adjustGrid(Row& row, std::size_t nCols)
{
    if (row.size() < nCols)
    {
        while (row.size() < nCols)
        {
            row.emplace_back(Row::value_type{});
        }
    } else if (row.size() > nCols)
    {
        for (auto& orow : m_data)
        {
            while (orow.size() < row.size())
            {
                orow.emplace_back(Row::value_type{});
            }
        }
    }
}

void DataGrid::checkTypes(const Row& row)
{
    std::uint64_t headerIdx = 0;
    HeaderRow newHeaders;
    for (auto it = row.begin(); it != row.end(); ++it)
    {
        if (headerIdx < m_headers.size())
        {
            // header exists
            DataType headerType = m_headers.at(headerIdx)->getType();
            if (headerType != DataType::GENERIC && headerType != (*it)->getType())
            {
                std::ostringstream os;
                os << "Header type mismatch. Row[" << headerIdx << "] = " <<
                    (*it)->getType() << ", Header[" << headerIdx << "] = " <<
                    headerType;
                throw std::invalid_argument(os.str());
            }
        }
        else
        {
            newHeaders.emplace_back(std::make_unique<Header>((*it)->getType()));
        }
        ++headerIdx;
    }
    if (!newHeaders.empty())
    {
        m_headers.insert(m_headers.end(),
                     std::make_move_iterator(newHeaders.begin()),
                     std::make_move_iterator(newHeaders.end()));
    }
}

std::vector<std::vector<std::string>> DataGrid::getStringGrid() const
{
    std::vector<std::vector<std::string>> stringGrid;
    stringGrid.reserve(m_data.size());
    for (const auto& row : m_data)
    {
        std::vector<std::string> stringRow;
        stringRow.reserve(m_headers.size());
        if (m_headers.size() != row.size())
        {
            throw std::runtime_error("DataGrid: getStringGrid: row size mismatch");
        }
        for (std::size_t i = 0; i < m_headers.size(); ++i)
        {
            stringRow.emplace(stringRow.begin() + i, 
                m_headers[i]->getFormat().formatCell(row[i].get()));
        }
        stringGrid.emplace_back(stringRow);
    }
    return stringGrid;
}

void DataGrid::serializeStringGrid(std::ostream& ostr, 
    const std::string& separator,
    const std::string& lineFeed) const
{
    for (const auto& row : m_data)
    {
        if (row.size() != m_headers.size()) {
            throw std::runtime_error("DataGrid::serializeStringGrid: corrupted grid structure");
        }
        auto headerIt = m_headers.begin();
        for (auto it = row.begin(); it != row.end(); ++it)
        {
            if (it != row.begin())
            {
                ostr << separator;
            }
            ostr << (*headerIt++)->getFormat().formatCell((*it).get());
        }
        ostr << lineFeed;
    }
}

void DataGrid::serializeHeaderRow(std::ostream& ostr, 
    const std::string& separator,
    const std::string& lineFeed) const
{
    auto colNames = getColNames();
    for (auto it = colNames.begin(); it != colNames.end(); ++it) {
        if (it != colNames.begin())
        {
            ostr << separator;
        }
        ostr << *it;
    }
    ostr << lineFeed;
}


std::string DataGrid::makeSetFormatError(std::size_t at, DataType type) const
{
    std::ostringstream os;
    os << "DataGrid: setFormat: type mismatch. Trying to set format for " << type <<
         ", but Header[" << at << "] is of type " <<
        m_headers[at]->getType();
    return os.str();
}

std::string DataGrid::makeSetValueError(std::size_t at, DataType type) const
{
    std::ostringstream os;
    os << "DataGrid: setValue: type mismatch. Trying to set value for " << type <<
        ", but Header[" << at << "] is of type " <<
        m_headers[at]->getType();
    return os.str();
}
