#pragma once

#include <cstdint>
#include <string>

namespace pid_control
{
namespace ec
{

typedef struct
{
    double min = 0.0;
    double max = 0.0;
} limits_t;

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp and the initialization code in pid/buildjson.hpp files.
 */
typedef struct
{
    bool initialized = false;          // has pid been initialized
    bool checkHysterWithSetpt = false; // compare current input and setpoint to
                                       // check hysteresis

    double ts = 0.0;                   // sample time in seconds
    double integral = 0.0;             // integral of error
    double lastOutput = 0.0;           // value of last output
    double lastError = 0.0;            // value of last error

    double proportionalCoeff = 0.0;    // coeff for P
    double integralCoeff = 0.0;        // coeff for I
    double derivativeCoeff = 0.0;      // coeff for D
    double feedFwdOffset = 0.0;        // offset coeff for feed-forward term
    double feedFwdGain = 0.0;          // gain for feed-forward term

    limits_t integralLimit;            // clamp of integral
    limits_t outLim;                   // clamp of output
    double slewNeg = 0.0;
    double slewPos = 0.0;
    double positiveHysteresis = 0.0;
    double negativeHysteresis = 0.0;
} pid_info_t;

double pid(pid_info_t* pidinfoptr, double input, double setpoint,
           const std::string* nameptr = nullptr);

/* Condensed version for use by the configuration. */
struct pidinfo
{
    bool checkHysterWithSetpt = false; // compare current input and setpoint to
                                       // check hysteresis

    double ts = 0.0;                // sample time in seconds
    double proportionalCoeff = 0.0; // coeff for P
    double integralCoeff = 0.0;     // coeff for I
    double derivativeCoeff = 0.0;   // coeff for D
    double feedFwdOffset = 0.0;     // offset coeff for feed-forward term
    double feedFwdGain = 0.0;       // gain for feed-forward term
    ec::limits_t integralLimit;     // clamp of integral
    ec::limits_t outLim;            // clamp of output
    double slewNeg = 0.0;
    double slewPos = 0.0;
    double positiveHysteresis = 0.0;
    double negativeHysteresis = 0.0;
};

} // namespace ec
} // namespace pid_control
