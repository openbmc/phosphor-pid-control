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
static float clamp(float x, float min, float max)
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
float pid(pid_info_t* pidinfoptr, float input, float setpoint)
{
    float error;

    float pTerm;
    float iTerm = 0.0f;
    float ffTerm = 0.0f;

    float output;

    // calculate P, I, D, FF

    // Pid
    error = setpoint - input;
    pTerm = pidinfoptr->p_c * error;

    // pId
    if (0.0f != pidinfoptr->i_c)
    {
        iTerm = pidinfoptr->integral;
        iTerm += error * pidinfoptr->i_c * pidinfoptr->ts;
        iTerm = clamp(iTerm, pidinfoptr->i_lim.min, pidinfoptr->i_lim.max);
    }

    // FF
    ffTerm = (setpoint + pidinfoptr->ff_off) * pidinfoptr->ff_gain;

    output = pTerm + iTerm + ffTerm;
    output = clamp(output, pidinfoptr->out_lim.min, pidinfoptr->out_lim.max);

    // slew rate
    // TODO(aarena) - Simplify logic as Andy suggested by creating dynamic
    // out_lim_min/max that are affected by slew rate control and just clamping
    // to those instead of effectively clamping twice.
    if (pidinfoptr->initialized)
    {
        if (pidinfoptr->slew_neg != 0.0f)
        {
            // Don't decrease too fast
            float minOut =
                pidinfoptr->last_output + pidinfoptr->slew_neg * pidinfoptr->ts;
            if (output < minOut)
            {
                output = minOut;
            }
        }
        if (pidinfoptr->slew_pos != 0.0f)
        {
            // Don't increase too fast
            float maxOut =
                pidinfoptr->last_output + pidinfoptr->slew_pos * pidinfoptr->ts;
            if (output > maxOut)
            {
                output = maxOut;
            }
        }

        if (pidinfoptr->slew_neg != 0.0f || pidinfoptr->slew_pos != 0.0f)
        {
            // Back calculate integral term for the cases where we limited the
            // output
            iTerm = output - pTerm;
        }
    }

    // Clamp again because having limited the output may result in a
    // larger integral term
    iTerm = clamp(iTerm, pidinfoptr->i_lim.min, pidinfoptr->i_lim.max);
    pidinfoptr->integral = iTerm;
    pidinfoptr->initialized = true;
    pidinfoptr->last_output = output;

    return output;
}

} // namespace ec
