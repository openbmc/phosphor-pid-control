#pragma once

#include "ec/pid.hpp"
#include "pidcontroller.hpp"

#include <memory>
#include <string>
#include <vector>

/*
 * A ThermalController is a PID controller that reads a number of sensors and
 * provides the set-points for the fans.
 */
enum class ThermalType
{
    margin,
    absolute
};

class ThermalController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        createThermalPid(ZoneInterface* owner, const std::string& id,
                         const std::vector<std::string>& inputs,
                         double setpoint, const ec::pidinfo& initial,
                         const ThermalType& type);

    ThermalController(const std::string& id,
                      const std::vector<std::string>& inputs,
                      const ThermalType& type, ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs), type(type)
    {
    }

    double inputProc(void) override;
    double setptProc(void) override;
    void outputProc(double value) override;

  private:
    std::vector<std::string> _inputs;
    ThermalType type;
};
