#pragma once

#include "controller.hpp"
#include "ec/pid.hpp"
#include "fan.hpp"

#include <memory>
#include <vector>

class ZoneInterface;

/*
 * Base class for PID controllers.  Each PID that implements this needs to
 * provide an inputProc, setptProc, and outputProc.
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

    virtual float inputProc(void) = 0;
    virtual float setptProc(void) = 0;
    virtual void outputProc(float value) = 0;

    void process(void);

    std::string getID(void)
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

    ec::pid_info_t* getPIDInfo(void)
    {
        return &_pid_info;
    }

  protected:
    ZoneInterface* _owner;

  private:
    // parameters
    ec::pid_info_t _pid_info;
    float _setpoint;
    std::string _id;
};
