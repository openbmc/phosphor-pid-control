#pragma once

#include "sensors/sensor.hpp"

#include <string>

namespace pid_control
{

class ZoneInterface
{
  public:
    virtual ~ZoneInterface() = default;

    virtual double getCachedValue(const std::string& name) = 0;
    virtual void addSetPoint(double setpoint) = 0;
    virtual void addRPMCeiling(double ceiling) = 0;
    virtual double getMaxSetPointRequest() const = 0;
    virtual bool getFailSafeMode() const = 0;
    virtual double getFailSafePercent() const = 0;
    virtual Sensor* getSensor(const std::string& name) = 0;
};

} // namespace pid_control
