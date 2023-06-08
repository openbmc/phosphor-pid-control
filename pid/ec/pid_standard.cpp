/**
 * Copyright 2017 Google Inc.
 * Copyright 2022-2023 Raptor Engineering, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// For pidinfo, pid_info_t, and limits_t structs
#include "pid.hpp"

#include "../tuning.hpp"
#include "logging.hpp"

#include "pid_standard.hpp"

namespace pid_control
{
namespace ec
{

/********************************
 *  clamp
 */
static double clamp(double x, double min, double max)
{
    if (x < min)
    {
        return min;
    }
    else if (x > max)
    {
        return max;
    }
    return x;
}

/********************************
 *  Standard PID algorithm
 *  Note: This implementation assumes the ts field is non-zero
 */
double pid_standard(pid_info_t* pidinfoptr, double input, double setpoint,
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

    double error;

    double proportionalTerm = 0.0f;
    double integralTerm = 0.0f;
    double derivativeTerm = 0.0f;

    double output;

    coreContext.input = input;
    coreContext.setpoint = setpoint;

    // Initialize internal values if needed
    if (!pidinfoptr->initialized)
    {
        pidinfoptr->integral = 0;
        pidinfoptr->lastError = 0;
        pidinfoptr->lastOutput = 0;
    }

    // calculate P, I, D
    error = setpoint - input;

    // Proportional (P)
    proportionalTerm = error * pidinfoptr->proportionalCoeff;

    // Integral (I)
    integralTerm = pidinfoptr->integral;
    integralTerm += (error * pidinfoptr->ts) * pidinfoptr->integralCoeff;
    integralTerm = clamp(integralTerm, pidinfoptr->integralLimit.min,
                         pidinfoptr->integralLimit.max);

    // Derivative (D)
    derivativeTerm = (pidinfoptr->lastError - error) * pidinfoptr->derivativeCoeff;

    coreContext.error = error;
    coreContext.proportionalTerm = proportionalTerm;
    coreContext.integralTerm1 = integralTerm;
    coreContext.derivativeTerm = derivativeTerm;

    output = proportionalTerm + integralTerm + derivativeTerm;
    coreContext.output1 = output;

    output = clamp(output, pidinfoptr->outLim.min, pidinfoptr->outLim.max);
    coreContext.output2 = output;

    pidinfoptr->integral = integralTerm;
    pidinfoptr->lastError = error;
    pidinfoptr->lastOutput = output;
    pidinfoptr->initialized = true;

    coreContext.integralTerm = pidinfoptr->integral;
    coreContext.output = pidinfoptr->lastOutput;;

    if (logPtr)
    {
        LogContext(*logPtr, msNow, coreContext);
    }

    return output;
}

} // namespace ec
} // namespace pid_control
