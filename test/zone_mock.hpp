#pragma once

#include <gmock/gmock.h>

#include "pid/zone.hpp"

class ZoneMock : public ZoneInterface
{
    public:
        virtual ~ZoneMock() = default;

        MOCK_METHOD1(getCachedValue, double(const std::string&));
        MOCK_METHOD1(addRPMSetPoint, void(float));
};
