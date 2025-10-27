// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "pid.hpp"

#include "logging.hpp"

#include <chrono>
#include <string>

namespace pid_control
{
namespace ec
{

/********************************
 *  clamp
 *
 */
static double clamp(double x, double min, double max)
{
    if (x < min)
    {
        return min;
    }
    if (x > max)
    {
        return max;
    }
    return x;
}

/********************************
 *  pid code
 *  Note: Codes assumes the ts field is non-zero
 */
double pid(pid_info_t* pidinfoptr, double input, double setpoint,
           const std::string* nameptr)
{
    if (nameptr)
    {
        if (!(pidinfoptr->initialized))
        {
            LogInit(*nameptr, pidinfoptr);
        }
    }

    auto logPtr = nameptr ? LogPeek(*nameptr) : nullptr;

    PidCoreContext coreContext;
    std::chrono::milliseconds msNow;

    if (logPtr)
    {
        msNow = LogTimestamp();
    }

    coreContext.input = input;
    coreContext.setpoint = setpoint;

    double error;

    double proportionalTerm;
    double integralTerm = 0.0f;
    double derivativeTerm = 0.0f;
    double feedFwdTerm = 0.0f;

    double output;

    // calculate P, I, D, FF

    // Pid
    error = setpoint - input;
    proportionalTerm = pidinfoptr->proportionalCoeff * error;

    coreContext.error = error;
    coreContext.proportionalTerm = proportionalTerm;
    coreContext.integralTerm1 = 0.0;

    // pId
    if (0.0f != pidinfoptr->integralCoeff)
    {
        integralTerm = pidinfoptr->integral;
        integralTerm += error * pidinfoptr->integralCoeff * pidinfoptr->ts;

        coreContext.integralTerm1 = integralTerm;

        integralTerm = clamp(integralTerm, pidinfoptr->integralLimit.min,
                             pidinfoptr->integralLimit.max);
    }

    coreContext.integralTerm2 = integralTerm;

    // piD
    derivativeTerm = pidinfoptr->derivativeCoeff *
                     ((error - pidinfoptr->lastError) / pidinfoptr->ts);

    coreContext.derivativeTerm = derivativeTerm;

    // FF
    feedFwdTerm = (setpoint + pidinfoptr->feedFwdOffset) *
                  pidinfoptr->feedFwdGain;

    coreContext.feedFwdTerm = feedFwdTerm;

    output = proportionalTerm + integralTerm + derivativeTerm + feedFwdTerm;

    coreContext.output1 = output;

    output = clamp(output, pidinfoptr->outLim.min, pidinfoptr->outLim.max);

    coreContext.output2 = output;

    coreContext.minOut = 0.0;
    coreContext.maxOut = 0.0;

    // slew rate
    // TODO(aarena) - Simplify logic as Andy suggested by creating dynamic
    // outLim_min/max that are affected by slew rate control and just clamping
    // to those instead of effectively clamping twice.
    if (pidinfoptr->initialized)
    {
        if (pidinfoptr->slewNeg != 0.0f)
        {
            // Don't decrease too fast
            double minOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewNeg * pidinfoptr->ts;

            coreContext.minOut = minOut;

            if (output < minOut)
            {
                output = minOut;
            }
        }
        if (pidinfoptr->slewPos != 0.0f)
        {
            // Don't increase too fast
            double maxOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewPos * pidinfoptr->ts;

            coreContext.maxOut = maxOut;

            if (output > maxOut)
            {
                output = maxOut;
            }
        }

        if (pidinfoptr->slewNeg != 0.0f || pidinfoptr->slewPos != 0.0f)
        {
            // Back calculate integral term for the cases where we limited the
            // output
            integralTerm = output - proportionalTerm;
        }
    }

    coreContext.output3 = output;
    coreContext.integralTerm3 = integralTerm;

    // Clamp again because having limited the output may result in a
    // larger integral term
    integralTerm = clamp(integralTerm, pidinfoptr->integralLimit.min,
                         pidinfoptr->integralLimit.max);
    pidinfoptr->integral = integralTerm;
    pidinfoptr->initialized = true;
    pidinfoptr->lastError = error;
    pidinfoptr->lastOutput = output;

    coreContext.integralTerm = pidinfoptr->integral;
    coreContext.output = pidinfoptr->lastOutput;

    if (logPtr)
    {
        LogContext(*logPtr, msNow, coreContext);
    }

    return output;
}

} // namespace ec
} // namespace pid_control
