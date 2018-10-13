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

    float p_term;
    float i_term = 0.0f;
    float ff_term = 0.0f;

    float output;

    // calculate P, I, D, FF

    // Pid
    error = setpoint - input;
    p_term = pidinfoptr->p_c * error;

    // pId
    if (0.0f != pidinfoptr->i_c)
    {
        i_term = pidinfoptr->integral;
        i_term += error * pidinfoptr->i_c * pidinfoptr->ts;
        i_term = clamp(i_term, pidinfoptr->i_lim.min, pidinfoptr->i_lim.max);
    }

    // FF
    ff_term = (setpoint + pidinfoptr->ff_off) * pidinfoptr->ff_gain;

    output = p_term + i_term + ff_term;
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
            float min_out =
                pidinfoptr->last_output + pidinfoptr->slew_neg * pidinfoptr->ts;
            if (output < min_out)
            {
                output = min_out;
            }
        }
        if (pidinfoptr->slew_pos != 0.0f)
        {
            // Don't increase too fast
            float max_out =
                pidinfoptr->last_output + pidinfoptr->slew_pos * pidinfoptr->ts;
            if (output > max_out)
            {
                output = max_out;
            }
        }

        if (pidinfoptr->slew_neg != 0.0f || pidinfoptr->slew_pos != 0.0f)
        {
            // Back calculate integral term for the cases where we limited the
            // output
            i_term = output - p_term;
        }
    }

    // Clamp again because having limited the output may result in a
    // larger integral term
    i_term = clamp(i_term, pidinfoptr->i_lim.min, pidinfoptr->i_lim.max);
    pidinfoptr->integral = i_term;
    pidinfoptr->initialized = true;
    pidinfoptr->last_output = output;

    return output;
}

} // namespace ec
