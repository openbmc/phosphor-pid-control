#include "util.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using Property = std::string;
using Value = std::variant<int64_t, double, std::string, bool>;
using PropertyMap = std::map<Property, Value>;

namespace pid_control
{

bool findSensors(const std::unordered_map<std::string, std::string>& sensors,
                 const std::string& search,
                 std::vector<std::pair<std::string, std::string>>& matches)
{
    std::smatch match;
    std::regex reg(search + '$');
    for (const auto& sensor : sensors)
    {
        if (std::regex_search(sensor.first, match, reg))
        {
            matches.push_back(sensor);
        }
    }
    return matches.size() > 0;
}

std::string getSensorPath(const std::string& type, const std::string& id)
{
    std::string layer = type;
    if (type == "fan")
    {
        layer = "fan_tach";
    }
    else if (type == "temp")
    {
        layer = "temperature";
    }
    else if (type == "margin")
    {
        layer = "temperature";
    }
    else
    {
        layer = "unknown"; // TODO(venture): Need to handle.
    }

    return std::string("/xyz/openbmc_project/sensors/" + layer + "/" + id);
}

std::string getMatch(const std::string& type, const std::string& id)
{
    return std::string("type='signal',"
                       "interface='org.freedesktop.DBus.Properties',"
                       "member='PropertiesChanged',"
                       "path='" +
                       getSensorPath(type, id) + "'");
}

bool validType(const std::string& type)
{
    static std::set<std::string> valid = {"fan", "temp", "margin"};
    return (valid.find(type) != valid.end());
}

void scaleSensorReading(const double min, const double max, double& value)
{
    if (max <= 0 || max <= min)
    {
        return;
    }
    value -= min;
    value /= (max - min);
}

} // namespace pid_control
