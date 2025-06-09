#include "bentoclient/persistercsv.hpp"
#include "bentoclient/optionchain.hpp"
#include "bentoclient/marketenvironment.hpp"
#include "bentoclient/csvfromoptionchain.hpp"
#include "bentoclient/apputils.hpp"
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fstream>
#include <filesystem>

using namespace bentoclient;

PersisterCSV::PersisterCSV(const std::string& basePath, 
    bool splitFoldersByDate, CSVFormat csvFormat) :
    m_basePath(basePath),
    m_splitFoldersByDate(splitFoldersByDate),
    m_csvFormat(csvFormat),
    m_outputter(makeFileOutputter()),
    m_missingOutputter(makeFileOutputter())
{}

void PersisterCSV::persist(OptionChain&& optionChain, 
    std::shared_ptr<MarketEnvironment> marketEnvironment)
{
    Timestamp chainTime = optionChain.getChainTime();
    Timestamp expiryTime = optionChain.getExpiryTime(marketEnvironment->getExchangeClose());
    double fRiskFreeRate = marketEnvironment->getRiskFreeRate(chainTime, expiryTime);
    CSVFromOptionChain toCsv(marketEnvironment);
    std::string outputPath = filenamePart(
        optionChain.getValuationDate(), optionChain.getUnderlier());
    std::string fileNameEnd = fmt::format("_chain_{}_{}_n{}.csv", 
        optionChain.getValuationDate(),
        optionChain.getExpiryDate(),
        optionChain.getPuts().size());
    outputPath += fileNameEnd;
    Outputter::result_type ostreamPtr = m_outputter(outputPath);
    switch(m_csvFormat)
    {
    case CSVFormat::SideBySide:
        toCsv.sideBySide(*ostreamPtr, optionChain);
        break;
    case CSVFormat::Stacked:
        toCsv.stacked(*ostreamPtr, optionChain);
        break;
    default:
        throw std::invalid_argument(fmt::format("Unsupported CSV format {}", 
            static_cast<int>(m_csvFormat)));
    }
}

void PersisterCSV::persistMissing(const std::string& symbol, const std::string& sDate,
    std::list<std::pair<Timestamp, std::string>>&& missingList)
{
    if (missingList.empty())
        return;
    std::string basePath = filenamePart(sDate, symbol);
    std::string fileNameEnd = fmt::format("_missing_{0:}_{1:%H-%M-%S}.txt", sDate, 
        missingList.front().first);
    std::string pathName = basePath + fileNameEnd;
    Outputter::result_type ostreamPtr = m_missingOutputter(pathName);
    for (auto& pair: missingList)
    {
        *ostreamPtr << fmt::format("{0:%Y-%m-%d %H:%M:%S}", pair.first) 
            << " EXP " << pair.second << "\n";
    }
}

void PersisterCSV::setOutputter(Outputter&& outputter)
{
    m_outputter = std::move(outputter);
}

void PersisterCSV::setMissingOutputter(Outputter&& outputter)
{
    m_missingOutputter = std::move(outputter);
}

std::string PersisterCSV::filenamePart(
    const std::string& sDate, const std::string& symbol) const
{
    std::string outputPath(m_basePath);
    if (m_splitFoldersByDate)
    {
        outputPath += "/" + sDate;
    }
    outputPath += "/" + AppUtils::toLower(symbol);
    return outputPath;
}

PersisterCSV::Outputter PersisterCSV::makeFileOutputter()
{
    Outputter outputter([](const std::string& pathname) -> std::unique_ptr<std::ostream>
    {
        // standard file system outputter creating directories if nonexist
        std::filesystem::path path(pathname);
        if (path.has_parent_path())
        {
            std::filesystem::create_directories(path.parent_path());
        }
        return std::make_unique<std::ofstream>(pathname);
    });
    return outputter;
}

