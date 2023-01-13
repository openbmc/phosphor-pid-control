#include "pid/buildjson.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

TEST(ZoneFromJson, emptyZone)
{
    // There is a zone key, but it's empty.
    // This is technically invalid.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones": []
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);

    EXPECT_TRUE(pidConfig.empty());
    EXPECT_TRUE(zoneConfig.empty());
}

TEST(ZoneFromJson, oneZoneOnePid)
{
    // Parse a valid configuration with one zone and one PID.
    // Intentionally omits "derivativeCoeff" to test that it is optional.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
    EXPECT_EQ(pidConfig.size(), static_cast<u_int64_t>(1));
    EXPECT_EQ(zoneConfig.size(), static_cast<u_int64_t>(1));

    EXPECT_EQ(pidConfig[1]["fan1-5"].type, "fan");
    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalOutput, 3000.0);
}

TEST(ZoneFromJson, marginZone)
{
    // Parse a valid configuration with one zone and one PID.
    // This is a margin zone, and has both kinds of temperature
    // sensors in it, absolute temperature and margin temperature.
    // Tests that TempToMargin is parsed correctly.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "myPid",
            "type": "margin",
            "inputs": ["absolute0", "absolute1", "margin0", "margin1"],
            "tempToMargin": [
              85.0,
              100.0
            ],
            "setpoint": 10.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
    EXPECT_EQ(pidConfig.size(), static_cast<u_int64_t>(1));
    EXPECT_EQ(zoneConfig.size(), static_cast<u_int64_t>(1));

    EXPECT_EQ(pidConfig[1]["myPid"].type, "margin");
    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalOutput, 3000.0);

    EXPECT_EQ(pidConfig[1]["myPid"].inputs[0].name, "absolute0");
    EXPECT_DOUBLE_EQ(pidConfig[1]["myPid"].inputs[0].convertMarginZero, 85.0);
    EXPECT_EQ(pidConfig[1]["myPid"].inputs[0].convertTempToMargin, true);

    EXPECT_EQ(pidConfig[1]["myPid"].inputs[1].name, "absolute1");
    EXPECT_DOUBLE_EQ(pidConfig[1]["myPid"].inputs[1].convertMarginZero, 100.0);
    EXPECT_EQ(pidConfig[1]["myPid"].inputs[1].convertTempToMargin, true);

    EXPECT_EQ(pidConfig[1]["myPid"].inputs[2].name, "margin0");
    EXPECT_EQ(pidConfig[1]["myPid"].inputs[2].convertTempToMargin, false);

    EXPECT_EQ(pidConfig[1]["myPid"].inputs[3].name, "margin1");
    EXPECT_EQ(pidConfig[1]["myPid"].inputs[3].convertTempToMargin, false);
}

TEST(ZoneFromJson, oneZoneOnePidWithHysteresis)
{
    // Parse a valid configuration with one zone and one PID and the PID uses
    // Hysteresis parameters.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "derivativeCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0,
              "positiveHysteresis": 1000.0,
              "negativeHysteresis": 9000.0
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
    EXPECT_EQ(pidConfig.size(), static_cast<u_int64_t>(1));
    EXPECT_EQ(zoneConfig.size(), static_cast<u_int64_t>(1));

    EXPECT_EQ(pidConfig[1]["fan1-5"].type, "fan");
    EXPECT_DOUBLE_EQ(pidConfig[1]["fan1-5"].pidInfo.positiveHysteresis, 1000.0);

    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalOutput, 3000.0);
}

TEST(ZoneFromJson, oneZoneOneStepwiseWithHysteresis)
{
    // Parse a valid configuration with one zone and one PID and the PID uses
    // Hysteresis parameters.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "temp1",
            "type": "stepwise",
            "inputs": ["temp1"],
            "setpoint": 30.0,
            "pid": {
              "samplePeriod": 0.1,
              "positiveHysteresis": 1.0,
              "negativeHysteresis": 1.0,
              "isCeiling": false,
              "reading": {
                "0": 45,
                "1": 46,
                "2": 47,
                "3": 48,
                "4": 49,
                "5": 50,
                "6": 51,
                "7": 52,
                "8": 53,
                "9": 54,
                "10": 55,
                "11": 56,
                "12": 57,
                "13": 58,
                "14": 59,
                "15": 60,
                "16": 61,
                "17": 62,
                "18": 63,
                "19": 64
              },
              "output": {
                "0": 5000,
                "1": 2400,
                "2": 2600,
                "3": 2800,
                "4": 3000,
                "5": 3200,
                "6": 3400,
                "7": 3600,
                "8": 3800,
                "9": 4000,
                "10": 4200,
                "11": 4400,
                "12": 4600,
                "13": 4800,
                "14": 5000,
                "15": 5200,
                "16": 5400,
                "17": 5600,
                "18": 5800,
                "19": 6000
              }
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
    EXPECT_EQ(pidConfig.size(), static_cast<u_int64_t>(1));
    EXPECT_EQ(zoneConfig.size(), static_cast<u_int64_t>(1));

    EXPECT_EQ(pidConfig[1]["temp1"].type, "stepwise");
    EXPECT_DOUBLE_EQ(pidConfig[1]["temp1"].stepwiseInfo.positiveHysteresis,
                     1.0);

    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalOutput, 3000.0);
}

TEST(ZoneFromJson, getCycleInterval)
{
    // Parse a valid configuration with one zone and one PID and the zone have
    // cycleIntervalTime and updateThermalsTime parameters.

    std::map<int64_t, conf::PIDConf> pidConfig;
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalOutput": 3000.0,
          "failsafePercent": 75.0,
          "cycleIntervalTimeMS": 1000.0,
          "updateThermalsTimeMS": 1000.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "samplePeriod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "derivativeCoeff": 0.0,
              "feedFwdOffsetCoeff": 0.0,
              "feedFwdGainCoeff": 0.010,
              "integralLimit_min": 0.0,
              "integralLimit_max": 0.0,
              "outLim_min": 30.0,
              "outLim_max": 100.0,
              "slewNeg": 0.0,
              "slewPos": 0.0
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
    EXPECT_EQ(pidConfig.size(), static_cast<u_int64_t>(1));
    EXPECT_EQ(zoneConfig.size(), static_cast<u_int64_t>(1));

    EXPECT_EQ(pidConfig[1]["fan1-5"].type, "fan");
    EXPECT_EQ(zoneConfig[1].cycleTime.cycleIntervalTimeMS,
              static_cast<u_int64_t>(1000));
    EXPECT_EQ(zoneConfig[1].cycleTime.updateThermalsTimeMS,
              static_cast<u_int64_t>(1000));
    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalOutput, 3000.0);
}

} // namespace
} // namespace pid_control
