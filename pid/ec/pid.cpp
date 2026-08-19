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
 *  pid_positional
 *  Standard positional (absolute) PID algorithm.
 *  This is the original implementation, preserved for backward compatibility.
 */
static double pid_positional(pid_info_t* pidinfoptr, double input,
                             double setpoint, PidCoreContext* coreContext,
                             bool logEnabled)
{
    double error;
    double proportionalTerm;
    double integralTerm = 0.0f;
    double derivativeTerm = 0.0f;
    double feedFwdTerm = 0.0f;
    double output;

    // Pid
    error = setpoint - input;
    proportionalTerm = pidinfoptr->proportionalCoeff * error;

    coreContext->error = error;
    coreContext->proportionalTerm = proportionalTerm;
    coreContext->integralTerm1 = 0.0;

    // pId
    if (0.0f != pidinfoptr->integralCoeff)
    {
        integralTerm = pidinfoptr->integral;
        integralTerm += error * pidinfoptr->integralCoeff * pidinfoptr->ts;

        coreContext->integralTerm1 = integralTerm;

        integralTerm = clamp(integralTerm, pidinfoptr->integralLimit.min,
                             pidinfoptr->integralLimit.max);
    }

    coreContext->integralTerm2 = integralTerm;

    // piD
    derivativeTerm = pidinfoptr->derivativeCoeff *
                     ((error - pidinfoptr->lastError) / pidinfoptr->ts);

    coreContext->derivativeTerm = derivativeTerm;

    // FF
    feedFwdTerm = (setpoint + pidinfoptr->feedFwdOffset) *
                  pidinfoptr->feedFwdGain;

    coreContext->feedFwdTerm = feedFwdTerm;

    output = proportionalTerm + integralTerm + derivativeTerm + feedFwdTerm;

    coreContext->output1 = output;

    output = clamp(output, pidinfoptr->outLim.min, pidinfoptr->outLim.max);

    coreContext->output2 = output;

    coreContext->minOut = 0.0;
    coreContext->maxOut = 0.0;

    // slew rate
    if (pidinfoptr->initialized)
    {
        if (pidinfoptr->slewNeg != 0.0f)
        {
            double minOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewNeg * pidinfoptr->ts;

            coreContext->minOut = minOut;

            if (output < minOut)
            {
                output = minOut;
            }
        }
        if (pidinfoptr->slewPos != 0.0f)
        {
            double maxOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewPos * pidinfoptr->ts;

            coreContext->maxOut = maxOut;

            if (output > maxOut)
            {
                output = maxOut;
            }
        }

        if (pidinfoptr->slewNeg != 0.0f || pidinfoptr->slewPos != 0.0f)
        {
            integralTerm = output - proportionalTerm;
        }
    }

    coreContext->output3 = output;
    coreContext->integralTerm3 = integralTerm;

    integralTerm = clamp(integralTerm, pidinfoptr->integralLimit.min,
                         pidinfoptr->integralLimit.max);
    pidinfoptr->integral = integralTerm;
    pidinfoptr->initialized = true;
    pidinfoptr->lastError = error;
    pidinfoptr->lastOutput = output;

    coreContext->integralTerm = pidinfoptr->integral;
    coreContext->output = pidinfoptr->lastOutput;

    return output;
}

/********************************
 *  pid_incremental
 *  Incremental (velocity) PID algorithm.
 *  Computes delta output and adds to previous output.
 *  Formula:
 *    deltaP = Kp * (e[k] - e[k-1])
 *    deltaI = Ki * e[k] * Ts
 *    deltaD = -Kd * (input[k] - 2*input[k-1] + input[k-2]) / Ts
 *    deltaFF = FF[k] - FF[k-1]
 *    output = lastOutput + deltaP + deltaI + deltaD + deltaFF
 *
 *  Note: Derivative is computed on input (temperature) rather than error
 *  to avoid "derivative kick" when setpoint changes.
 */
static double pid_incremental(pid_info_t* pidinfoptr, double input,
                              double setpoint, PidCoreContext* coreContext,
                              bool logEnabled)
{
    double error;
    double deltaProportional = 0.0;
    double deltaIntegral = 0.0;
    double deltaDerivative = 0.0;
    double deltaFeedFwd = 0.0;
    double output;

    // Calculate current error
    error = setpoint - input;

    coreContext->error = error;

    // deltaP = Kp * (e[k] - e[k-1])
    if (pidinfoptr->initialized)
    {
        deltaProportional =
            pidinfoptr->proportionalCoeff * (error - pidinfoptr->lastError);
    }
    else
    {
        // First call: treat lastError as 0
        deltaProportional = pidinfoptr->proportionalCoeff * error;
    }

    coreContext->proportionalTerm = deltaProportional;
    coreContext->integralTerm1 = 0.0;

    // deltaI = Ki * e[k] * Ts
    if (0.0f != pidinfoptr->integralCoeff)
    {
        deltaIntegral =
            pidinfoptr->integralCoeff * error * pidinfoptr->ts;

        // Accumulate integral for limit checking
        double newIntegral = pidinfoptr->integral + deltaIntegral;

        coreContext->integralTerm1 = newIntegral;

        newIntegral = clamp(newIntegral, pidinfoptr->integralLimit.min,
                            pidinfoptr->integralLimit.max);

        // Recalculate deltaIntegral after clamping
        deltaIntegral = newIntegral - pidinfoptr->integral;
        pidinfoptr->integral = newIntegral;
    }

    coreContext->integralTerm2 = pidinfoptr->integral;

    // deltaD = -Kd * (input[k] - 2*input[k-1] + input[k-2]) / Ts
    // Derivative on input (temperature) to avoid derivative kick on setpoint change
    if (pidinfoptr->initialized)
    {
        deltaDerivative =
            -pidinfoptr->derivativeCoeff *
            (input - 2.0 * pidinfoptr->lastInput + pidinfoptr->lastLastInput) /
            pidinfoptr->ts;
    }
    else
    {
        // First call: use error-based derivative to match positional.
        // The input-based derivative formula is only equivalent to the
        // error-based one when setpoint is constant AND valid history
        // exists. On the first call neither condition holds, so fall
        // back to the positional derivative: Kd * error / ts.
        deltaDerivative = pidinfoptr->derivativeCoeff * error / pidinfoptr->ts;
    }

    coreContext->derivativeTerm = deltaDerivative;

    // deltaFF = FF[k] - FF[k-1]
    {
        double feedFwdCurrent =
            (setpoint + pidinfoptr->feedFwdOffset) * pidinfoptr->feedFwdGain;
        double feedFwdPrevious;
        if (pidinfoptr->initialized)
        {
            feedFwdPrevious =
                (pidinfoptr->lastSetpoint + pidinfoptr->feedFwdOffset) *
                pidinfoptr->feedFwdGain;
        }
        else
        {
            feedFwdPrevious = 0.0;
        }
        deltaFeedFwd = feedFwdCurrent - feedFwdPrevious;
    }

    coreContext->feedFwdTerm = deltaFeedFwd;

    // Sum deltas
    double deltaOutput =
        deltaProportional + deltaIntegral + deltaDerivative + deltaFeedFwd;

    coreContext->output1 = deltaOutput;

    // Apply delta to last output
    if (pidinfoptr->initialized)
    {
        output = pidinfoptr->lastOutput + deltaOutput;
    }
    else
    {
        // First call: start from 0
        output = deltaOutput;
    }

    // Clamp output
    output = clamp(output, pidinfoptr->outLim.min, pidinfoptr->outLim.max);

    coreContext->output2 = output;

    coreContext->minOut = 0.0;
    coreContext->maxOut = 0.0;

    // Slew rate: limit delta from last output
    if (pidinfoptr->initialized)
    {
        if (pidinfoptr->slewNeg != 0.0f)
        {
            double minOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewNeg * pidinfoptr->ts;

            coreContext->minOut = minOut;

            if (output < minOut)
            {
                output = minOut;
            }
        }
        if (pidinfoptr->slewPos != 0.0f)
        {
            double maxOut = pidinfoptr->lastOutput +
                            pidinfoptr->slewPos * pidinfoptr->ts;

            coreContext->maxOut = maxOut;

            if (output > maxOut)
            {
                output = maxOut;
            }
        }
    }

    // Slew rate back-calculation (matches positional PID behavior).
    // When slew is configured, the positional PID recalculates the integral
    // as output - proportionalTerm, effectively absorbing D and FF terms.
    // The incremental PID must do the same to maintain equivalence.
    if (pidinfoptr->initialized &&
        (pidinfoptr->slewNeg != 0.0f || pidinfoptr->slewPos != 0.0f))
    {
        double proportionalTerm = pidinfoptr->proportionalCoeff * error;
        pidinfoptr->integral = output - proportionalTerm;
    }

    coreContext->output3 = output;
    coreContext->integralTerm3 = pidinfoptr->integral;

    // Final integral clamp (matches positional PID)
    pidinfoptr->integral = clamp(pidinfoptr->integral,
                                  pidinfoptr->integralLimit.min,
                                  pidinfoptr->integralLimit.max);

    // Update state
    if (pidinfoptr->initialized)
    {
        pidinfoptr->lastLastInput = pidinfoptr->lastInput;
    }
    else
    {
        // First call: the positional PID's default lastError=0 implies
        // lastInput = setpoint (since error = setpoint - input). Set
        // lastLastInput = setpoint to keep the pre-history consistent,
        // so the input-based derivative matches the error-based one on
        // the next call when setpoint is constant.
        pidinfoptr->lastLastInput = setpoint;
    }
    pidinfoptr->lastInput = input;
    pidinfoptr->lastError = error;
    pidinfoptr->lastSetpoint = setpoint;
    pidinfoptr->lastOutput = output;
    pidinfoptr->initialized = true;

    coreContext->integralTerm = pidinfoptr->integral;
    coreContext->output = pidinfoptr->lastOutput;

    return output;
}

/********************************
 *  pid code
 *  Note: Codes assumes the ts field is non-zero
 *  Dispatches to positional or incremental algorithm based on
 *  pidinfoptr->incrementalMode.
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
    coreContext.incrementalMode = pidinfoptr->incrementalMode;

    double output;

    if (pidinfoptr->incrementalMode)
    {
        output = pid_incremental(pidinfoptr, input, setpoint, &coreContext,
                                 logPtr != nullptr);
    }
    else
    {
        output = pid_positional(pidinfoptr, input, setpoint, &coreContext,
                                logPtr != nullptr);
    }

    if (logPtr)
    {
        LogContext(*logPtr, msNow, coreContext);
    }

    return output;
}

} // namespace ec
} // namespace pid_control
