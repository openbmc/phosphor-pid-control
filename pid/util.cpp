// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "util.hpp"

#include "ec/pid.hpp"

#include <cstring>
#include <iostream>

namespace pid_control
{

void initializePIDStruct(ec::pid_info_t* info, const ec::pidinfo& initial)
{
    info->checkHysterWithSetpt = initial.checkHysterWithSetpt;
    info->ts = initial.ts;
    info->proportionalCoeff = initial.proportionalCoeff;
    info->integralCoeff = initial.integralCoeff;
    info->derivativeCoeff = initial.derivativeCoeff;
    info->feedFwdOffset = initial.feedFwdOffset;
    info->feedFwdGain = initial.feedFwdGain;
    info->integralLimit.min = initial.integralLimit.min;
    info->integralLimit.max = initial.integralLimit.max;
    info->outLim.min = initial.outLim.min;
    info->outLim.max = initial.outLim.max;
    info->slewNeg = initial.slewNeg;
    info->slewPos = initial.slewPos;
    info->negativeHysteresis = initial.negativeHysteresis;
    info->positiveHysteresis = initial.positiveHysteresis;
}

void dumpPIDStruct(ec::pid_info_t* info)
{
    std::cerr << " ts: " << info->ts
              << " proportionalCoeff: " << info->proportionalCoeff
              << " integralCoeff: " << info->integralCoeff
              << " derivativeCoeff: " << info->derivativeCoeff
              << " feedFwdOffset: " << info->feedFwdOffset
              << " feedFwdGain: " << info->feedFwdGain
              << " integralLimit.min: " << info->integralLimit.min
              << " integralLimit.max: " << info->integralLimit.max
              << " outLim.min: " << info->outLim.min
              << " outLim.max: " << info->outLim.max
              << " slewNeg: " << info->slewNeg << " slewPos: " << info->slewPos
              << " last_output: " << info->lastOutput
              << " last_error: " << info->lastError
              << " integral: " << info->integral << std::endl;

    return;
}

} // namespace pid_control
