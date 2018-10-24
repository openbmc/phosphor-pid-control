#pragma once

#include "controller.hpp"
#include "ec/pid.hpp"
#include "fan.hpp"

#include <memory>
#include <vector>

class ZoneInterface;

/*
 * Base class for PID controllers.  Each PID that implements this needs to
 * provide an input_proc, setpt_proc, and output_proc.
 */
class PIDController : public Controller
{
  public:
    PIDController(const std::string& id, ZoneInterface* owner) :
        Controller(), _owner(owner), _setpoint(0), _id(id)
    {
    }

    virtual ~PIDController()
    {
    }

    virtual float input_proc(void) = 0;
    virtual float setptProc(void) = 0;
    virtual void output_proc(float value) = 0;

    void process(void);

    std::string get_id(void)
    {
        return _id;
    }
    float getSetpoint(void)
    {
        return _setpoint;
    }
    void setSetpoint(float setpoint)
    {
        _setpoint = setpoint;
    }

    ec::pid_info_t* getPidInfo(void)
    {
        return &_pid_info;
    }

  protected:
    ZoneInterface* _owner;

  private:
    // parameters
    ec::pid_info_t _pidInfo;
    float _setpoint;
    std::string _id;
};
