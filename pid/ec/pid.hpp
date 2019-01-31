#pragma once

#include <cstdint>

namespace ec
{

typedef struct
{
    double min;
    double max;
} limits_t;

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp.
 */
typedef struct
{
    bool initialized; // has pid been initialized

    double ts;          // sample time in seconds
    double integral;    // intergal of error
    double last_output; // value of last output

    double p_c;     // coeff for P
    double i_c;     // coeff for I
    double ff_off;  // offset coeff for feed-forward term
    double ff_gain; // gain for feed-forward term

    limits_t i_lim;   // clamp of integral
    limits_t out_lim; // clamp of output
    double slew_neg;
    double slew_pos;
    double positiveHysteresis;
    double negativeHysteresis;
} pid_info_t;

double pid(pid_info_t* pidinfoptr, double input, double setpoint);

/* Condensed version for use by the configuration. */
struct pidinfo
{
    double ts;            // sample time in seconds
    double p_c;           // coeff for P
    double i_c;           // coeff for I
    double ff_off;        // offset coeff for feed-forward term
    double ff_gain;       // gain for feed-forward term
    ec::limits_t i_lim;   // clamp of integral
    ec::limits_t out_lim; // clamp of output
    double slew_neg;
    double slew_pos;
    double positiveHysteresis;
    double negativeHysteresis;
};

} // namespace ec
