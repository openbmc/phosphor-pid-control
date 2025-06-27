#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pid_control
{

struct VariantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double> operator()(
        const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double> operator()(
        [[maybe_unused]] const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to double");
    }
};

std::string getSensorUnit(const std::string& type);
std::string getSensorPath(const std::string& type, const std::string& id);
std::string getMatch(const std::string& path);
void scaleSensorReading(const double min, const double max, double& value);
bool validType(const std::string& type);

bool findSensors(const std::unordered_map<std::string, std::string>& sensors,
                 const std::string& search,
                 std::vector<std::pair<std::string, std::string>>& matches);

// Set zone number for a zone, 0-based
int64_t setZoneIndex(const std::string& name,
                     std::map<std::string, int64_t>& zones, int64_t index);

// Read zone number for a zone.
int64_t getZoneIndex(const std::string& name,
                     std::map<std::string, int64_t>& zones);

} // namespace pid_control
