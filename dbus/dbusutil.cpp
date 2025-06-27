
#include <sdbusplus/bus/match.hpp>

#include <cmath>
#include <cstdint>
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

int64_t setZoneIndex(const std::string& name,
                     std::map<std::string, int64_t>& zones, int64_t index)
{
    auto it = zones.find(name);
    if (it != zones.end())
    {
        // Name already allocated, make no change, return existing
        return it->second;
    }

    // The zone name is known not to exist yet
    for (;;)
    {
        bool usedIndex = false;

        // See if desired index number is free
        for (const auto& zi : zones)
        {
            if (index == zi.second)
            {
                usedIndex = true;
                break;
            }
        }

        // Increment until a free index number is found
        if (usedIndex)
        {
            ++index;
            continue;
        }

        break;
    }

    // Allocate and return new zone index number for this name
    zones[name] = index;
    return index;
}

int64_t getZoneIndex(const std::string& name,
                     std::map<std::string, int64_t>& zones)
{
    auto it = zones.find(name);
    if (it != zones.end())
    {
        return it->second;
    }

    // Auto-assign next unused zone number, using 0-based numbering
    return setZoneIndex(name, zones, 0);
}

bool findSensors(const std::unordered_map<std::string, std::string>& sensors,
                 const std::string& search,
                 std::vector<std::pair<std::string, std::string>>& matches)
{
    std::smatch match;
    std::regex reg('/' + search + '$');
    for (const auto& sensor : sensors)
    {
        if (std::regex_search(sensor.first, match, reg))
        {
            matches.emplace_back(sensor);
        }
    }
    return matches.size() > 0;
}

std::string getSensorUnit(const std::string& type)
{
    std::string unit;
    if (type == "fan")
    {
        unit = "xyz.openbmc_project.Sensor.Value.Unit.RPMS";
    }
    else if (type == "temp" || type == "margin")
    {
        unit = "xyz.openbmc_project.Sensor.Value.Unit.DegreesC";
    }
    else if (type == "power" || type == "powersum")
    {
        unit = "xyz.openbmc_project.Sensor.Value.Unit.Watts";
    }
    else
    {
        unit = "unknown";
    }
    return unit;
}

std::string getSensorPath(const std::string& type, const std::string& id)
{
    std::string layer;
    if (type == "fan")
    {
        layer = "fan_tach";
    }
    else if (type == "temp" || type == "margin")
    {
        layer = "temperature";
    }
    else if (type == "power" || type == "powersum")
    {
        layer = "power";
    }
    else
    {
        layer = "unknown"; // TODO(venture): Need to handle.
    }

    return std::string("/xyz/openbmc_project/sensors/" + layer + "/" + id);
}

std::string getMatch(const std::string& path)
{
    return sdbusplus::bus::match::rules::propertiesChangedNamespace(
        path, "xyz.openbmc_project");
}

bool validType(const std::string& type)
{
    static std::set<std::string> valid = {"fan", "temp", "margin", "power",
                                          "powersum"};
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
