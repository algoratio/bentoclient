#include "bentoclient/retry.hpp"
#include <databento/exceptions.hpp>

using namespace bentoclient;


bool Retry::isZstdBufferOverflow(const std::exception& e)
{
    auto dbe = dynamic_cast<const databento::DbnResponseError*>(&e);
    if (dbe)
    {
        if (std::string(e.what()).find("Zstd") != std::string::npos)
            return true;
    }
    return false;

}
bool Retry::noRetryError(const std::exception& e)
{
    if (isZstdBufferOverflow(e))
        return true;
    return false;
}