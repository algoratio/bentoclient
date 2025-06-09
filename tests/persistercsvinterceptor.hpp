#pragma once

#include "bentoclient/persistercsv.hpp"
#include <sstream>
#include <list>
namespace bentotests
{
    /// @brief Captures buffer content at destruction for test cases of
    ///        methods that write content to an ostream flushed at destruction.
    class StringCaptureStream : public std::ostream
    {
    public:
        using Callback = std::function<void(const std::string&)>;

        explicit StringCaptureStream(Callback callback)
            : std::ostream(nullptr), 
            m_buffer{}, 
            m_callback(std::move(callback)) 
        {
            // pass the stream to superclass only after initialization
            // instead of std::ostream(&m_buffer) in initializer list
            this->rdbuf(&m_buffer);
        }

        ~StringCaptureStream()
        {
            // Trigger the callback with the captured string when the stream is destroyed
            if (m_callback) {
                m_callback(m_buffer.str());
            }
        }

    private:
        std::stringbuf m_buffer; // Internal buffer to capture the output
        Callback m_callback;     // Callback to handle the captured string
    };

    // Outputter creation
    bentoclient::PersisterCSV::Outputter createStringCaptureOutputter(
        std::list<std::string>& capturedPath, std::list<std::string>& capturedOutput);
}
