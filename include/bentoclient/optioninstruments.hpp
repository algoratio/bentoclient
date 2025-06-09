#pragma once
#include <databento/symbology.hpp>
#include <memory>
#include <list>
#include <map>

namespace bentoclient {
    /// @brief Container of symbology data for an underlier
    class OptionInstruments
    {
    public:
        /// \brief Struct to hold potentially unmapped parts of a resolution.
        /// \details This struct contains lists of OSI identifiers, invalid OSI identifiers,
        /// and mappings. Can be used to log potential data issues.
        struct Unmapped {
            std::list<std::string> m_osiIdentifiers;
            std::list<std::string> m_invalidOsiIdentifiers;
            std::list<std::pair<std::string, databento::MappingInterval>> m_mappings;
        };
        /// \brief Maps and pairs of strings holding the analyzed options data. Typedefs
        /// are here to help programming and explain the data structure.
        /// @brief Maps an OSI ID to its instrument ID string for a valuation date.
        typedef std::pair<std::string, std::string> OsiToInstrumentId;
        /// @brief Maps all strikes for an underlier and an expiry date to the OSI identifier.
        typedef std::map<std::string, OsiToInstrumentId> StrikeKeyToOsiInstrumentMap;
        /// @brief Combines data for puts and calls for a given underlier and expiry date.
        typedef std::pair<StrikeKeyToOsiInstrumentMap, StrikeKeyToOsiInstrumentMap> StrikeKeyPutCallMap;
        /// @brief The StrikeKeyPutCallMapPtr shared the main data item: the option chain.
        typedef std::shared_ptr<StrikeKeyPutCallMap> StrikeKeyPutCallMapPtr;
        /// @brief Maps all expiry dates for an underlier to available put and call options.
        typedef std::map<std::string, StrikeKeyPutCallMapPtr> ExpiryToPutCallMap;
        /// @brief Lists puts and calls per underlier and valuation date.
        typedef std::map<std::string, ExpiryToPutCallMap> ValuationDateToExpiryPutCallMap;
        /// @brief Maps available underliers to known put and call options for value and expiry date.
        typedef std::map<std::string, ValuationDateToExpiryPutCallMap> UnderlierToPutCallMap;
    private:
        // Subset constructor
        OptionInstruments(UnderlierToPutCallMap&& underlierToPutCallMap);
    public:
        OptionInstruments();
        // Deleted copy constructor and assignment operator
        OptionInstruments(const OptionInstruments&) = delete;
        OptionInstruments& operator=(const OptionInstruments&) = delete;
        // Deleted move constructor and assignment operator
        OptionInstruments(OptionInstruments&&) = default;
        OptionInstruments& operator=(OptionInstruments&&) = default;
        ~OptionInstruments() = default;

        // Other member functions and variables can be added here
        /// @brief Returns a shared pointer to the StrikeKeyPutCallMap, an option chain,
        StrikeKeyPutCallMapPtr getStrikeKeyPutCallMap(const std::string& underlier,
            const std::string& date, const std::string& expiryDate) const;

        const StrikeKeyPutCallMap& getStrikeKeyPutCallMap() const;

        bool contains(const std::string& underlier, const std::string& date,
            const std::string& expiryDate) const {
                return getStrikeKeyPutCallMap(underlier, date, expiryDate) != nullptr;
        }

        /// @brief Returns an OptionInstruments object containing a single option chain
        /// @details Elsewhere referred to as single chain instance
        /// @param underlier Stock symbol
        /// @param date Valuation date
        /// @param expiryDate Expiry date
        /// @return Option instruments instance just for that underlier, date and expiry
        OptionInstruments get(const std::string& underlier, const std::string& date,
            const std::string& expiryDate) const;

        /// @brief Get expiry dates for start date until given number of days to expiry
        /// @param underlier Stock symbol
        /// @param date Valuation date
        /// @param nDte Integer number of days to expiry to cover.
        /// @return List of yyyy-mm-dd dates.
        std::list<std::string> getExpiryDatesForDTE(const std::string& underlier,
            const std::string& date, std::int64_t nDte) const;

        /// @brief Get next expiry date for symbol that's beyond 0Dte
        /// @param underlier Stock symbol
        /// @param date Valuation date
        /// @return The 0Dte expiry, if exists, and the next expiry date.
        std::list<std::string> getNextExpiryDate(const std::string& underlier,
            const std::string& date) const;

        /// @brief maps OSI IDs to instrument IDs from a StrikeKeyPutCallMap
        static std::map<std::string, std::string> makeOsiToInstrumentIdMap(const StrikeKeyPutCallMap& strikeKeyPutCallMap);
        /// @brief maps instrument IDs to OSI IDs from a StrikeKeyPutCallMap
        static std::map<std::string, std::string> makeInstrumentIdToOsiMap(const StrikeKeyPutCallMap& strikeKeyPutCallMap);
        /// @brief maps strike keys to instrument IDs (just for puts or calls, as strike keys refer to both)
        static std::map<std::string, std::string> makeStrikeKeyToInstrumentIdMap(const StrikeKeyToOsiInstrumentMap& strikeToOsiMap);                    
    
        /// @brief Gets a map from OSI IDs to databento instrument IDs 
        /// @param underlier Stock symbol
        /// @param date Valuation date
        /// @param expiryDate Expiry date
        /// @return Map of OSI to ID
        std::map<std::string, std::string> getOsiToInstrumentIdMap(const std::string& underlier,
            const std::string& date, const std::string& expiryDate) const;
        /// @brief gets a map from OSI IDs to databento instrument IDs for use in single chain instances
        std::map<std::string, std::string> getOsiToInstrumentIdMap() const;

        /// @brief Gets a map from databento instrument IDs to OSI IDs
        /// @param underlier Stock symbol
        /// @param date Valuation date
        /// @param expiryDate Expiry date
        /// @return Map of ID to OSI
        std::map<std::string, std::string> getInstrumentIdToOsiMap(const std::string& underlier,
            const std::string& date, const std::string& expiryDate) const;
        /// @brief Gets a map from databento instrument IDs to OSI IDs for use in single chain instance
        std::map<std::string, std::string> getInstrumentIdToOsiMap() const;

        /// @brief Gets the underlier for a single chain instance
        const std::string& getUnderlier() const; 
        /// @brief Gets the valuation date for a single chain instance
        const std::string& getValuationDate() const; 
        /// @brief Gets the expiry date for a single chain instance
        const std::string& getExpiryDate() const;

        /// @brief Builds a multiple instancce option chain from a databento symbology resolution
        void insert(const databento::SymbologyResolution& resolution);

        /// @brief Get potentially unmapped parts of a resultion
        const Unmapped* getUnmapped() const {
            return m_unmapped.get();
        }
        /// @brief Get the contained mappings
        const UnderlierToPutCallMap& getMappings() const {
            return m_underlierToPutCallMap;
        }
        // helpers for lists of contained items
        std::list<std::string> getUnderliers() const;
        std::list<std::string> getValuationDates(const std::string& underlier) const;
        std::list<std::string> getExpiryDates(const std::string& underlier,
            const std::string& valuationDate) const;
        std::list<std::string> getStrikeKeys(const std::string& underlier, 
            const std::string& valuationDate, const std::string& expiryDate, bool put) const;
        std::list<std::string> getStrikes(const std::string& underlier, 
            const std::string& valuationDate, const std::string& expiryDate, bool put) const;
    private:
        std::unique_ptr<Unmapped> m_unmapped;
        UnderlierToPutCallMap m_underlierToPutCallMap;
    };
}