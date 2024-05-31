#pragma once

#include "conf.hpp"
#include "ec/pid.hpp"
#include "pidcontroller.hpp"

#include <memory>
#include <string>
#include <vector>

namespace pid_control
{

/*
 * A ThermalController is a PID controller that reads a number of sensors and
 * provides the setpoints for the fans.
 * With addition of support for power sensors, this name is misleading,
 * as it now works for power sensors also, not just thermal sensors.
 * If rewritten today, a better name would be "ComputationType".
 */

enum class ThermalType
{
    margin,
    absolute,
    summation
};

/**
 * Get the ThermalType for a given string.
 *
 * @param[in] typeString - a string representation of a type.
 * @return the ThermalType representation.
 */
ThermalType getThermalType(const std::string& typeString);

/**
 * Is the type specified a thermal type?
 *
 * @param[in] typeString - a string representation of a PID type.
 * @return true if it's a thermal PID type.
 */
bool isThermalType(const std::string& typeString);

class ThermalController : public PIDController
{
  public:
    static std::unique_ptr<PIDController> createThermalPid(
        ZoneInterface* owner, const std::string& id,
        const std::vector<pid_control::conf::SensorInput>& inputs,
        const ec::pidinfo& initial, const ThermalType& type, double setpoint);

    ThermalController(const std::string& id,
                      const std::vector<pid_control::conf::SensorInput>& inputs,
                      const ThermalType& type, ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs), type(type)
    {}

    ~ThermalController() = default;

    double inputProc(void) override;
    double setptProc(void) override;
    void outputProc(double value) override;

  private:
    std::vector<pid_control::conf::SensorInput> _inputs;
    ThermalType type;
};

} // namespace pid_control
