#include "persistercsvinterceptor.hpp"

// Outputter implementation
bentoclient::PersisterCSV::Outputter bentotests::createStringCaptureOutputter(
    std::list<std::string>& capturedPath, std::list<std::string>& capturedOutput)
{
    return [&capturedPath, &capturedOutput](const std::string& pathName) -> std::unique_ptr<std::ostream> {
        capturedPath.push_back(pathName);
        return std::make_unique<StringCaptureStream>(
            [&capturedOutput](const std::string& output) {
                capturedOutput.push_back(output); // Store the captured output in the provided string
            });
    };
}
