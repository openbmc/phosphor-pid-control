#include "pid/buildjson.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ZoneFromJson, emptyZone)
{
    // There is a zone key, but it's empty.
    // This is technically invalid.

    std::map<int64_t, PIDConf> pidConfig;
    std::map<int64_t, struct ZoneConfig> zoneConfig;

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

    std::map<int64_t, PIDConf> pidConfig;
    std::map<int64_t, struct ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones" : [{
          "id": 1,
          "minThermalRpm": 3000.0,
          "failsafePercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "setpoint": 90.0,
            "pid": {
              "sampleperiod": 0.1,
              "proportionalCoeff": 0.0,
              "integralCoeff": 0.0,
              "feedFwdOffOffsetCoeff": 0.0,
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
    EXPECT_EQ(pidConfig.size(), 1);
    EXPECT_EQ(zoneConfig.size(), 1);

    EXPECT_EQ(pidConfig[1]["fan1-5"].type, "fan");
    EXPECT_DOUBLE_EQ(zoneConfig[1].minThermalRpm, 3000.0);
}
