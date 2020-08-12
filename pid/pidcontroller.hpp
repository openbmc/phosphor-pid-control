#pragma once

#include "controller.hpp"
#include "ec/pid.hpp"
#include "fan.hpp"

#include <limits>
#include <memory>
#include <vector>

namespace pid_control
{

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
    {}

    virtual ~PIDController() = default;

    virtual double inputProc(void) override = 0;
    virtual double setptProc(void) = 0;
    virtual void outputProc(double value) override = 0;

    void process(void) override;

    std::string getID(void) override
    {
        return _id;
    }
    double getSetpoint(void)
    {
        return _setpoint;
    }
    void setSetpoint(double setpoint)
    {
        _setpoint = setpoint;
    }

    ec::pid_info_t* getPIDInfo(void)
    {
        return &_pidInfo;
    }

    double getLastInput(void)
    {
        return _lastInput;
    }

  protected:
    ZoneInterface* _owner;

  private:
    // parameters
    ec::pid_info_t _pidInfo;
    double _setpoint;
    std::string _id;
    double _lastInput = std::numeric_limits<double>::quiet_NaN();
};

} // namespace pid_control
