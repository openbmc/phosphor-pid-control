#pragma once

#include <cstdint>

namespace ec
{

typedef struct
{
    float       min;
    float       max;
} limits_t;

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp.
 */
typedef struct
{
    bool        initialized;        // has pid been initialized

    float       ts;                 // sample time in seconds
    float       integral;           // intergal of error
    float       last_output;        // value of last output

    float       p_c;                // coeff for P
    float       i_c;                // coeff for I
    float       ff_off;             // offset coeff for feed-forward term
    float       ff_gain;            // gain for feed-forward term

    limits_t    i_lim;              // clamp of integral
    limits_t    out_lim;            // clamp of output
    float       slew_neg;
    float       slew_pos;
} pid_info_t;

float pid(pid_info_t* pidinfoptr, float input, float setpoint);

/* Condensed version for use by the configuration. */
struct pidinfo
{
    float        ts;                 // sample time in seconds
    float        p_c;                // coeff for P
    float        i_c;                // coeff for I
    float        ff_off;             // offset coeff for feed-forward term
    float        ff_gain;            // gain for feed-forward term
    ec::limits_t i_lim;              // clamp of integral
    ec::limits_t out_lim;            // clamp of output
    float        slew_neg;
    float        slew_pos;
};


}
