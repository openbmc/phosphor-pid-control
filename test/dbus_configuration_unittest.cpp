#include "conf.hpp"
#include "dbus/dbusconfiguration.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{

std::map<std::string, struct conf::SensorConfig> sensorConfig;
std::map<int64_t, conf::PIDConf> zoneConfig;
std::map<int64_t, struct conf::ZoneConfig> zoneDetailsConfig;

namespace dbus_configuration
{
namespace
{

TEST(DbusFindSensorTest, ValidateEmptyWithNoMatches)
{
    // A map of <path, interface> for sensors
    std::unordered_map<std::string, std::string> sensors = {
        {"path/a", "interface"},
        {"path/b", "interface"},
    };

    std::vector<std::pair<std::string, std::string>> result;

    EXPECT_FALSE(findSensors(sensors, "c", result));
}

TEST(DbusFindSensorTest, ValidateFindsMultipleMatches)
{}

} // namespace
} // namespace dbus_configuration
} // namespace pid_control
