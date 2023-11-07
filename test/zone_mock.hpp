#pragma once

#include "pid/zone_interface.hpp"

#include <string>

#include <gmock/gmock.h>

namespace pid_control
{

class ZoneMock : public ZoneInterface
{
  public:
    virtual ~ZoneMock() = default;

    MOCK_METHOD0(updateFanTelemetry, void());
    MOCK_METHOD0(updateSensors, void());
    MOCK_METHOD0(initializeCache, void());
    MOCK_METHOD1(getCachedValue, double(const std::string&));

    // Compatibility interface for getCachedValues
    ValueCacheEntry getCachedValues(const std::string& s)
    {
        auto v = getCachedValue(s);
        return {v, v};
    }

    MOCK_CONST_METHOD0(getRedundantWrite, bool(void));
    MOCK_METHOD2(addSetPoint, void(double, const std::string&));
    MOCK_METHOD2(setOutputCache,
                 void(std::string_view name, const ValueCacheEntry& values));
    MOCK_METHOD0(clearSetPoints, void());
    MOCK_METHOD1(addRPMCeiling, void(double));
    MOCK_METHOD0(clearRPMCeilings, void());
    MOCK_METHOD0(determineMaxSetPointRequest, void());
    MOCK_CONST_METHOD0(getMaxSetPointRequest, double());

    MOCK_METHOD0(processFans, void());
    MOCK_METHOD0(processThermals, void());

    MOCK_CONST_METHOD0(getManualMode, bool());
    MOCK_CONST_METHOD0(getFailSafeMode, bool());
    MOCK_CONST_METHOD2(getFailSafePercent,
                       void(std::string& name, double& percent));
    MOCK_CONST_METHOD0(getZoneID, int64_t());

    MOCK_CONST_METHOD0(getCycleIntervalTime, uint64_t());
    MOCK_CONST_METHOD0(getUpdateThermalsCycle, uint64_t());

    MOCK_METHOD1(getSensor, Sensor*(const std::string&));

    MOCK_METHOD0(initializeLog, void());
    MOCK_METHOD1(writeLog, void(const std::string&));

    MOCK_METHOD4(updateThermalPowerDebugInterface,
                 void(std::string pidName, std::string leader, double input,
                      double output));
};

} // namespace pid_control
