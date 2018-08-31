#pragma once

#include "ec/pid.hpp"
#include "fan.hpp"
#include "pidcontroller.hpp"

#include <memory>
#include <string>
#include <vector>

/*
 * A FanController is a PID controller that reads a number of fans and given
 * the output then tries to set them to the goal values set by the thermal
 * controllers.
 */
class FanController : public PIDController
{
  public:
    static std::unique_ptr<PIDController>
        CreateFanPid(ZoneInterface* owner, const std::string& id,
                     std::vector<std::string>& inputs, ec::pidinfo initial);

    FanController(const std::string& id, std::vector<std::string>& inputs,
                  ZoneInterface* owner) :
        PIDController(id, owner),
        _inputs(inputs), _direction(FanSpeedDirection::NEUTRAL)
    {
    }

    float input_proc(void) override;
    float setpt_proc(void) override;
    void output_proc(float value) override;

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
};
