#pragma once

#include <gmock/gmock.h>
#include <memory>
#include <string>

#include "pid/zone.hpp"

class ZoneMock : public ZoneInterface
{
    public:
        virtual ~ZoneMock() = default;

        MOCK_METHOD1(getCachedValue, double(const std::string&));
        MOCK_METHOD1(addRPMSetPoint, void(float));
        MOCK_CONST_METHOD0(getMaxRPMRequest, float());
        MOCK_CONST_METHOD0(getFailSafeMode, bool());
        MOCK_CONST_METHOD0(getFailSafePercent, float());
        MOCK_METHOD1(getSensor, const std::unique_ptr<Sensor>&(std::string));
};
