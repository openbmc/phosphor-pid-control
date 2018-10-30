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
    info->p_c = initial.p_c;
    info->i_c = initial.i_c;
    info->ff_off = initial.ff_off;
    info->ff_gain = initial.ff_gain;
    info->i_lim.min = initial.i_lim.min;
    info->i_lim.max = initial.i_lim.max;
    info->out_lim.min = initial.out_lim.min;
    info->out_lim.max = initial.out_lim.max;
    info->slew_neg = initial.slew_neg;
    info->slew_pos = initial.slew_pos;
}

void dumpPIDStruct(ec::pid_info_t* info)
{
    std::cerr << " ts: " << info->ts << " p_c: " << info->p_c
              << " i_c: " << info->i_c << " ff_off: " << info->ff_off
              << " ff_gain: " << info->ff_gain
              << " i_lim.min: " << info->i_lim.min
              << " i_lim.max: " << info->i_lim.max
              << " out_lim.min: " << info->out_lim.min
              << " out_lim.max: " << info->out_lim.max
              << " slew_neg: " << info->slew_neg
              << " slew_pos: " << info->slew_pos
              << " last_output: " << info->last_output
              << " integral: " << info->integral << std::endl;

    return;
}
