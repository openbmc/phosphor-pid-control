#pragma once

#include "pid/zone.hpp"

#include <string>

#include <gmock/gmock.h>

class ZoneMock : public ZoneInterface
{
  public:
    virtual ~ZoneMock() = default;

    MOCK_METHOD1(getCachedValue, double(const std::string&));
    MOCK_METHOD1(addRPMSetPoint, void(float));
    MOCK_CONST_METHOD0(getMaxRPMRequest, float());
    MOCK_CONST_METHOD0(getFailSafeMode, bool());
    MOCK_CONST_METHOD0(getFailSafePercent, float());
    MOCK_METHOD1(getSensor, Sensor*(std::string));
};
