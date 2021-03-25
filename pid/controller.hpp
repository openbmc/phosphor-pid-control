#pragma once

#include "ec/pid.hpp"
#include "fan.hpp"

#include <string>

namespace pid_control
{

/*
 * Base class for controllers.  Each controller that implements this needs to
 * provide an inputProc, process, and outputProc.
 */
class ZoneInterface;

struct Controller
{
    virtual ~Controller() = default;

    virtual double inputProc(void) = 0;

    virtual void outputProc(double value, bool forceUpdate) = 0;

    virtual void process(bool forceUpdate) = 0;

    virtual std::string getID(void) = 0;
};

} // namespace pid_control
