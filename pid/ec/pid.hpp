#pragma once

#include <cstdint>

namespace pid_control
{
namespace ec
{

using limits_t = struct
{
    double min;
    double max;
};

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp.
 */
struct pid_info_t
{
    bool initialized; // has pid been initialized

    double ts;         // sample time in seconds
    double integral;   // intergal of error
    double lastOutput; // value of last output

    double proportionalCoeff; // coeff for P
    double integralCoeff;     // coeff for I
    double feedFwdOffset;     // offset coeff for feed-forward term
    double feedFwdGain;       // gain for feed-forward term

    limits_t integralLimit; // clamp of integral
    limits_t outLim;        // clamp of output
    double slewNeg;
    double slewPos;
    double positiveHysteresis;
    double negativeHysteresis;
};

double pid(pid_info_t* pidinfoptr, double input, double setpoint);

/* Condensed version for use by the configuration. */
struct Pidinfo
{
    double ts;                  // sample time in seconds
    double proportionalCoeff;   // coeff for P
    double integralCoeff;       // coeff for I
    double feedFwdOffset;       // offset coeff for feed-forward term
    double feedFwdGain;         // gain for feed-forward term
    ec::limits_t integralLimit; // clamp of integral
    ec::limits_t outLim;        // clamp of output
    double slewNeg;
    double slewPos;
    double positiveHysteresis;
    double negativeHysteresis;
};

} // namespace ec
} // namespace pid_control
