#pragma once

#include "interfaces.hpp"
#include "sensors/sensor.hpp"

#include <gmock/gmock.h>

namespace pid_control
{

class SensorMock : public Sensor
{
  public:
    ~SensorMock() override = default;

    SensorMock(const std::string& name, int64_t timeout) : Sensor(name, timeout)
    {}

    MOCK_METHOD0(read, ReadReturn());
    MOCK_METHOD1(write, void(double));
    MOCK_METHOD3(write, void(double, bool, int64_t*));
};

} // namespace pid_control
