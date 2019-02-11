/**
 * Copyright 2017 Google Inc.
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

#include "pid.hpp"

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
    else if (x > max)
    {
        return max;
    }
    return x;
}

/********************************
 *  pid code
 *  Note: Codes assumes the ts field is non-zero
 */
double pid(pid_info_t* pidinfoptr, double input, double setpoint)
{
    double error;

    double p_term;
    double i_term = 0.0f;
    double ff_term = 0.0f;

    double output;

    // calculate P, I, D, FF

    // Pid
    error = setpoint - input;
    p_term = pidinfoptr->proportionalCoeff * error;

    // pId
    if (0.0f != pidinfoptr->integralCoeff)
    {
        i_term = pidinfoptr->integral;
        i_term += error * pidinfoptr->integralCoeff * pidinfoptr->ts;
        i_term = clamp(i_term, pidinfoptr->integralLimit.min,
                       pidinfoptr->integralLimit.max);
    }

    // FF
    ff_term = (setpoint + pidinfoptr->feedFwdOffset) * pidinfoptr->feedFwdGain;

    output = p_term + i_term + ff_term;
    output = clamp(output, pidinfoptr->outLim.min, pidinfoptr->outLim.max);

    // slew rate
    // TODO(aarena) - Simplify logic as Andy suggested by creating dynamic
    // out_lim_min/max that are affected by slew rate control and just clamping
    // to those instead of effectively clamping twice.
    if (pidinfoptr->initialized)
    {
        if (pidinfoptr->slewNeg != 0.0f)
        {
            // Don't decrease too fast
            double min_out =
                pidinfoptr->lastOutput + pidinfoptr->slewNeg * pidinfoptr->ts;
            if (output < min_out)
            {
                output = min_out;
            }
        }
        if (pidinfoptr->slewPos != 0.0f)
        {
            // Don't increase too fast
            double max_out =
                pidinfoptr->lastOutput + pidinfoptr->slewPos * pidinfoptr->ts;
            if (output > max_out)
            {
                output = max_out;
            }
        }

        if (pidinfoptr->slewNeg != 0.0f || pidinfoptr->slewPos != 0.0f)
        {
            // Back calculate integral term for the cases where we limited the
            // output
            i_term = output - p_term;
        }
    }

    // Clamp again because having limited the output may result in a
    // larger integral term
    i_term = clamp(i_term, pidinfoptr->integralLimit.min,
                   pidinfoptr->integralLimit.max);
    pidinfoptr->integral = i_term;
    pidinfoptr->initialized = true;
    pidinfoptr->lastOutput = output;

    return output;
}

} // namespace ec
