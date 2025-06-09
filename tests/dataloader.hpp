#pragma once
#include <databento/symbology.hpp>
#include <databento/record.hpp>
#include <list>
#include <map>

namespace bentoclient
{
    class OptionInstruments;
    class OptionChain;
}
namespace bentotests
{
    class DataLoader
    {
    public:
        DataLoader(const std::string& path = "./tests/data") :
        m_path(path)
        {}
        ~DataLoader() = default;

        databento::SymbologyResolution getSymbologyResolution(const std::string& fileName) const;

        std::list<databento::TradeMsg> getTradeMessages(const std::string& fileName) const;

        std::list<databento::CbboMsg> getCbboMessages(const std::string& fileName) const;

        std::map<std::string, std::list<databento::CbboMsg>> getMappedCbboMessages(const std::string& fileName) const;

        bentoclient::OptionInstruments getOptionInstruments(
            const std::string& symbologyFile
        ) const;
        
        bentoclient::OptionInstruments getOptionInstruments(
            const std::string& symbologyFile,
            const std::string& sSymbol,
            const std::string& sDate,
            const std::string& sExpiryDate) const;

        bentoclient::OptionChain buildOptionChain(
            const std::string& symbologyFile,
            const std::string& sSymbol,
            const std::string& sDate,
            const std::string& sExpiryDate,
            const std::string& sCbboMessages        
        ) const;

        bentoclient::OptionChain buildOptionChainFromCbboMap(
            const std::string& symbologyFile,
            const std::string& sSymbol,
            const std::string& sDate,
            const std::string& sExpiryDate,
            const std::string& sCbboMap        
        ) const;


        std::string pathName(const std::string& fileName) const{
            return m_path + "/" + fileName;
        }
    private:
        std::string m_path;
    };
}