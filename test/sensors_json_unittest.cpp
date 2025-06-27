#include "sensors/buildjson.hpp"
#include "sensors/sensor.hpp"

#include <sys/types.h>

#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

TEST(SensorsFromJson, emptyJsonNoSensors)
{
    // If the json has no sensors, the map is empty.

    auto j2 = R"(
      {
        "sensors": []
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_TRUE(output.empty());
}

TEST(SensorsFromJson, oneFanSensor)
{
    // If the json has one sensor, it's in the map.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
            "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm1",
            "min": 0,
            "max": 255
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(static_cast<u_int64_t>(1), output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath,
              "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/"
              "hwmon/**/pwm1");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 255);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, false);
}

TEST(SensorsFromJson, IgnoreDbusSensor)
{
    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
            "ignoreDbusMinMax": true
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(static_cast<u_int64_t>(1), output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath, "");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 0);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, true);
}

TEST(SensorsFromJson, TempDbusSensor)
{
    auto j2 = R"(
      {
        "sensors": [{
            "name": "CPU_DTS",
            "type": "temp",
            "readPath": "/xyz/openbmc_project/sensors/temperature/CPU_DTS",
            "unavailableAsFailed": false
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(static_cast<u_int64_t>(1), output.size());
    EXPECT_EQ(output["CPU_DTS"].type, "temp");
    EXPECT_EQ(output["CPU_DTS"].readPath,
              "/xyz/openbmc_project/sensors/temperature/CPU_DTS");
    EXPECT_EQ(output["CPU_DTS"].writePath, "");
    EXPECT_EQ(output["CPU_DTS"].min, 0);
    EXPECT_EQ(output["CPU_DTS"].max, 0);
    EXPECT_EQ(output["CPU_DTS"].timeout,
              Sensor::getDefaultTimeout(output["CPU_DTS"].type));
    EXPECT_EQ(output["CPU_DTS"].unavailableAsFailed, false);
}

TEST(SensorsFromJson, validateOptionalFields)
{
    // The writePath, min, max, timeout, and ignoreDbusMinMax fields are
    // optional.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(static_cast<u_int64_t>(1), output.size());
    EXPECT_EQ(output["fan1"].type, "fan");
    EXPECT_EQ(output["fan1"].readPath,
              "/xyz/openbmc_project/sensors/fan_tach/fan1");
    EXPECT_EQ(output["fan1"].writePath, "");
    EXPECT_EQ(output["fan1"].min, 0);
    EXPECT_EQ(output["fan1"].max, 0);
    EXPECT_EQ(output["fan1"].timeout,
              Sensor::getDefaultTimeout(output["fan1"].type));
    EXPECT_EQ(output["fan1"].ignoreDbusMinMax, false);
    EXPECT_EQ(output["fan1"].unavailableAsFailed, true);
}

TEST(SensorsFromJson, twoSensors)
{
    // Same as one sensor, but two.
    // If a configuration has two sensors with the same name the information
    // last is the information used.

    auto j2 = R"(
      {
        "sensors": [{
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }, {
            "name": "fan2",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }]
      }
    )"_json;

    auto output = buildSensorsFromJson(j2);
    EXPECT_EQ(static_cast<u_int64_t>(2), output.size());
}

} // namespace
} // namespace pid_control
