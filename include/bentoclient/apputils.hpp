#pragma once
#include <string>
#include <map>
#include <sstream>
#include <list>
#include <vector>

namespace bentoclient {

/// @brief General utility function collection
class AppUtils {
public:
    /// @brief Executes a command string in a shell and captures stdout
    /// @param command Command to execute
    /// @return Caputred stdout
    static std::string executeShellCommand(const std::string& command);
    
    /// @brief Remove trailing spaces from string
    /// @param str Strint to trim
    /// @return Trimmed string
    static std::string trim(const std::string& str);

    /// @brief Gets the default location of the key script in the bin directory
    /// @param argv Argv from command line
    /// @return Default key script path
    static std::string getKeyScriptInBinDir(char* argv[]);

    /// @brief Gets the key script from command line (for some of the executables)
    /// @param argc passed to main
    /// @param argv passed to main
    /// @return Key script path
    static std::string getKeyScript(int argc, char* argv[]);

    /// @brief Get the name of the running executables for command line help outputs
    /// @param argv passed to main
    /// @return The name part of the executable string
    static std::string getExecutableName(char* argv[]);

    /// @brief Make string lower case
    /// @param str string
    /// @return string transformed to lower case
    static std::string toLower(const std::string& str);

    /// @brief Make string upper case
    /// @param str string
    /// @return string transformed to upper case
    static std::string toUpper(const std::string& str);

    /// @brief Split string implementation for lists or vectors
    /// @tparam C Container type (vector or list)
    /// @param container Vector or list
    /// @param str string to split
    /// @param delim split delimiter
    template <typename C>
    static void _splitStr(C& container, const std::string& str, const std::string& delim)
    {
        std::size_t start = 0;
        std::size_t end = 0;
        std::size_t len = delim.length();
        while ((end = str.find(delim, start)) != std::string::npos) {
            std::string token = str.substr(start, end - start);
            container.emplace_back(std::move(token));
            start = end + len;
        }
        if (start < str.length()) {
            container.emplace_back(std::move(str.substr(start)));
        }
    }

    /// @brief Split string function with line feed as default separator (line split)
    /// @param str string to split
    /// @return Split lines
    static std::vector<std::string> splitByLinefeed(const std::string& str);

    /// @brief Split string to list
    /// @param str String to split
    /// @param delim Field delimiter
    /// @return list of fields
    static std::list<std::string> splitStr(const std::string& str, const std::string& delim = m_defaultSep);

    /// @brief Join the keys of a map
    /// @tparam K Key type
    /// @tparam V Value type
    /// @param map Map for which keys will be joined
    /// @param sep Separator string
    /// @return 
    template <typename K, typename V>  static std::string joinKeys(const std::map<K,V>& map, 
        const std::string& sep = m_defaultSep) 
    {
        std::ostringstream joinedKeys;
        for (auto it = map.begin(); it != map.end(); ++it) {
            if (it != map.begin()) {
                joinedKeys << sep; // Add a comma before each key except the first
            }
            joinedKeys << it->first; // Append the key
        }   
        return joinedKeys.str(); // Return the joined string         
    }

    /// @brief Joins a list to a string
    /// @tparam K Value type of list
    /// @param list List to join
    /// @param sep Separator between joined fields
    /// @return Joined string
    template <typename K> static std::string joinList(const std::list<K>& list, 
        const std::string& sep = m_defaultSep) 
    {
        std::ostringstream joinedKeys;
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it != list.begin()) {
                joinedKeys << sep; // Add a comma before each key except the first
            }
            joinedKeys << *it; // Append the key
        }   
        return joinedKeys.str(); // Return the joined string         
    }

    /// @brief Joins a vector to a string
    /// @tparam K Value type of vector
    /// @param vector List to join
    /// @param sep Separator between joined fields
    /// @return Joined string
    template <typename K> static std::string joinVector(const std::vector<K>& vector, 
        const std::string& sep = m_defaultSep) 
    {
        std::ostringstream joinedKeys;
        for (auto it = vector.begin(); it != vector.end(); ++it) {
            if (it != vector.begin()) {
                joinedKeys << sep; // Add a comma before each key except the first
            }
            joinedKeys << *it; // Append the key
        }   
        return joinedKeys.str(); // Return the joined string         
    }

    /// @brief Write the keys of a map to a list
    /// @tparam K Key type
    /// @tparam V Value tupe
    /// @param map Map for which we obtain the key list
    /// @return List of keys
    template <typename K, typename V>  static std::list<K> keyList(const std::map<K,V>& map) 
    {
        std::list<K> keyList;
        for (auto it = map.begin(); it != map.end(); ++it) {
            keyList.push_back(it->first); // Append the key
        }   
        return keyList; // Return the joined string         
    }

    /// @brief Join the keys of a map to a vector
    /// @tparam K Key type
    /// @tparam V Value type
    /// @param map Map for which we obtain a vector of keys
    /// @return Key vector
    template <typename K, typename V>  static std::vector<K> keyVector(const std::map<K,V>& map) 
    {
        std::vector<K> keyVector;
        keyVector.reserve(map.size());
        for (auto it = map.begin(); it != map.end(); ++it) {
            keyVector.push_back(it->first); // Append the key
        }   
        return keyVector; // Return the joined string         
    }

    /// @brief Default separator for split and join string functions
    static const std::string m_defaultSep;
};

}