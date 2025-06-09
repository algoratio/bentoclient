#include "bentoclient/apputils.hpp"
#include <memory>
#include <array>   // For std::array
#include <stdexcept> // For std::runtime_error
#include <cstdio>  // For popen and pclose
#include <filesystem>
#include <algorithm>
#include <fmt/core.h>
#include <sys/wait.h>

using namespace bentoclient;

const std::string AppUtils::m_defaultSep(",");

// Function to execute a shell command and capture its stdout
std::string AppUtils::executeShellCommand(const std::string& command) {
    if (!std::filesystem::exists(command))
    {
        throw std::invalid_argument(fmt::format("Executable {} not found", command));
    }
    auto perms = std::filesystem::status(command).permissions();
    if ((perms & std::filesystem::perms::owner_exec) == std::filesystem::perms::none &&
        (perms & std::filesystem::perms::group_exec) == std::filesystem::perms::none)
    {
        throw std::runtime_error(fmt::format("Executable file {} lacking executable flags", command));
    }
    std::array<char, 128> buffer;
    std::string result;
    // Exception safe keeping of pipe raw pointer
    struct Piper
    {
        Piper(const std::string& command)
        {
            m_file = popen(command.c_str(), "r");
        }

        bool valid() const
        {
            return m_file != nullptr;
        }

        int close()
        {
            int exitStatus = pclose(m_file);
            m_file = 0x0;
            int exitCode = -1;
            if (WIFEXITED(exitStatus)) {
                exitCode = WEXITSTATUS(exitStatus);
            }
            return exitCode;
        }

        ~Piper()
        {
            if (m_file != nullptr)
                pclose(m_file);
        }
        FILE* m_file;
    } piper(command);

    if (!piper.valid()) {
        throw std::runtime_error("Failed to open pipe for command execution");
    }

    // read stdout of command script
    while (fgets(buffer.data(), buffer.size(), piper.m_file) != nullptr) {
        result += buffer.data();
    }

    int exitCode = piper.close();
    if (exitCode != 0) {
        throw std::runtime_error(fmt::format("Command script exited with {}\nScript output:\n{}", 
            exitCode, result));
    }
    return result;
}

// Utility function to trim trailing whitespace from a string
std::string AppUtils::trim(const std::string& str) {
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::vector<std::string> AppUtils::splitByLinefeed(const std::string& str)
{
    std::vector<std::string> ret;
    _splitStr(ret, str, "\n");
    return ret;
}

std::list<std::string> AppUtils::splitStr(const std::string& str, const std::string& delim)
{
    std::list<std::string> ret;
    _splitStr(ret, str, delim);
    return ret;
}

std::string AppUtils::getKeyScriptInBinDir(char* argv[]) {
    // Extract the directory from the binary path
    std::string sBinPath(argv[0]);
    std::string sBinDir = std::filesystem::path(sBinPath).parent_path().string();
    // Set sGetKey to a default script path in the binary directory
    return sBinDir + "/getkey.sh";
}

std::string AppUtils::getKeyScript(int argc, char* argv[]) {
    std::string sGetKey;
    if (argc > 1) {
        sGetKey = argv[1];
    } else {
        sGetKey = getKeyScriptInBinDir(argv);
    }
    return sGetKey;
}

std::string AppUtils::getExecutableName(char* argv[])
{
    std::string sBinPath(argv[0]);
    std::string sFileName = std::filesystem::path(sBinPath).filename().string();
    return sFileName;
}

std::string AppUtils::toLower(const std::string& str) {
        std::string data(str);
        std::transform(data.begin(), data.end(), data.begin(),
            [](unsigned char c){ return std::tolower(c); });
        return data;
};

std::string AppUtils::toUpper(const std::string& str) {
        std::string data(str);
        std::transform(data.begin(), data.end(), data.begin(),
            [](unsigned char c){ return std::toupper(c); });
        return data;
};

