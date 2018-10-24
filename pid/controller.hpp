#pragma once

#include "ec/pid.hpp"
#include "fan.hpp"

#include <string>

/*
 * Base class for controllers.  Each controller that implements this needs to
 * provide an input_proc, process, and output_proc.
 */
class ZoneInterface;

struct Controller
{
    virtual ~Controller() = default;

    virtual float inputProc(void) = 0;

    virtual void outputProc(float value) = 0;

    virtual void process(void) = 0;

    virtual std::string getId(void) = 0;
};
