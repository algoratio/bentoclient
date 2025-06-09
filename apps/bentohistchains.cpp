#include "bentoclient/apputils.hpp"
#include "bentoclient/signalhandler.hpp"
#include "bentoclient/requesterasynchronous.hpp"
#include "bentoclient/dateutils.hpp"
#include "bentoclient/requesterasynchronous.hpp"
#include "bentoclient/logging.hpp"
#include <fmt/format.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <algorithm>

namespace po = boost::program_options;
namespace bc = bentoclient;

bc::SignalHandler gSignalHandler;

namespace
{
    class CommandLine
    {
    public:
        CommandLine(int argc, char* argv[]) :
            desc(bc::AppUtils::getExecutableName(argv) + " option chain command line options"),
            vm{},
            optSymbols("symbols"),
            optDate("date"), 
            optTime("time"), optTimeDefault("10:30"),
            optDte("dte"), optDteDefault("14"),
            optKeyScript("keyscript"), optKeyScriptDefault(bc::AppUtils::getKeyScriptInBinDir(argv)),
            optBasePath("basepath"), optBasePathDefault("./optdata"),
            optDefaultRiskFreeRate("riskfreerate"), optDefaultRiskFreeRateDefault("0.042"),
            optRatesCsv("yieldcurve"), optRatesCsvDefault("./data/TSY.2025-06-06.csv"),
            bStacked("csvstacked"), bStackedDefault(false),
            bDateDirs("outdatedirs"), bDateDirsDefault(true),
            optSymbologyThreads("symbologythreads"), optSymbologyThreadsDefault("5"),
            optTimeseriesThreads("timeseriesthreads"), optTimeseriesThreadsDefault("10"),
            optRetries("retries"), optRetriesDefault("3"),
            optLookupTimeRange("lookuptimerange"), optLookupTimeRangeDefault("10"),
            optCbbo1sTimeRange("cbbo1stimerange"), optCbbo1sTimeRangeDefault("10"),
            optCbbo1mTimeRange("cbbo1mtimerange"), optCbbo1mTimeRangeDefault("120"),
            optLogLevel("loglevel"), optLogLevelDefault("error"),
            bLogThreadId("logthreadid"), bLogThreadIdDefault(false)
        {
            addOptions();
        }
    private:
        void addOptions()
        {
        /// command line options
        desc.add_options()

            (
            fmt::format("{},s", optSymbols).c_str(), 
            po::value<std::string>(), 
            "Underlier symbols separated by ','"
            )
            
            (
            fmt::format("{},d", optDate).c_str(), 
            po::value<std::string>(), 
            "Valuation date for options"
            )
            
            (
            fmt::format("{},t", optTime).c_str(),
            po::value<std::string>()->default_value(optTimeDefault),
            fmt::format("Key script path, Default: {}", optTimeDefault).c_str()
            )

            (
            fmt::format("{},n", optDte).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optDteDefault)),
            fmt::format("Max days to expiration, Default: {}", optDteDefault).c_str()
            )
            
            (
            fmt::format("{},k", optKeyScript).c_str(),
            po::value<std::string>()->default_value(optKeyScriptDefault),
            fmt::format("Key script path, Default: {}", optKeyScriptDefault).c_str()
            )

            (
            fmt::format("{}", optBasePath).c_str(),
            po::value<std::string>()->default_value(optBasePathDefault),
            fmt::format("Base CSV output path, Default: {}", optBasePathDefault).c_str()
            )

            (
            fmt::format("{}", optDefaultRiskFreeRate).c_str(),
            po::value<double>()->default_value(std::stod(optDefaultRiskFreeRateDefault)),
            fmt::format("Default continuously compounded risk free rate, Default: {}", optDefaultRiskFreeRateDefault).c_str()
            )

            (
            fmt::format("{}", optRatesCsv).c_str(),
            po::value<std::string>()->default_value(optRatesCsvDefault),
            fmt::format("Yield curve CSV treasury.org format, Default: {}", optRatesCsvDefault).c_str()
            )

            (
            fmt::format("{},f",bStacked).c_str(),
            po::value<bool>()->default_value(bStackedDefault),
            fmt::format("CSV with put/call stacked, Default: {} (side by side)", bStackedDefault).c_str()
            )
            
            (
            fmt::format("{}",bDateDirs).c_str(),
            po::value<bool>()->default_value(bDateDirsDefault),
            fmt::format("CSV into date directories below base path, Default: {}", bDateDirsDefault).c_str()
            )
            
            (
            fmt::format("{}", optSymbologyThreads).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optSymbologyThreadsDefault)),
            fmt::format("Number of symbology request threads enforcing rate limits, Default: {}", optSymbologyThreadsDefault).c_str()
            )
            
            (
            fmt::format("{}", optTimeseriesThreads).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optTimeseriesThreadsDefault)),
            fmt::format("Number of time series request threads enforcing rate limits, Default: {}", optTimeseriesThreadsDefault).c_str()
            )
            
            (
            fmt::format("{},r", optRetries).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optRetriesDefault)),
            fmt::format("Number of retries for data requests, 1 retry == 2 tries, Default: {}", optRetriesDefault).c_str()
            )
            
            (
            fmt::format("{}", optLookupTimeRange).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optLookupTimeRangeDefault)),
            fmt::format("Historical chain lookup time range in minutes, Default: {}", optLookupTimeRangeDefault).c_str()
            )

            (
            fmt::format("{}", optCbbo1sTimeRange).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optCbbo1sTimeRangeDefault)),
            fmt::format("Historical chain cbbo1s sampling duration in seconds, Default: {}", optCbbo1sTimeRangeDefault).c_str()
            )

            (
            fmt::format("{}", optCbbo1mTimeRange).c_str(),
            po::value<std::uint16_t>()->default_value(std::stoi(optCbbo1mTimeRangeDefault)),
            fmt::format("Historical chain cbbo1m sampling duration in minutes, Default: {}", optCbbo1mTimeRangeDefault).c_str()
            )

            (
            fmt::format("{},l", optLogLevel).c_str(),
            po::value<std::string>()->default_value(optLogLevelDefault),
            fmt::format("LogLevel, Default: {}, options {}", optLogLevelDefault, bc::logging::get_log_levels()).c_str()
            )

            (
            fmt::format("{}",bLogThreadId).c_str(),
            po::value<bool>()->default_value(bLogThreadIdDefault),
            fmt::format("Output thread ID in logging, Default: {}", bLogThreadIdDefault).c_str()
            )

            ;
        }
    public:
        std::pair<int, bool> parseCommandLine(int argc, char* argv[])
        {
            try {
                po::store(po::parse_command_line(argc, argv, desc), vm);
                po::notify(vm);
                if (vm.count("help") 
                    || vm.count(optSymbols) == 0
                    || vm.count(optDate) == 0) {
                    std::cout << desc << std::endl;
                    return {0,true};
                }
            } catch (const std::exception& e) {
                fmt::print("Command line error: {}\n", e.what());
                std::cout << desc << std::endl;
                return {1,true};
            }
            return {0,false};
        }

        std::string getSymbols() const
        {
            return bc::AppUtils::toUpper(vm[optSymbols].as<std::string>());
        }
        std::string getDate() const
        {
            return vm[optDate].as<std::string>();
        }
        std::string getTime() const
        {
            return vm[optTime].as<std::string>();
        }
        std::uint16_t getNDte() const
        {
            return vm[optDte].as<std::uint16_t>();
        }
        std::string getBasePath() const
        {
            return vm[optBasePath].as<std::string>();
        }
        double getDefaultRiskFreeRate() const
        {
            return vm[optDefaultRiskFreeRate].as<double>();
        }
        std::string getRatesCsv() const
        {
            return vm[optRatesCsv].as<std::string>();
        }
        std::string getKeyScript() const
        {
            return vm[optKeyScript].as<std::string>();
        }
        bool getStacked() const
        {
            return vm[bStacked].as<bool>();
        }
        bool getDateDirs() const
        {
            return vm[bDateDirs].as<bool>();
        }
        std::uint16_t getSymbologyThreads() const
        {
            return vm[optSymbologyThreads].as<std::uint16_t>();
        }
        std::uint16_t getTimeseriesThreads() const
        {
            return vm[optTimeseriesThreads].as<std::uint16_t>();
        }
        std::uint16_t getRetries() const
        {
            return vm[optRetries].as<std::uint16_t>();
        }
        std::uint16_t getLookupTimeRange() const
        {
            return vm[optLookupTimeRange].as<std::uint16_t>();
        }
        std::uint16_t getCbbo1sTimeRange() const
        {
            return vm[optCbbo1sTimeRange].as<std::uint16_t>();
        }
        std::uint16_t getCbbo1mTimeRange() const
        {
            return vm[optCbbo1mTimeRange].as<std::uint16_t>();
        }
        std::string getLogLevel() const
        {
            return vm[optLogLevel].as<std::string>();
        }
        bool getLogThreadId() const
        {
            return vm[bLogThreadId].as<bool>();
        }

    private:
        po::options_description desc;
        po::variables_map vm;
        std::string optSymbols;
        std::string optDate;
        std::string optTime, optTimeDefault;
        std::string optDte, optDteDefault;
        std::string optKeyScript, optKeyScriptDefault;
        std::string optBasePath, optBasePathDefault;
        std::string optDefaultRiskFreeRate, optDefaultRiskFreeRateDefault;
        std::string optRatesCsv, optRatesCsvDefault;
        std::string bStacked;
        bool bStackedDefault;
        std::string bDateDirs;
        bool bDateDirsDefault;
        std::string optSymbologyThreads, optSymbologyThreadsDefault;
        std::string optTimeseriesThreads, optTimeseriesThreadsDefault;
        std::string optRetries, optRetriesDefault;
        std::string optLookupTimeRange, optLookupTimeRangeDefault;
        std::string optCbbo1sTimeRange, optCbbo1sTimeRangeDefault;
        std::string optCbbo1mTimeRange, optCbbo1mTimeRangeDefault;
        std::string optLogLevel, optLogLevelDefault;
        std::string bLogThreadId;
        bool bLogThreadIdDefault;
    };
}

int main(int argc, char* argv[]) {
    CommandLine cli(argc,argv);
    auto parseResult = cli.parseCommandLine(argc,argv);
    if (parseResult.second) {
        return parseResult.first;
    }

    // get command line arguments
    std::string symbols;
    std::string sDate;
    std::string sTime;
    std::uint16_t nDte = 0;
    std::string sBasePath;
    double fDefaultRiskFreeRate(0.0);
    std::string sRatesCsv;
    std::string sKeyScript;
    bool bStacked = false;
    bool bDateDirs = true;
    // the oomp of this app, its number of request threads hammering databento servers
    std::uint64_t nThreadsSymbology = 0;
    std::uint64_t nThreadsTimeseries = 0;
    // the maximum number of retries for failing databento requests. 3 retries == 4 tries.
    std::uint64_t nRetries = 0;
    std::uint64_t nLookupTimeRange = 0;
    std::uint64_t nCbbo1sTimeRange = 0;
    std::uint64_t nCbbo1mTimeRange = 0;
    std::string sLogLevel;
    bool bLogThreadId = false;
    auto minMax = [](std::uint64_t val, std::uint64_t min, std::uint64_t max) -> std::uint64_t
    {
        val = std::max(val, min);
        val = std::min(val, max);
        return val;
    };
    try {
        symbols = cli.getSymbols();
        sDate = cli.getDate();
        sTime = cli.getTime();
        nDte = cli.getNDte();
        sBasePath = cli.getBasePath();
        fDefaultRiskFreeRate = cli.getDefaultRiskFreeRate();
        sRatesCsv = cli.getRatesCsv();
        sKeyScript = cli.getKeyScript();
        bStacked = cli.getStacked();
        bDateDirs = cli.getDateDirs();
        nThreadsSymbology = minMax(cli.getSymbologyThreads(), 1, 10);
        nThreadsTimeseries = minMax(cli.getTimeseriesThreads(), 1, 100);
        nRetries = minMax(cli.getRetries(), 0, 5);
        nLookupTimeRange = cli.getLookupTimeRange();
        nCbbo1sTimeRange = cli.getCbbo1sTimeRange();
        nCbbo1mTimeRange = cli.getCbbo1mTimeRange();
        sLogLevel = cli.getLogLevel();
        bLogThreadId = cli.getLogThreadId();
    } catch (const std::exception& e) {
        fmt::print("Error converting command options: {}", e.what());
        return 1;
    }

    try {
        bc::logging::init_logging(bLogThreadId, sLogLevel);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    // obtain the API key
    std::string apiKey;
    try {
        // Execute the shell script and capture its output
        apiKey = bc::AppUtils::executeShellCommand(sKeyScript);
        apiKey = bc::AppUtils::trim(apiKey);
    } catch (const std::exception& e) {
        fmt::print("Failed to obtain API Key: {}\nRun {} -help for options\n", 
            e.what(), bc::AppUtils::getExecutableName(argv));
        return 1;
    }

    // give a start message of what is going to be loaded
    std::cout << "Getting option chains for symbols \"" 
        << symbols << "\"" << std::endl 
        << "as " << (bStacked? "stacked" : "side by side")
        << " CSV files for up to " << nDte << " days to expiration" << std::endl
        << "from valuation date "
        << sDate << ", time " << sTime << std::endl
        << "to base output path: " << sBasePath << std::endl; 
    std::list<std::string> symbolList = bc::AppUtils::splitStr(symbols);
    bc::Timestamp at = bc::DateUtils::makeTimestamp(sDate, sTime, 
        bc::DateUtils::Timezone::m_NYC);

    // *** set up the requester interface ***

    // Define number of threads for work on underliers (symbols) such that request pools are busy
    std::uint64_t nThreadsRequester = nThreadsSymbology + nThreadsTimeseries + 5;
    // maximum number of instrument ids in one timeseries request to keep 
    // https request headers in limits. A lower number here results in 
    // an array of https requests fired with subsets of the total number
    // of instrument ids. Bento requests can also fail on receive buffer overflow
    // with higher number of instruments in split, like 300.
    std::uint64_t nSplitInstrumentIds = 100;
    // the time period for option chain valuation times to match requested.
    bc::TimeRange chainLookupTimeRange = std::chrono::minutes(nLookupTimeRange);
    bc::TimeRange cbbo1sTimeRange = std::chrono::seconds(nCbbo1sTimeRange);
    bc::TimeRange cbbo1mTimeRange = std::chrono::minutes(nCbbo1mTimeRange);
    // forwards ctrl-C to threaded tasks for bailout
    std::function<bool()> terminateSignal = [](){
        return bc::SignalHandler::getSignal() != 0;
    };
    // dataset for US option chains
    std::string sDataset("OPRA.PILLAR");
    // the interface for asynchronous job submission
    std::unique_ptr<bc::RequesterAsynchronous> requester = 
        bc::RequesterAsynchronous::makeRequesterCSV(
        apiKey,
        sDataset,
        sBasePath,
        nThreadsRequester,
        nThreadsSymbology,
        nThreadsTimeseries,
        terminateSignal,
        nSplitInstrumentIds,
        nRetries,
        chainLookupTimeRange,
        cbbo1sTimeRange,
        cbbo1mTimeRange,
        fDefaultRiskFreeRate,
        sRatesCsv,
        bStacked,
        bDateDirs);

    std::map<bc::Requester::JobId, std::string> requestMap;
    for (auto& symbol : symbolList)
    {
        requestMap[requester->requestOptionChains(symbol, at, nDte)] = symbol;
        std::cout << "Submitted request for symbol " << symbol << " at valuation time "
            << bc::DateUtils::timestampToStringIntSeconds(at) << " for up to " << nDte
            << " days to expiry" << std::endl;
    }
    // wait for results. The call to query blocks until some results are in.
    bc::ThreadPool::ResultMap resultMap = requester->query();
    while (!resultMap.empty()) {
        for (auto it = resultMap.begin(); it != resultMap.end(); ++it)
        {
            auto symIt = requestMap.find(it->first);
            if (symIt == requestMap.end())
            {
                std::cerr << "Potentially corrupted job handling for JobId " << it->first << std::endl;
            } 
            else 
            {
                if (it->second.m_failed == false) {
                    if (terminateSignal()) 
                    {
                        std::cerr << "Terminate processing for symbol " << symIt->second 
                            << " after signal" << std::endl; 
                    }
                    else
                    {
                        std::cout << "Received all results symbol " << symIt->second << std::endl;
                    }
                }
                else
                {
                    std::cout << "Received error for symbol " << symIt->second << ": " << it->second.m_message << std::endl;
                } 
            }
        }
        // returns empty map if all pending jobs are done. Otherwise blocks until more results are in.
        resultMap = requester->query();
    }
    return 0;
}