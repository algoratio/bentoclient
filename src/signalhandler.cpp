#include "bentoclient/signalhandler.hpp"
using namespace bentoclient;
std::sig_atomic_t volatile SignalHandler::m_gSignal(0);

SignalHandler::SignalHandler() {
    // Constructor implementation
    // Set up signal handler for Ctrl+C
    std::signal(SIGINT, [](int signal) { m_gSignal = signal; });
}

std::sig_atomic_t SignalHandler::getSignal() {
    return m_gSignal;
}