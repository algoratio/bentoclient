#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "bentoclient/datagrid.hpp"
#include <sstream>

TEST_CASE( "Data grid operations", "[datagrid]" ) {
    auto getHeadCsv = [](const bentoclient::DataGrid& grid)
    {
        std::ostringstream ostr;
        grid.serializeHeaderRow(ostr);
        return ostr.str();
    };
    auto getGridCsv = [](const bentoclient::DataGrid& grid)
    {
        std::ostringstream ostr;
        grid.serializeStringGrid(ostr);
        return ostr.str();
    };
    {
    bentoclient::DataGrid grid;

    grid.addRow(1, 2U, 3.0, std::string("Hello"), 
        bentoclient::Timestamp(std::chrono::nanoseconds(1234567890)));

    std::list<bentoclient::DataGrid::DataType> expectedTypes = {
        bentoclient::DataGrid::DataType::INT,
        bentoclient::DataGrid::DataType::UINT,
        bentoclient::DataGrid::DataType::DOUBLE,
        bentoclient::DataGrid::DataType::STRING,
        bentoclient::DataGrid::DataType::TIMESTAMP
    };
    auto types = grid.getColTypes();
    REQUIRE(types == expectedTypes);
    auto& data = grid.getData();
    REQUIRE(data.size() == 1);
    REQUIRE(data[0][0]->toString() == "1");
    REQUIRE(data[0][1]->toString() == "2");
    REQUIRE(std::stod(data[0][2]->toString()) == Catch::Approx(3.0));
    REQUIRE(data[0][3]->toString() == "Hello");
    REQUIRE(data[0][4]->toString() == "1970-01-01 00:00:01.234567890Z");
    }
    {
    bentoclient::DataGrid grid;

    grid.addRow(1, 2U, 3.0, std::string("Hello"), bentoclient::Timestamp(std::chrono::nanoseconds(1234567890)));
    grid.insertRow(0, 2);
    grid.insertNullColumn(3, bentoclient::DataGrid::DataType::GENERIC, "NewGenericCol");
    grid.insertColumn(2, std::string("IV"), "NewStringCol");
    std::list<bentoclient::DataGrid::DataType> expectedTypes = {
        bentoclient::DataGrid::DataType::INT,
        bentoclient::DataGrid::DataType::UINT,
        bentoclient::DataGrid::DataType::STRING,
        bentoclient::DataGrid::DataType::DOUBLE,
        bentoclient::DataGrid::DataType::GENERIC,
        bentoclient::DataGrid::DataType::STRING,
        bentoclient::DataGrid::DataType::TIMESTAMP
    };
    auto types = grid.getColTypes();
    REQUIRE(types == expectedTypes);
    auto gridData = grid.getStringGrid();
    REQUIRE(gridData.size() == 2);
    std::string csvString = getGridCsv(grid);
    std::string expectedCsvString = "2,{null},IV,{null},{null},{null},{null}\n"
        "1,2,IV,3.000000,{null},Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    REQUIRE(csvString == expectedCsvString);
    grid.setColName(0, "IntCol");
    grid.setColName(1, "UIntCol");
    grid.setColName(3, "DoubleCol");
    grid.setColName(4, "GenericCol");
    grid.setColName(5, "StringCol");
    grid.setColName(6, "TimestampCol");
    auto headerNames = grid.getColNames();
    std::list<std::string> expectedHeaderNames = {
        "IntCol", "UIntCol", "NewStringCol", "DoubleCol", "GenericCol", "StringCol", "TimestampCol"
    };
    REQUIRE(headerNames == expectedHeaderNames);
    std::string csvString2 = getHeadCsv(grid);
    std::string expectedCsvString2 = "IntCol,UIntCol,NewStringCol,DoubleCol,GenericCol,StringCol,TimestampCol\n";
    REQUIRE(csvString2 == expectedCsvString2);
    grid.setNullValue("NULL");
    std::string newNullCsvString = getGridCsv(grid);
    std::string newNullExpectedCsvString = "2,NULL,IV,NULL,NULL,NULL,NULL\n"
        "1,2,IV,3.000000,NULL,Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    REQUIRE(newNullCsvString == newNullExpectedCsvString);
    std::ostringstream oss;
    grid.serializeStringGrid(oss, ",", "\n");
    std::string serializedString = oss.str();
    REQUIRE(serializedString == newNullExpectedCsvString);
    grid.setFormat(3, std::make_unique<bentoclient::DataGrid::DoubleFormat>(".3f"));
    std::string doubleFormattedString = getGridCsv(grid);
    std::string expectedDoubleFormattedString = "2,NULL,IV,{null},NULL,NULL,NULL\n"
        "1,2,IV,3.000,NULL,Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    REQUIRE(doubleFormattedString == expectedDoubleFormattedString);
    grid.setFormat(3, std::make_unique<bentoclient::DataGrid::DoubleFormat>(".0f", "Nope"));
    std::string doubleFormattedString2 = getGridCsv(grid);
    std::string expectedDoubleFormattedString2 = "2,NULL,IV,Nope,NULL,NULL,NULL\n"
        "1,2,IV,3,NULL,Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    grid.setFormat(3, std::make_unique<bentoclient::DataGrid::DoubleFormat>("+8.1f", ""));
    std::string doubleFormattedString3 = getGridCsv(grid);
    std::string expectedDoubleFormattedString3 = "2,NULL,IV,,NULL,NULL,NULL\n"
        "1,2,IV,    +3.0,NULL,Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    REQUIRE(doubleFormattedString3 == expectedDoubleFormattedString3);
    grid.setFormat(3, std::make_unique<bentoclient::DataGrid::DoubleFormat>("08.1f", ""));
    std::string doubleFormattedString4 = getGridCsv(grid);
    std::string expectedDoubleFormattedString4 = "2,NULL,IV,,NULL,NULL,NULL\n"
        "1,2,IV,000003.0,NULL,Hello,1970-01-01 00:00:01.234567890Z\n"
        ;
    REQUIRE(doubleFormattedString4 == expectedDoubleFormattedString4);
    grid.setFormat(6, std::make_unique<bentoclient::DataGrid::TimestampFormat>(
        "%Y-%m-%d %H:%M:%S"));
    std::string timestampFormattedString = getGridCsv(grid);
    std::string expectedTimestampFormattedString = "2,NULL,IV,,NULL,NULL,{null}\n"
        "1,2,IV,000003.0,NULL,Hello,1970-01-01 00:00:01.234567890\n"
        ;
    REQUIRE(timestampFormattedString == expectedTimestampFormattedString);
    grid.setFormat(6, std::make_unique<bentoclient::DataGrid::TimestampFormatSeconds>());
    std::string timestampFormattedString2 = getGridCsv(grid);
    std::string expectedTimestampFormattedString2 = "2,NULL,IV,,NULL,NULL,{null}\n"
        "1,2,IV,000003.0,NULL,Hello,1970-01-01 00:00:01\n"
        ;
    REQUIRE(timestampFormattedString2 == expectedTimestampFormattedString2);
    }
    {
    bentoclient::DataGrid grid;
    grid.createHeaders({
        {"Date", bentoclient::DataGrid::DataType::TIMESTAMP},
    });
    grid.addRow(bentoclient::DateUtils::makeTimestampZulu(2025, 4, 2, 17, 30, 0));
    std::string serializedString = getGridCsv(grid);
    std::string expectedSerializedString = "2025-04-02 17:30:00.000000000Z\n";
    REQUIRE(serializedString == expectedSerializedString);
    grid.setFormat(0, std::make_unique<bentoclient::DataGrid::TimestampFormatSeconds>());
    std::string serializedString2 = getGridCsv(grid);
    std::string expectedSerializedString2 = "2025-04-02 17:30:00\n";
    REQUIRE(serializedString2 == expectedSerializedString2);
    grid.setFormat(0, std::make_unique<bentoclient::DataGrid::TimestampFormatSeconds>(
        bentoclient::DataGrid::m_defaultTimestampFormat, 
        bentoclient::DateUtils::Timezone::m_NYC));
    std::string serializedString3 = getGridCsv(grid);
    std::string expectedSerializedString3 = "2025-04-02 13:30:00\n";
    // check if timestamps can be outputted as time and date only
    grid.setFormat(0, std::make_unique<bentoclient::DataGrid::TimestampFormatSeconds>(
        bentoclient::DataGrid::m_defaultDateFormat));
    std::string serializedString4 = getGridCsv(grid);
    std::string expectedSerializedString4 = "2025-04-02\n";
    REQUIRE(serializedString4 == expectedSerializedString4);
    grid.setFormat(0, std::make_unique<bentoclient::DataGrid::TimestampFormatSeconds>(
        bentoclient::DataGrid::m_defaultTimeFormat));
    std::string serializedString5 = getGridCsv(grid);
    std::string expectedSerializedString5 = "17:30:00\n";
    REQUIRE(serializedString5 == expectedSerializedString5);
    try {
        grid.setFormat(0, std::make_unique<bentoclient::DataGrid::DoubleFormat>());
        REQUIRE(false);
    } catch (const std::invalid_argument& e) {
        REQUIRE(std::string(e.what()) == "DataGrid: setFormat: type mismatch."
            " Trying to set format for DOUBLE, but Header[0] is of type TIMESTAMP");
    }
    try {
        grid.addRow(1);
        REQUIRE(false);
    }   catch (const std::invalid_argument& e) {
        REQUIRE(std::string(e.what()) == "Header type mismatch. Row[0] = INT, Header[0] = TIMESTAMP");
    }
    try {
        grid.setValue(0, 0, 1);
        REQUIRE(false);
    }   catch (const std::invalid_argument& e) {
        REQUIRE(std::string(e.what()) == "DataGrid: setValue: type mismatch. Trying to set value for INT,"
            " but Header[0] is of type TIMESTAMP");
    }
    grid.setNull(0,0);
    std::string oss6val = getGridCsv(grid);
    REQUIRE(oss6val == "{null}\n");
    }   
}


