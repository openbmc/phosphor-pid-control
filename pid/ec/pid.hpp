#pragma once

#include <cstdint>

namespace ec
{

typedef struct
{
    float _min;
    float _max;
} LimitsT;

/* Note: If you update these structs you need to update the copy code in
 * pid/util.cpp.
 */
typedef struct
{
    bool _initialized; // has pid been initialized

    float _ts;         // sample time in seconds
    float _integral;   // intergal of error
    float _lastOutput; // value of last output

    float _pC;     // coeff for P
    float _iC;     // coeff for I
    float _ffOff;  // offset coeff for feed-forward term
    float _ffGain; // gain for feed-forward term

    LimitsT _iLim;   // clamp of integral
    LimitsT _outLim; // clamp of output
    float _slewNeg;
    float _slewPos;
} PidInfoT;

float pid(pid_info_t* pidinfoptr, float input, float setpoint);

/* Condensed version for use by the configuration. */
struct Pidinfo
{
    float _ts;                 // sample time in seconds
    float _pC;                 // coeff for P
    float _iC;                 // coeff for I
    float _ffOff;              // offset coeff for feed-forward term
    float _ffGain;             // gain for feed-forward term
    LimitsT::limits_t _iLim;   // clamp of integral
    LimitsT::limits_t _outLim; // clamp of output
    float _slewNeg;
    float _slewPos;
};

} // namespace ec
