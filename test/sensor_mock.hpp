#pragma once

#include <gmock/gmock.h>

#include "interfaces.hpp"
#include "sensors/sensor.hpp"

class SensorMock : public Sensor
{
    public:
        virtual ~SensorMock() = default;

        SensorMock(const std::string& name, int64_t timeout)
            : Sensor(name, timeout) {}

        MOCK_METHOD0(read, ReadReturn());
        MOCK_METHOD1(write, void(double));
};
