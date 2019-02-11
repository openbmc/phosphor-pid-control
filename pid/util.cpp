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

#include "ec/pid.hpp"

#include <cstring>
#include <iostream>

void initializePIDStruct(ec::pid_info_t* info, const ec::pidinfo& initial)
{
    std::memset(info, 0x00, sizeof(ec::pid_info_t));

    info->ts = initial.ts;
    info->proportionalCoeff = initial.p_c;
    info->integralCoeff = initial.i_c;
    info->feedFwdOffset = initial.ff_off;
    info->feedFwdGain = initial.ff_gain;
    info->integralLimit.min = initial.i_lim.min;
    info->integralLimit.max = initial.i_lim.max;
    info->outLim.min = initial.out_lim.min;
    info->outLim.max = initial.out_lim.max;
    info->slewNeg = initial.slew_neg;
    info->slewPos = initial.slew_pos;
    info->negativeHysteresis = initial.negativeHysteresis;
    info->positiveHysteresis = initial.positiveHysteresis;
}

void dumpPIDStruct(ec::pid_info_t* info)
{
    std::cerr << " ts: " << info->ts << " p_c: " << info->proportionalCoeff
              << " i_c: " << info->integralCoeff
              << " ff_off: " << info->feedFwdOffset
              << " ff_gain: " << info->feedFwdGain
              << " i_lim.min: " << info->integralLimit.min
              << " i_lim.max: " << info->integralLimit.max
              << " out_lim.min: " << info->outLim.min
              << " out_lim.max: " << info->outLim.max
              << " slew_neg: " << info->slewNeg
              << " slew_pos: " << info->slewPos
              << " last_output: " << info->lastOutput
              << " integral: " << info->integral << std::endl;

    return;
}
