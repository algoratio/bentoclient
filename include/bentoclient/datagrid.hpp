#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <type_traits>
#include <stdexcept>
#include <functional>
#include "bentoclient/dateutils.hpp"

namespace bentoclient
{
    /// @brief A basic data frame-like structure for spreadsheat formatting of outputs.
    class DataGrid
    {
    public:
        /// @brief Supported source data types for columns
        enum class DataType : int
        {
            GENERIC,
            INT,
            UINT,
            DOUBLE,
            STRING,
            TIMESTAMP
        };
        /// @brief Maps data types to their names for error messages
        const static std::map<DataType, std::string> m_dataTypeToStringMap;
        typedef int IntType;
        typedef unsigned int UIntType;
        typedef double DoubleType;
        /// @brief Get type enum for source data type
        /// @tparam T Source data type
        /// @return DataType enum val
        template <typename T>
        static DataType getDataType()
        {
            if constexpr (std::is_same_v<T, IntType>)
                return DataType::INT;
            else if constexpr (std::is_same_v<T, UIntType>)
                return DataType::UINT;
            else if constexpr (std::is_same_v<T, DoubleType>)
                return DataType::DOUBLE;
            else if constexpr (std::is_same_v<T, std::string>)
                return DataType::STRING;
            else if constexpr (std::is_same_v<T, Timestamp>)
                return DataType::TIMESTAMP;
            else
                return DataType::GENERIC;
        }
        // default fmtlib format strings for data types
        const static std::string m_defaultIntFormat;
        const static std::string m_defaultUIntFormat;
        const static std::string m_defaultDoubleFormat; 
        const static std::string m_defaultStringFormat; 
        const static std::string m_defaultTimestampFormat;
        const static std::string m_defaultDateFormat;
        const static std::string m_defaultTimeFormat;
        const static std::string m_defaultFormat;

        /// @brief Get a source data type specific formatter string
        /// @tparam T Source data type
        /// @return Formatter string for fmtlib::fmt
        template <typename T>
        static const std::string& getDefaultFormat()
        {
            DataType type = getDataType<T>();
            switch(type) {
            case DataType::INT:
                return m_defaultIntFormat;
            case DataType::UINT:
                return m_defaultUIntFormat;
            case DataType::DOUBLE:
                return m_defaultDoubleFormat;
            case DataType::STRING:
                return m_defaultStringFormat;
            case DataType::TIMESTAMP:
                return m_defaultTimestampFormat;
            default:
                return m_defaultFormat;
            }
        }
        /// @brief Cell base class allowing to subclass for different cell data types
        class Cell
        {
        public:
            Cell() = default;
            Cell(const Cell&) = delete;
            Cell& operator=(const Cell&) = delete;
            Cell(Cell&&) = default;
            Cell& operator=(Cell&&) = default;
            virtual ~Cell() = default;

            virtual std::string toString() const = 0;
            virtual DataType getType() const = 0;
        };
        /// @brief Generic cell value container handling string and arithmetic data types
        /// @tparam T Source data type (decayed!)
        template <typename T>
        class GenericCell : public Cell
        {
        public:
            GenericCell(const T& value) : m_value(value) {}
            GenericCell(T&& value) : m_value(std::move(value)) {}
            GenericCell& operator=(GenericCell&&) = default;
            ~GenericCell() override = default;
        public:
           std::string toString() const override {
                if constexpr (std::is_same_v<T, std::string>) {
                    return m_value; // For std::string, return as-is
                } else if constexpr (std::is_arithmetic_v<T>) {
                    return std::to_string(m_value); // For numeric types, use std::to_string
                } else if constexpr (std::is_same_v<T, Timestamp>) {
                    // For Timestamp, convert to string (example implementation)
                    return serializeTimestamp(m_value);
                } else {
                    // Fallback for other unsupported types
                    static_assert(!std::is_same_v<T, T>, "Unsupported type for toString");
                }
            }
            DataType getType() const override
            {
                return getDataType<T>();
            }
            T& get()
            {
                return m_value;
            }
            const T& get() const
            {
                return m_value;
            }
            void set(const T& value)
            {
                m_value = value;
            }
            void set (T&& value)
            {
                m_value = std::move(value);
            }
        private:
            T m_value;
        };
        /// @brief Formatter base class converting cell values to CSV field strings
        class Format
        {
        public:
            Format() :
            m_null(m_defaultNull)
            {}
            Format(const std::string& defaultNull) :
            m_null(defaultNull)
            {}
            Format(const Format&) = delete;
            Format& operator=(const Format&) = delete;
            Format(Format&&) = default;
            Format& operator=(Format&&) = default;
            virtual ~Format() = default;

            virtual std::string formatCell(const Cell& cell) const {
                return cell.toString();
            }
            std::string formatCell(const Cell* cell) const {
                if (cell)
                {
                    return formatCell(*cell);
                }
                return m_null;
            }
            void setNull(const std::string& null)
            {
                m_null = null;
            }
            virtual DataType getType() const
            {
                return DataType::GENERIC;
            }
        public:
            static std::string makeFmtString(const std::string& fmt)
            {
                return "{0:" + fmt + "}";
            }
        protected:
            std::string m_null;
        protected:
            /// @brief default value for empty cells
            static const std::string m_defaultNull;
            // total specializations avoid fmt library in header
            static std::string fmt(const std::string& format, IntType value);
            static std::string fmt(const std::string& format, UIntType value);
            static std::string fmt(const std::string& format, DoubleType value);
            static std::string fmt(const std::string& format, const std::string& value);
            static std::string fmt(const std::string& format, const Timestamp& value);
        };
        /// @brief Generic formatter delegating to fmtlib::fmt
        /// @tparam T Source data type for formatter
        template <typename T>
        class GenericFormat : public Format
        {
        public:
            GenericFormat() :
                Format(),
                m_fmt(makeFmtString(getDefaultFormat<T>()))
            {}
            GenericFormat(const std::string& fmt, 
                const std::string& defaultNull = m_defaultNull) :
                Format(defaultNull),
                m_fmt(makeFmtString(fmt))
            {}
            GenericFormat(const GenericFormat&) = delete;
            GenericFormat& operator=(const GenericFormat&) = delete;
            GenericFormat(GenericFormat&&) = default;
            GenericFormat& operator=(GenericFormat&&) = default;
            ~GenericFormat() override = default;

            std::string formatCell(const Cell& cell) const override
            {
                if (cell.getType() != getType())
                {
                    throw std::runtime_error("DataGrid: GenericFormat: cell type mismatch");
                }
                const GenericCell<T>& genericCell = 
                    static_cast<const GenericCell<T>&>(cell);
                return fmt(m_fmt, genericCell.get());
            }

            DataType getType() const override
            {
                return getDataType<T>();
            }
        protected:
            std::string m_fmt;
        };
        
        // Formatters for supported types.
        typedef GenericFormat<IntType> IntFormat;
        typedef GenericFormat<UIntType> UIntFormat;
        typedef GenericFormat<DoubleType> DoubleFormat;
        typedef GenericFormat<std::string> StringFormat;
        typedef GenericFormat<Timestamp> TimestampFormat;
        // Make Format functor
        typedef std::function<std::unique_ptr<Format>()> FormatFunctor;

        /// @brief TimestampFormatSeconds formats a timestamp in seconds without fractional
        /// seconds. It is used for formatting timestamps and shifts in local timezones.
        class TimestampFormatSeconds : public TimestampFormat
        {
        public:
            TimestampFormatSeconds() :
                TimestampFormat(),
                m_timeZone(DateUtils::Timezone::m_UTC)
            {}
            TimestampFormatSeconds(const std::string& fmt, 
                const std::string& timeZone = DateUtils::Timezone::m_UTC,
                const std::string& defaultNull = m_defaultNull) :
                TimestampFormat(fmt, defaultNull),
                m_timeZone(timeZone)
            {}
            TimestampFormatSeconds(const TimestampFormatSeconds&) = delete;
            TimestampFormatSeconds& operator=(const TimestampFormatSeconds&) = delete;
            TimestampFormatSeconds(TimestampFormatSeconds&&) = default;
            TimestampFormatSeconds& operator=(TimestampFormatSeconds&&) = default;
            ~TimestampFormatSeconds() override = default;

            std::string formatCell(const Cell& cell) const override;
        private:
            std::string m_timeZone;
        };
        /// @brief Header definition with name and data format
        class Header
        {
        public:
            Header() :
                m_name{}, 
                m_type(DataType::GENERIC), 
                m_format(std::make_unique<Format>())
                {}
            Header(const std::string & name) : 
                m_name(name),
                m_type(DataType::GENERIC), 
                m_format(std::make_unique<Format>())
                {}
            Header(const DataType dataType) : 
                m_name{},
                m_type(dataType), 
                m_format(std::make_unique<Format>())
                {}
            Header(const std::string& name, const DataType dataType) : 
                m_name(name),
                m_type(dataType), 
                m_format(std::make_unique<Format>())
                {}
            Header(const Header&) = delete;
            Header& operator=(const Header&) = delete;
            Header(Header&&) = default;
            Header& operator=(Header&&) = default;
            ~Header() = default;

            DataType getType() const
            {
                return m_type;
            }
            const std::string& getName() const
            {
                return m_name;
            }
            void setName(const std::string& name)
            {
                m_name = name;
            }
            void setName(std::string&& name)
            {
                m_name = std::move(name);
            }
            const Format& getFormat() const
            {
                return *m_format;
            }
            Format& getFormat()
            {
                return *m_format;
            }
            void setFormat(std::unique_ptr<Format>&& format)
            {
                m_format = std::move(format);
            }
        private:
            std::string m_name;
            DataType m_type;
            std::unique_ptr<Format> m_format;
        };
        typedef std::vector<std::unique_ptr<Cell>> Row;
        typedef std::vector<std::unique_ptr<Header>> HeaderRow;
        typedef std::vector<Row> DataGridType;
    public:
        DataGrid() = default;
        // Deleted copy constructor and assignment operator
        DataGrid(const DataGrid&) = delete;
        DataGrid& operator=(const DataGrid&) = delete;
        // Deleted move constructor and assignment operator
        DataGrid(DataGrid&&) = default;
        DataGrid& operator=(DataGrid&&) = default;
        ~DataGrid() = default;

        /// @brief Creates a cell from variadic argument
        /// @tparam T Source data type for cell
        /// @param value Source value for cell
        /// @return Unique ptr to data cell
        template<typename T>
        static std::unique_ptr<Cell> makeCell(T&& value)
        {
            // *NOTE*: this routine maps a variety of source types
            // to cell containers, avoiding reference types by the decay.
            // Without this fixed mapping of types, data may end up in
            // references to undefined.
            typedef std::decay_t<T> dT;
            if constexpr (std::is_same_v<dT, std::string>) {
                if constexpr (std::is_rvalue_reference_v<T&&>) {
                    // just for strings, we do emplace
                    return std::make_unique<GenericCell<std::string>>(std::move(value));
                } else {
                    return std::make_unique<GenericCell<std::string>>(value);
                }
            } else if constexpr (std::is_arithmetic_v<dT> ||
                                 std::is_same_v<dT, Timestamp>) {
                return std::make_unique<GenericCell<dT>>(value);
            } else if constexpr (std::is_same_v<dT, const char*> ||
                                 std::is_same_v<dT, char *>){
                return std::make_unique<GenericCell<std::string>>(std::string(value));
            } else {
                // Fallback for other unsupported types, including char[] and const char[]
                static_assert(!std::is_same_v<T, T>, "Unsupported type for makeCell");
            }
        }

        /// @brief Creates a row from variadic args
        /// @tparam ...Args Argument types
        /// @param ...args Variadic args
        /// @return New row
        template<typename... Args>
        static Row makeRow(Args&&... args)
        {
            Row row;
            // short syntax for recursive reduction
            (row.emplace_back(makeCell(std::forward<Args>(args))), ...);
            return row;
        }

        /// @brief Adds a data row initialized from variadic args
        /// @tparam ...Args Argument types
        /// @param ...args Variadic arguments
        /// @return Row index of new row
        template<typename... Args>
        std::size_t addRow(Args&&... args)
        {
            Row row = makeRow(std::forward<Args>(args)...);
            std::size_t nCols = m_headers.size();
            checkTypes(row);
            adjustGrid(row, nCols);
            m_data.push_back(std::move(row));
            return m_data.size() - 1;
        }

        /// @brief Inserts a new row initialized from variadic args
        /// @tparam ...Args Types of arguments
        /// @param at Insert index
        /// @param ...args Variadic arguments
        template<typename... Args>
        void insertRow(std::size_t at, Args&&... args)
        {
            if (at > m_data.size())
            {
                throw std::out_of_range("DataGrid: insertRow: index out of range");
            }
            Row row = makeRow(std::forward<Args>(args)...);
            std::size_t nCols = m_headers.size();
            checkTypes(row);
            adjustGrid(row, nCols);
            m_data.insert(m_data.begin() + at, std::move(row));
        }

        /// @brief Set the value of a specific cell in the data grid.
        /// @tparam Arg Type of the value to set.
        /// @param row Row index
        /// @param col Col index
        /// @param arg Value to set
        template<typename Arg>
        void setValue(std::size_t row, std::size_t col, const Arg& arg)
        {
            if (row >= m_data.size())
            {
                throw std::out_of_range("DataGrid: setValue: row index out of range");
            }
            if (col >= m_data[row].size())
            {
                throw std::out_of_range("DataGrid: setValue: column index out of range");
            }
            DataType headerType = m_headers[col]->getType();
            if (headerType != DataType::GENERIC && headerType != getDataType<Arg>())
            {
                std::string error = makeSetValueError(col, getDataType<Arg>());
                throw std::invalid_argument(error);
            }
            m_data[row][col] = std::make_unique<GenericCell<Arg>>(arg);
        }

        /// @brief Set data cell to empty
        /// @param row Row index
        /// @param col Col index
        void setNull(std::size_t row, std::size_t col) {
            if (row >= m_data.size())
            {
                throw std::out_of_range("DataGrid: setNull: row index out of range");
            }
            if (col >= m_data[row].size())
            {
                throw std::out_of_range("DataGrid: setValue: column index out of range");
            }
            m_data[row][col] = Row::value_type{};
        }

        /// @brief Inserts new column initialized to NULL
        /// @param at Column index to insert at
        /// @param type Data type of new column
        /// @param name Name of new column
        void insertNullColumn(std::size_t at, DataType type = DataType::GENERIC, 
            const std::string& name = "")
        {
            if (at > m_headers.size())
            {
                throw std::out_of_range("DataGrid: insertColumn: index out of range");
            }
            m_headers.insert(m_headers.begin() + at, std::make_unique<Header>(name, type));
            for (auto& row : m_data)
            {
                row.insert(row.begin() + at, Row::value_type{});
            }
        }

        /// @brief Inserts a new colummn and initializes with value
        /// @tparam Arg Data type of new column
        /// @param at Column index
        /// @param arg Value to initialize
        /// @param name Column name
        template <typename Arg>
        void insertColumn(std::size_t at, const Arg& arg, const std::string& name = "")
        {
            if (at > m_headers.size())
            {
                throw std::out_of_range("DataGrid: insertColumn: index out of range");
            }
            DataType dataType = GenericCell<Arg>(arg).getType();
            m_headers.insert(m_headers.begin() + at, std::make_unique<Header>(name, dataType));
            for (auto& row : m_data)
            {
                row.insert(row.begin() + at, std::make_unique<GenericCell<Arg>>(arg));
            }
        }

        /// @brief Adds a new column to the end of the column array and initializes all rows with given value
        /// @tparam Arg Data type of new column 
        /// @param arg Value to initialize
        /// @param name Name of new column
        /// @return Column index of new column
        template <typename Arg>
        std::size_t addColumn(const Arg& arg, const std::string& name = "")
        {
            insertColumn(m_headers.size(), arg, name);
            return m_headers.size() - 1;
        }

        /// @brief Access the internal header row
        /// @return Header row
        const HeaderRow& getHeaders() const
        {
            return m_headers;
        }

        /// @brief Set the value that the serializer returns for empty cells at a specific column.
        /// @param at Column index
        /// @param null Null value
        void setNullValue(std::size_t at, const std::string& null)
        {
            if (at >= m_headers.size())
            {
                throw std::out_of_range("DataGrid: setNullValue: index out of range");
            }
            m_headers[at]->getFormat().setNull(null);
        }

        /// @brief Sets the value that the serializer returns for empty cells.
        /// @param null Null value
        void setNullValue(const std::string& null)
        {
            for (auto& header : m_headers)
            {
                header->getFormat().setNull(null);
            }
        }

        /// @brief Set the format of a column header at a specific index
        /// @param at Column index
        /// @param format Unique pointer to formatter
        void setFormat(std::size_t at, std::unique_ptr<Format>&& format)
        {
            if (at >= m_headers.size())
            {
                throw std::out_of_range("DataGrid: setFormat: index out of range");
            }
            if (m_headers[at]->getType() != format->getType())
            {
                std::string error = makeSetFormatError(at, format->getType());
                throw std::invalid_argument(error);
            }
            m_headers[at]->setFormat(std::move(format));
        }

        /// @brief Set format for all columns matching the data type of FormatFunctor
        /// @param formatMaker Functor creating unique formatters
        void setFormat(FormatFunctor formatMaker) {
            std::unique_ptr<Format> fmt(formatMaker());
            for (auto& header : m_headers) {
                if (header->getType() == fmt->getType())
                {
                    header->setFormat(formatMaker());
                }
            }
        }

        /// @brief Get list of column data types
        /// @return List of data types
        std::list<DataType> getColTypes() const
        {
            std::list<DataType> types;
            for (const auto& header : m_headers)
            {
                types.push_back(header->getType());
            }
            return types;
        }
        
        /// @brief Set column name at specific cindes
        /// @param at Column index
        /// @param name Column name
        void setColName(std::size_t at, const std::string& name)
        {
            if (at >= m_headers.size())
            {
                throw std::out_of_range("DataGrid: setColName: index out of range");
            }
            m_headers[at]->setName(name);
        }

        /// @brief Set the names of column headers
        /// @param colNames List of column names
        void setColNames(const std::list<std::string> colNames)
        {
            auto headerIt = m_headers.begin();
            for (auto valueIt = colNames.begin(); 
                valueIt != colNames.end() && headerIt != m_headers.end(); ++valueIt)
            {
                (*headerIt)->setName(*valueIt);
                ++headerIt;
            }
        }

        /// @brief Get list of names of column headers
        /// @return List of names 
        std::list<std::string> getColNames() const
        {
            std::list<std::string> names;
            for (const auto& header : m_headers)
            {
                names.push_back(header->getName());
            }
            return names;
        }

        /// @brief Create column headers with name and data type
        /// @param headers list of header definitions
        void createHeaders(const std::list<std::pair<std::string, DataType>>& headers)
        {
            for (const auto& header : headers)
            {
                m_headers.emplace_back(std::make_unique<Header>(header.first, header.second));
            }
        }

        /// @brief Access internal data grid
        /// @return Data grid
        const DataGridType& getData() const
        {
            return m_data;
        }

        /// @brief String formatted version of the data grid.
        /// @return vector of vector of strings
        std::vector<std::vector<std::string>> getStringGrid() const;

        /// @brief High performace serialization avoiding intermediate grid
        /// @param ostr output stream to write to  
        /// @param separator CSV separator
        /// @param lineFeed CSV line feed
        void serializeStringGrid(std::ostream& ostr, 
            const std::string& separator = m_defaultSeparator,
            const std::string& lineFeed = m_lineFeed) const;

        /// @brief Serialize the header row of CSV file
        /// @param ostr output stream to write to 
        /// @param separator CSV separator
        /// @param lineFeed CSV line feed
        void serializeHeaderRow(std::ostream& ostr,
            const std::string& separator = m_defaultSeparator,
            const std::string& lineFeed = m_lineFeed) const;

     private:
        /// @brief Generate a type mismatch error message with serialized types on setting a format
        /// @param at Column index for the error
        /// @param type Attempted data type
        /// @return error message
        std::string makeSetFormatError(std::size_t at, DataType type) const;

        /// @brief Generate a type mismatch error message with serialized types on setting a value
        /// @param at Column index for the error
        /// @param type Attempted data type
        /// @return error message
        std::string makeSetValueError(std::size_t at, DataType type) const;
 
        /// @brief Adjust the data grid after size changes
        /// @param row A new row to be inserted
        /// @param nCols Number of grid columns before the insert
        void adjustGrid(Row& row, std::size_t nCols);

        /// @brief Check data types of row elements before adding them to grid
        /// @param row Data row to be inserted
        void checkTypes(const Row& row);


    private:
        /// @brief The data held in the data grid
        DataGridType m_data;
        /// @brief The header row, matching the length of all data rows
        HeaderRow m_headers;
    public:
        // default field separator for CSV output streaming
        static const std::string m_defaultSeparator;
        // default line feed for CSV lines
        static const std::string m_lineFeed;
    };
}

std::ostream& operator<<(std::ostream& os, const bentoclient::DataGrid::DataType& dataType);


