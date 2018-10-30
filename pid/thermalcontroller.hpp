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
class ThermalController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        createThermalPid(ZoneInterface* owner, const std::string& id,
                         const std::vector<std::string>& inputs, float setpoint,
                         const ec::pidinfo& initial);

    ThermalController(const std::string& id,
                      const std::vector<std::string>& inputs,
                      ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs)
    {
    }

    float inputProc(void) override;
    float setptProc(void) override;
    void outputProc(float value) override;

  private:
    std::vector<std::string> _inputs;
};
