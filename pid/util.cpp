// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "util.hpp"

#include "ec/pid.hpp"

#include <phosphor-logging/lg2.hpp>

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
    lg2::debug(
        "ts: {TS} proportionalCoeff: {PCOEFF} integralCoeff: {ICOEFF} "
        "derivativeCoeff: {DCOEFF} feedFwdOffset: {FF_OFFSET} "
        "feedFwdGain: {FF_GAIN} integralLimit.min: {ILIM_MIN} "
        "integralLimit.max: {ILIM_MAX} outLim.min: {OLIM_MIN} "
        "outLim.max: {OLIM_MAX} slewNeg: {SLEW_NEG} "
        "slewPos: {SLEW_POS} last_output: {LAST_OUT} "
        "last_error: {LAST_ERR} integral: {INTEGRAL}",
        "TS", info->ts, "PCOEFF", info->proportionalCoeff, "ICOEFF",
        info->integralCoeff, "DCOEFF", info->derivativeCoeff, "FF_OFFSET",
        info->feedFwdOffset, "FF_GAIN", info->feedFwdGain, "ILIM_MIN",
        info->integralLimit.min, "ILIM_MAX", info->integralLimit.max,
        "OLIM_MIN", info->outLim.min, "OLIM_MAX", info->outLim.max, "SLEW_NEG",
        info->slewNeg, "SLEW_POS", info->slewPos, "LAST_OUT", info->lastOutput,
        "LAST_ERR", info->lastError, "INTEGRAL", info->integral);
    return;
}

} // namespace pid_control
