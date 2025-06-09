#include "bentoclient/optioninstruments.hpp"
#include "bentoclient/osioption.hpp"
#include "bentoclient/apputils.hpp"
#include "bentoclient/dateutils.hpp"

using namespace bentoclient;

OptionInstruments::OptionInstruments(UnderlierToPutCallMap&& underlierToPutCallMap) :
    m_underlierToPutCallMap(std::move(underlierToPutCallMap)),
    m_unmapped(nullptr) {
    // Constructor implementation
}

OptionInstruments::OptionInstruments() :
    m_unmapped(std::make_unique<Unmapped>()) {
    // Constructor implementation
}

OptionInstruments::StrikeKeyPutCallMapPtr OptionInstruments::getStrikeKeyPutCallMap(const std::string& underlier,
    const std::string& date, const std::string& expiryDate) const { 
    // Check if the option instruments contain the specified underlier, date, and expiry date
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr;
    auto it = m_underlierToPutCallMap.find(underlier);
    if (it != m_underlierToPutCallMap.end()) {
        auto it2 = it->second.find(date);
        if (it2 != it->second.end()) {
            auto it3 = it2->second.find(expiryDate);
            if (it3 != it2->second.end()) {
                strikeKeyPutCallMapPtr = it3->second;
            }
        }
    }
    return strikeKeyPutCallMapPtr;
}

const OptionInstruments::StrikeKeyPutCallMap& OptionInstruments::getStrikeKeyPutCallMap() const 
{
    // Get the first strike key put call map from the underlier to put call map
    if (m_underlierToPutCallMap.begin() != m_underlierToPutCallMap.end()) {
        auto& dateLevel = m_underlierToPutCallMap.begin()->second;
        if (dateLevel.begin() != dateLevel.end()) {
            auto& expiryLevel = dateLevel.begin()->second;
            if (expiryLevel.begin() != expiryLevel.end()) {
                return *expiryLevel.begin()->second;
            }
        }
    }
    throw std::invalid_argument("No default strike key put call map available.");
}

OptionInstruments OptionInstruments::get(const std::string& underlier, const std::string& date,
    const std::string& expiryDate) const {
    // Get the option instruments for the specified underlier, date, and expiry date only
    UnderlierToPutCallMap underlierToPutCallMap;
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr(getStrikeKeyPutCallMap(underlier, date, expiryDate));
    if (strikeKeyPutCallMapPtr) {
        underlierToPutCallMap[underlier][date][expiryDate] = strikeKeyPutCallMapPtr;
    }
    return OptionInstruments(std::move(underlierToPutCallMap));
}

std::list<std::string> OptionInstruments::getExpiryDatesForDTE(const std::string& underlier,
    const std::string& date, std::int64_t nDte) const
{
    std::list<std::string> expiryDates;
    auto symIt = m_underlierToPutCallMap.find(underlier);
    if (symIt != m_underlierToPutCallMap.end())
    {
        auto dateIt = symIt->second.find(date);
        if (dateIt != symIt->second.end())
        {
            Timestamp ofDate = DateUtils::makeTimestampZulu(date);
            for (auto expDateIt = dateIt->second.begin(); 
                expDateIt != dateIt->second.end(); ++expDateIt)
            {
                Timestamp ofExpDate = DateUtils::makeTimestampZulu(expDateIt->first);
                // if ofExpDate is before ofDate, hours will overflow and no dates be loaded
                if (ofExpDate < ofDate) 
                    continue;
                auto hours = std::chrono::duration_cast<std::chrono::hours>(
                    ofExpDate - ofDate);
                if (hours.count() <= nDte * 24)
                {
                    expiryDates.push_back(expDateIt->first);
                } else {
                    // due to ascending keys, all following dates will be further outside the 
                    // nDte window.
                    break;
                }
            }
        }
    }
    return expiryDates;
}

std::list<std::string> OptionInstruments::getNextExpiryDate(const std::string& underlier,
    const std::string& date) const
{
    std::list<std::string> expiryDates;
    auto symIt = m_underlierToPutCallMap.find(underlier);
    if (symIt != m_underlierToPutCallMap.end())
    {
        auto dateIt = symIt->second.find(date);
        if (dateIt != symIt->second.end())
        {
            auto expDateIt = dateIt->second.begin();
            if (expDateIt != dateIt->second.end())
            {
                Timestamp ofDate = DateUtils::makeTimestampZulu(date);
                for (auto expDateIt = dateIt->second.begin(); 
                    expDateIt != dateIt->second.end(); ++expDateIt)
                {
                    Timestamp ofExpDate = DateUtils::makeTimestampZulu(expDateIt->first);
                    // if ofExpDate is before ofDate, skip
                    if (ofExpDate < ofDate) 
                        continue;
                    expiryDates.push_back(expDateIt->first);
                    // if below dates are same, it's the 0Dte and we look for one more date
                    if (expDateIt->first != date)
                        break;
                }
            }
        }
    }
    return expiryDates;
}

void OptionInstruments::insert(const databento::SymbologyResolution& resolution) {
    // main interest is in the mappings contained in resolution. Sample record
    // from databento ostreaming:
    // { "SPY   240607C00425000", { MappingInterval { start_date = 2024-06-10, 
    //      end_date = 2024-06-11, symbol = "1308623139" } } },
    // While MappingInterval is a vector, in our use case we request data for
    // a single business day and there will practically always only be one
    // element. The start date is the business day for which mapping is valid.
    // Then the key value "SPY   240607C00425000" is the symbol of the underlier
    // and the symbol of a option instrument. On the start date, this option symbol
    // maps to the instrument ID "1308623139".
    // The option symbol '240607C00425000' is a 2024-06-07 expiry date, C for call
    // and 425.00 strike price. 
    // So the OSI format is 6 characters underlier, 2 digits year, two digits month, two digits day, C/P,
    // 5 digits dollar strike, 3 digits decimal strike. Basically, divide the number 
    // past C/P by 1000 to get the strike price. If underlier has less than 6 characters, 
    // it's padded with spaces.
    for (const auto& mapping : resolution.mappings) {
        const std::string& osiIdentifier = mapping.first;
        const std::vector<databento::MappingInterval>& intervals = mapping.second;
        if (intervals.empty()) {
            if (m_unmapped) {
                m_unmapped->m_osiIdentifiers.push_back(osiIdentifier);
            }
            continue; // Skip empty intervals
        }
        try {
            OsiOption osiOption(osiIdentifier);
            auto mi = intervals.begin();
            // insert the good mappings
            auto& valuationDateToExpiryPutCallMap =
                m_underlierToPutCallMap[osiOption.getUnderlier()];
            // get the valuation date in yyyy-mm-dd format
            auto& valuationDate = mi->start_date;
            std::ostringstream valuationDateStream;
            valuationDateStream << valuationDate;
            // get the options data for underlier and valuation date
            auto& expiryToPutCallMap = 
                valuationDateToExpiryPutCallMap[valuationDateStream.str()];
            auto& strikeKeyPutCallMapPtr = 
                expiryToPutCallMap[osiOption.getExpiryDate()];
            if (!strikeKeyPutCallMapPtr) {
                strikeKeyPutCallMapPtr = std::make_shared<StrikeKeyPutCallMap>();
            }
            auto& strikeKeyPutCallMap = *strikeKeyPutCallMapPtr;
            auto& strikeKeyToOsiInstrumentMap = osiOption.isPut() ?
                strikeKeyPutCallMap.first : strikeKeyPutCallMap.second;
            // insert the OSI ID to instrument ID mapping
            auto& instrumentId = mi->symbol;
            strikeKeyToOsiInstrumentMap[osiOption.getStrikeKey()] =
                std::make_pair(osiIdentifier, instrumentId);
            // check for unexpected additional mappings
            for (auto umi = ++mi; umi != intervals.end(); ++umi) {
                if (m_unmapped) {
                    m_unmapped->m_mappings.push_back(std::make_pair(osiIdentifier,*umi));
                }
            }
        } catch (const std::invalid_argument& e) {
            if (m_unmapped) {
                m_unmapped->m_invalidOsiIdentifiers.push_back(osiIdentifier);
            }
            continue; // Skip invalid OSI identifiers
        }

    }

}

std::map<std::string, std::string> OptionInstruments::makeOsiToInstrumentIdMap(const StrikeKeyPutCallMap& strikeKeyPutCallMap)
{
    std::map<std::string, std::string> osiToInstrumentId;
    auto inserter = [&osiToInstrumentId](const StrikeKeyToOsiInstrumentMap& keyMap) {
        for (const auto& pair : keyMap) {
            const auto& osiInstrumentPair = pair.second;
            osiToInstrumentId[osiInstrumentPair.first] = osiInstrumentPair.second;
        }
    };
    inserter(strikeKeyPutCallMap.first);
    inserter(strikeKeyPutCallMap.second);
    return osiToInstrumentId;
}
std::map<std::string, std::string> OptionInstruments::makeInstrumentIdToOsiMap(const StrikeKeyPutCallMap& strikeKeyPutCallMap)
{
    std::map<std::string, std::string> instrumentIdToOsi;
    auto inserter = [&instrumentIdToOsi](const StrikeKeyToOsiInstrumentMap& keyMap) {
        for (const auto& pair : keyMap) {
            const auto& osiInstrumentPair = pair.second;
            instrumentIdToOsi[osiInstrumentPair.second] = osiInstrumentPair.first;
        }
    };
    inserter(strikeKeyPutCallMap.first);
    inserter(strikeKeyPutCallMap.second);
    return instrumentIdToOsi;
}

std::map<std::string, std::string> OptionInstruments::makeStrikeKeyToInstrumentIdMap(const StrikeKeyToOsiInstrumentMap& strikeToOsiMap)
{
    std::map<std::string, std::string> strikeToInstrument;
    for (auto strikeToOsiIt = strikeToOsiMap.begin(); strikeToOsiIt != strikeToOsiMap.end(); ++strikeToOsiIt)
    {
        strikeToInstrument[strikeToOsiIt->first] = strikeToOsiIt->second.second;
    }
    return strikeToInstrument;
}


std::map<std::string, std::string> OptionInstruments::getOsiToInstrumentIdMap(const std::string& underlier,
    const std::string& date, const std::string& expiryDate) const 
{
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr(getStrikeKeyPutCallMap(underlier, date, expiryDate));
    if (strikeKeyPutCallMapPtr) {
        return makeOsiToInstrumentIdMap(*strikeKeyPutCallMapPtr);
    }
    return std::map<std::string, std::string>();
}

std::map<std::string, std::string> OptionInstruments::getOsiToInstrumentIdMap() const
{
    return makeOsiToInstrumentIdMap(getStrikeKeyPutCallMap());
}

std::map<std::string, std::string> OptionInstruments::getInstrumentIdToOsiMap(const std::string& underlier,
    const std::string& date, const std::string& expiryDate) const
{
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr(getStrikeKeyPutCallMap(underlier, date, expiryDate));
    if (strikeKeyPutCallMapPtr) {
        return makeInstrumentIdToOsiMap(*strikeKeyPutCallMapPtr);
    }
    return std::map<std::string, std::string>();
}

std::map<std::string, std::string> OptionInstruments::getInstrumentIdToOsiMap() const
{
    return makeInstrumentIdToOsiMap(getStrikeKeyPutCallMap());
}
const std::string& OptionInstruments::getUnderlier() const
{
    if (m_underlierToPutCallMap.begin() != m_underlierToPutCallMap.end()) {
        return m_underlierToPutCallMap.begin()->first;
    }

    throw std::invalid_argument("No default underlier available.");
} 
const std::string& OptionInstruments::getValuationDate() const
{
    if (m_underlierToPutCallMap.begin() != m_underlierToPutCallMap.end()) {
        auto& dateLevel = m_underlierToPutCallMap.begin()->second;
        if (dateLevel.begin() != dateLevel.end()) {
            return dateLevel.begin()->first;
        }
    }   

    throw std::invalid_argument("No default valuation date available.");
} 
const std::string& OptionInstruments::getExpiryDate() const
{
    if (m_underlierToPutCallMap.begin() != m_underlierToPutCallMap.end()) {
        auto& dateLevel = m_underlierToPutCallMap.begin()->second;
        if (dateLevel.begin() != dateLevel.end()) {
            auto& expiryLevel = dateLevel.begin()->second;
            if (expiryLevel.begin() != expiryLevel.end()) {
                return expiryLevel.begin()->first;
            }
        }
    }
    throw std::invalid_argument("No default expiry date available.");
} 

// helpers for lists of contained items
std::list<std::string> OptionInstruments::getUnderliers() const 
{
    return AppUtils::keyList(m_underlierToPutCallMap);
}

std::list<std::string> OptionInstruments::getValuationDates(const std::string& underlier) const {
    auto it = m_underlierToPutCallMap.find(underlier);
    if (it != m_underlierToPutCallMap.end()) {
        return AppUtils::keyList(it->second);
    }
    return std::list<std::string>();
}
std::list<std::string> OptionInstruments::getExpiryDates(const std::string& underlier,
    const std::string& valuationDate) const {
    auto it = m_underlierToPutCallMap.find(underlier);
    if (it != m_underlierToPutCallMap.end()) {
        auto it2 = it->second.find(valuationDate);
        if (it2 != it->second.end()) {
            return AppUtils::keyList(it2->second);
        }
    }
    return std::list<std::string>();
}
std::list<std::string> OptionInstruments::getStrikeKeys(const std::string& underlier, 
    const std::string& valuationDate, const std::string& expiryDate, bool put) const {
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr(getStrikeKeyPutCallMap(underlier, valuationDate, expiryDate));
    if (strikeKeyPutCallMapPtr) {
        auto& strikeKeyToOsiInstrumentMap = put ? 
            strikeKeyPutCallMapPtr->first : 
            strikeKeyPutCallMapPtr->second;
        return AppUtils::keyList(strikeKeyToOsiInstrumentMap);
    }
    return std::list<std::string>();
}

std::list<std::string> OptionInstruments::getStrikes(const std::string& underlier, 
    const std::string& valuationDate, const std::string& expiryDate, bool put) const {
    std::list<std::string> strikes;
    StrikeKeyPutCallMapPtr strikeKeyPutCallMapPtr(getStrikeKeyPutCallMap(underlier, valuationDate, expiryDate));
    if (strikeKeyPutCallMapPtr) {
        auto& strikeKeyToOsiInstrumentMap = put ? 
            strikeKeyPutCallMapPtr->first : 
            strikeKeyPutCallMapPtr->second;
        for (const auto& pair : strikeKeyToOsiInstrumentMap) {
            strikes.push_back(OsiOption(pair.second.first).getStrike());
        }
    }
    return strikes;
}
