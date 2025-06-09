#include <bentoclient/osioption.hpp>
#include <regex>
using namespace bentoclient;

std::ostream& operator<<(std::ostream& os, const bentoclient::OsiOption& osiOption)
{
    os << osiOption.getUnderlier()
       << " " << osiOption.getExpiryDate()
       << " " << osiOption.getType()
       << " " << osiOption.getStrike();
    return os;
}
const std::string OsiOption::m_call("C");
const std::string OsiOption::m_put("P");

OsiOption::OsiOption(const std::string& sOsiIdentifier) : 
    m_osiIdentifier(sOsiIdentifier) {
    // The OSI format is 6 characters underlier, 2 digits year, two digits month, two digits day, C/P,
    // 5 digits dollar strike, 3 digits decimal strike. Basically, divide the number 
    // past C/P by 1000 to get the strike price. If underlier has less than 6 characters, 
    // it's padded with spaces.
    // C++ 11 has thread safe statics initialization.
    static const std::regex osiRegex(R"(([A-Z]+)\s*(\d{2})(\d{2})(\d{2})(C|P)(\d{5})(\d{3}))");
    std::smatch match;
    if (std::regex_match(sOsiIdentifier, match, osiRegex)) {
        // The OSI identifier is valid
        m_underlier = match[1].str();
        m_expiryDate = "20" + match[2].str() + "-" + match[3].str() + "-" + match[4].str();
        m_type = match[5].str();
        m_strikeDollars = match[6].str();
        m_strikeDecimal = match[7].str();
        m_strike = getStrike(m_strikeDollars, m_strikeDecimal);
    } else {
        // The OSI identifier is invalid
        throw std::invalid_argument("Invalid OSI identifier format: " + sOsiIdentifier);
    }
}

std::string OsiOption::getStrike(const std::string& strikeDollars, const std::string& strikeDecimal)
{
    auto di = std::find_if(strikeDollars.begin(), strikeDollars.end(), [](char c) { return c != '0'; });
    std::string dollars(di, strikeDollars.end());
    auto rdi = std::find_if(strikeDecimal.rbegin(), strikeDecimal.rend(), [](char c) { return c != '0'; });
    std::string decimal(strikeDecimal.begin(), rdi.base());
    if (decimal.empty()) {
        return dollars;
    } else {
        return dollars + "." + decimal;
    }
}

/// \brief Turn a float strike into a strike key string
std::string OsiOption::toStrikeKey(double strike)
{
    std::string sStrikeDollars = std::to_string(static_cast<int>(strike));
    std::string sStrikeDecimal = std::to_string(strike - static_cast<int>(strike));
    sStrikeDecimal = sStrikeDecimal.substr(2); // remove 0.
    sStrikeDecimal = sStrikeDecimal.substr(0, 3); // keep 3 digits
    // pad on the right with zeros
    while (sStrikeDecimal.size() < 3) {
        sStrikeDecimal += "0";
    }
    if (sStrikeDollars.size() > 5) {
        size_t iStart = sStrikeDollars.size() - 5;
        sStrikeDollars = sStrikeDollars.substr(iStart); 
    }
    // pad on the left with zeros
    while (sStrikeDollars.size() < 5) {
        sStrikeDollars = "0" + sStrikeDollars;
    }
    return sStrikeDollars + sStrikeDecimal;
}

double OsiOption::fromStrikeKey(const std::string& strikeKey)
{
    // The strike is in the format 5 digits dollars, 3 digits decimal
    // The dollar part is an integer, the decimal part is a fraction
    // The strike is a double
    if (strikeKey.size() != 8) {
        throw std::invalid_argument("Invalid strike key format: " + strikeKey);
    }
    std::string sDollars = strikeKey.substr(0, 5);
    std::string sDecimal = strikeKey.substr(5, 3);
    double dDollars = static_cast<double>(std::stoi(sDollars));
    double dDecimal = static_cast<double>(std::stoi(sDecimal)) / 1000.0;
    return dDollars + dDecimal;
}

std::string OsiOption::fromStrikeKeyAsString(const std::string& strikeKey)
{
    // The strike is in the format 5 digits dollars, 3 digits decimal
    // The dollar part is an integer, the decimal part is a fraction
    if (strikeKey.size() != 8) {
        throw std::invalid_argument("Invalid strike key format: " + strikeKey);
    }
    std::string sDollars = strikeKey.substr(0, 5);
    std::string sDecimal = strikeKey.substr(5, 3);
    return getStrike(sDollars, sDecimal);
}

