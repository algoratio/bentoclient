#include <catch2/catch_test_macros.hpp>
#include "bentoclient/getterasynchronous.hpp"

TEST_CASE( "Instruments vector split", "[instvsplit]" ) {
    std::vector<int> iv;
    for (int i = 0; i < 333; ++i)
        iv.push_back(i);
    {
    std::uint64_t splitLen = 100;
    std::list<std::vector<int>> splitIv = bentoclient::GetterAsynchronous::splitVector(
        iv, splitLen);

    std::vector<int> recombined;
    for (const auto& subVector: splitIv) {
        recombined.insert(recombined.end(),
            subVector.begin(), subVector.end());
    }
    REQUIRE(recombined == iv);
    }
    {
    std::uint64_t splitLen = 9;
    std::list<std::vector<int>> splitIv = bentoclient::GetterAsynchronous::splitVector(
        iv, splitLen);

    std::vector<int> recombined;
    for (const auto& subVector: splitIv) {
        recombined.insert(recombined.end(),
            subVector.begin(), subVector.end());
    }
    REQUIRE(recombined == iv);
    }
    {
    std::uint64_t splitLen = 332;
    std::list<std::vector<int>> splitIv = bentoclient::GetterAsynchronous::splitVector(
        iv, splitLen);
    REQUIRE(splitIv.size() == 2);
    std::vector<int> recombined;
    for (const auto& subVector: splitIv) {
        recombined.insert(recombined.end(),
            subVector.begin(), subVector.end());
    }
    REQUIRE(recombined == iv);
    }
    {
    std::uint64_t splitLen = 333;
    std::list<std::vector<int>> splitIv = bentoclient::GetterAsynchronous::splitVector(
        iv, splitLen);
    REQUIRE(splitIv.size() == 1);
    std::vector<int> recombined;
    for (const auto& subVector: splitIv) {
        recombined.insert(recombined.end(),
            subVector.begin(), subVector.end());
    }
    REQUIRE(recombined == iv);
    }
    {
    std::uint64_t splitLen = 334;
    std::list<std::vector<int>> splitIv = bentoclient::GetterAsynchronous::splitVector(
        iv, splitLen);
    REQUIRE(splitIv.size() == 1);
    std::vector<int> recombined;
    for (const auto& subVector: splitIv) {
        recombined.insert(recombined.end(),
            subVector.begin(), subVector.end());
    }
    REQUIRE(recombined == iv);
    }

}

TEST_CASE( "Return lists join", "[cbbojoin]" ) {
    std::list<std::list<int>> ll;
    for (int i = 0; i < 10; ++i) {
        std::list<int> subList;
        for (int j = 0; j < 10; ++j) {
            subList.push_back(i * 10 + j);
        }
        ll.push_back(std::move(subList));
    }
    std::list<int> joined = bentoclient::GetterAsynchronous::joinLists(ll);
    std::list<int> expected;
    for (int i = 0; i < 100; ++i) {
        expected.push_back(i);
    }
    REQUIRE(joined == expected);
}

