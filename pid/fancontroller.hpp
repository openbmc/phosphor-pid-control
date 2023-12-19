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
 * A FanController is a PID controller that reads a number of fans and given
 * the output then tries to set them to the goal values set by the thermal
 * controllers.
 */
class FanController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        createFanPid(ZoneInterface* owner, const std::string& id,
                     const std::vector<std::string>& inputs,
                     const ec::pidinfo& initial);

    FanController(const std::string& id, const std::vector<std::string>& inputs,
                  ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs), _direction(FanSpeedDirection::NEUTRAL)
    {}

    double inputProc(void) override;
    double setptProc(void) override;
    void outputProc(double value) override;

    FanSpeedDirection getFanDirection(void) const
    {
        return _direction;
    }

    void setFanDirection(FanSpeedDirection direction)
    {
        _direction = direction;
    };

  private:
    std::vector<std::string> _inputs;
    FanSpeedDirection _direction;

    // Cosmetic only, to reduce frequency of repetitive messages
    bool failsafeTransition = true;
    bool failsafePrevState = false;
};

} // namespace pid_control
