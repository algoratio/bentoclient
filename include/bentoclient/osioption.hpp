#pragma once
#include <string>
#include <iostream>
namespace bentoclient {

/// @brief Utility class around an OSI identifier extracting all relevant fields
class OsiOption
{
public:
    /// @brief Constructs an OSI option from an OSI option identifier
    OsiOption(const std::string& osiIdentifier);
    // Default copy constructor and assignment operator
    OsiOption(const OsiOption&) = default;
    OsiOption& operator=(const OsiOption&) = default;
    // Default move constructor and assignment operator
    OsiOption(OsiOption&&) = default;
    OsiOption& operator=(OsiOption&&) = default;
    ~OsiOption() = default;

    /// @brief Gets the underlying OSI identifier
    const std::string& getOsiIdentifier() const {
        return m_osiIdentifier;
    }

    /// @brief Gets the underlier field of the OSI identifier
    const std::string& getUnderlier() const {
        return m_underlier;
    }

    /// @brief Gets the expiry date of the OSI identifier
    const std::string& getExpiryDate() const {
        return m_expiryDate;
    }
    
    /// @brief Gets the option type field of the OSI identifiere
    /// @return P or C
    const std::string& getType() const {
        return m_type;
    }

    /// @brief Gets the strike value dollar portion of the strike key
    const std::string& getStrikeDollars() const {
        return m_strikeDollars;
    }

    /// @brief Gets the strike value decimal portion of the strike key
    const std::string& getStrikeDecimal() const {
        return m_strikeDecimal;
    }

    /// @brief Gets a decimal string representation of the strike value
    const std::string& getStrike() const {
        return m_strike;
    }

    /// @brief Returns true if option is a call
    bool isCall() const {
        return m_type == m_call;
    }

    /// @brief Returns true if option is a put
    bool isPut() const {
        return m_type == m_put;
    }
    
    /// \brief Get a key value for sorting option instruments by strike price.
    std::string getStrikeKey() const {
        return m_strikeDollars + m_strikeDecimal;
    } 
    /// \brief Turn a float strike into a strike key string
    static std::string toStrikeKey(double strike);
    /// \brief Turn a string strike key into a strike value
    static double fromStrikeKey(const std::string& strikeKey);
    /// @brief Turn a string strike key into a double as string
    static std::string fromStrikeKeyAsString(const std::string& strikeKey);
private:
    static std::string getStrike(const std::string& strikeDollars, const std::string& strikeDecimal);
private:
    std::string m_osiIdentifier;
    std::string m_underlier;
    std::string m_expiryDate;
    std::string m_type;
    std::string m_strikeDollars;
    std::string m_strikeDecimal;
    std::string m_strike;
    // statics
public:
    static const std::string m_put;
    static const std::string m_call;
};

}

std::ostream& operator<<(std::ostream& os, const bentoclient::OsiOption& osiOption);
