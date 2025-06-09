#pragma once
#include "bentoclient/persister.hpp"
#include <string>
#include <functional>
#include <memory>

namespace bentoclient
{
    /// @brief Persister to CSV implementation
    class PersisterCSV : public Persister
    {
    public:
        enum class CSVFormat : int
        {
            Stacked,
            SideBySide
        };
        typedef std::function<std::unique_ptr<std::ostream>(const std::string&)> Outputter;
    public:
        /// @brief Persister to create CSV option chain files
        /// @param basePath Base path to store CSVs in
        /// @param splitFoldersByDate Add date subdirectories if true
        /// @param csvFormat Stacked or put/call side by side
        PersisterCSV(const std::string& basePath, 
            bool splitFoldersByDate, CSVFormat csvFormat = CSVFormat::Stacked);

        /// @brief Persist an option chain
        /// @param optionChain Chain to persist
        /// @param marketEnvironment Market enviroment for put-call-parity compatible rte
        void persist(OptionChain&& optionChain,
            std::shared_ptr<MarketEnvironment> marketEnvironment) override;

        /// @brief Perist missing chain notice
        void persistMissing(const std::string& symbol, const std::string& sDate,
            std::list<std::pair<Timestamp, std::string>>&& missingList) override;

        /// @brief Optional overwrite of stream outputter (for test cases)
        /// @param outputter Function to create output streams
        void setOutputter(Outputter&& outputter);

        /// @brief Optional overwrite of stream outputter for missing chains (for test cases)
        void setMissingOutputter(Outputter&& outputter);

    private:
        std::string filenamePart(const std::string& sDate, const std::string& symbol) const;
        static Outputter makeFileOutputter();
    private:
        std::string m_basePath;
        bool m_splitFoldersByDate;
        CSVFormat m_csvFormat;
        Outputter m_outputter;
        Outputter m_missingOutputter;
    };
}

