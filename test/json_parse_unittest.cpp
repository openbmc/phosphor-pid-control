#include "build/buildjson.hpp"
#include "errors/exception.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ConfigurationVerificationTest, VerifyHappy)
{
    auto j2 = R"(
      {
        "sensors": [{
          "name": "fan1",
          "type": "fan",
          "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1"
        }],
        "zones": [{
          "id": 1,
          "minThermalRpm": 3000.0,
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

    validateJson(j2);
}

TEST(ConfigurationVerificationTest, VerifyNoSensorKey)
{
    auto j2 = R"(
      {
        "zones": [{
          "id": 1,
          "minThermalRpm": 3000.0,
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

    EXPECT_THROW(validateJson(j2), ConfigurationException);
}

TEST(ConfigurationVerificationTest, VerifyNoZoneKey)
{
}

TEST(ConfigurationVerificationTest, VerifyNoSensor)
{
}

TEST(ConfigurationVerificationTest, VerifyNoPidInZone)
{
}
