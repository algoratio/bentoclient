#pragma once
#include <csignal>

namespace bentoclient {
/// @brief A CTRL-C signal handler 
class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler() = default;

    /// @brief Gets received signals, or 0
    static std::sig_atomic_t getSignal();
private:
    static std::sig_atomic_t volatile m_gSignal;

};
}