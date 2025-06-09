#include "bentoclient/getter.hpp"
#include <fmt/core.h>
#include <fmt/chrono.h>

using namespace bentoclient;

std::string Getter::format(Timestamp timestamp)
{
    return fmt::format("{:%Y-%m-%dT%H:%M:%S}", timestamp);
}