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
          "minthermalrpm": 3000.0,
          "failsafepercent": 75.0,
          "pids": [{
            "name": "fan1-5",
            "type": "fan",
            "inputs": ["fan1", "fan5"],
            "set-point": 90.0,
            "pid": {
              "sampleperiod": 0.1,
              "p_coefficient": 0.0,
              "i_coefficient": 0.0,
              "ff_off_coefficient": 0.0,
              "ff_gain_coefficient": 0.010,
              "i_limit_min": 0.0,
              "i_limit_max": 0.0,
              "out_limit_min": 30.0,
              "out_limit_max": 100.0,
              "slew_neg": 0.0,
              "slew_pos": 0.0
            }
          }]
        }]
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
}
