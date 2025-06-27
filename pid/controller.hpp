#pragma once

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

    virtual void outputProc(double value) = 0;

    virtual void process(void) = 0;

    virtual std::string getID(void) = 0;
};

} // namespace pid_control
