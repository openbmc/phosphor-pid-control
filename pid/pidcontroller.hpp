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
        Controller(), _owner(owner), _id(id)
    {
        _pid_info.initialized = false;
        _pid_info.checkHysteresisWithSetpoint = false;
        _pid_info.ts = static_cast<double>(0.0);
        _pid_info.integral = static_cast<double>(0.0);
        _pid_info.lastOutput = static_cast<double>(0.0);
        _pid_info.proportionalCoeff = static_cast<double>(0.0);
        _pid_info.integralCoeff = static_cast<double>(0.0);
        _pid_info.derivativeCoeff = static_cast<double>(0.0);
        _pid_info.feedFwdOffset = static_cast<double>(0.0);
        _pid_info.feedFwdGain = static_cast<double>(0.0);
        _pid_info.integralLimit.min = static_cast<double>(0.0);
        _pid_info.integralLimit.max = static_cast<double>(0.0);
        _pid_info.outLim.min = static_cast<double>(0.0);
        _pid_info.outLim.max = static_cast<double>(0.0);
        _pid_info.slewNeg = static_cast<double>(0.0);
        _pid_info.slewPos = static_cast<double>(0.0);
        _pid_info.negativeHysteresis = static_cast<double>(0.0);
        _pid_info.positiveHysteresis = static_cast<double>(0.0);
    }

    virtual ~PIDController() {}

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
        return &_pid_info;
    }

    double getLastInput(void)
    {
        return lastInput;
    }

    double calPIDOutput(double setpt, double input, ec::pid_info_t* info);

  protected:
    ZoneInterface* _owner;
    std::string _id;

  private:
    // parameters
    ec::pid_info_t _pid_info;
    double _setpoint = 0;
    double lastInput = std::numeric_limits<double>::quiet_NaN();
};

} // namespace pid_control
