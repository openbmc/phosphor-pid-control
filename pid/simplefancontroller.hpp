#pragma once

#include "ec/pid.hpp"
#include "fan.hpp"
#include "pidcontroller.hpp"

#include <memory>
#include <string>
#include <vector>

namespace pid_control
{

/*
 * A SimpleFanController is a PID controller that reads a number of fans and
 * given the output then blindly sets the PWM values set by the thermal
 * controllers.
 */
class SimpleFanController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        createFanPid(ZoneInterface* owner, const std::string& id,
                     const std::vector<std::string>& inputs,
                     const ec::pidinfo& initial);

    SimpleFanController(const std::string& id,
                        const std::vector<std::string>& inputs,
                        ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs)
    {}

    double inputProc(void) override;
    double setptProc(void) override;
    void outputProc(double value) override;
    void process(void) override;

  private:
    std::vector<std::string> _inputs;
};

} // namespace pid_control
