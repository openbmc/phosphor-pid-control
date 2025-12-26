/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "stepwisecontroller.hpp"

#include "controller.hpp"
#include "ec/stepwise.hpp"
#include "errors/exception.hpp"
#include "tuning.hpp"
#include "util.hpp"
#include "zone.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace pid_control
{

void StepwiseController::process(void)
{
    // Get input value
    double input = inputProc();

    ec::StepwiseInfo info = getStepwiseInfo();

    double output = lastOutput;

    // Calculate new output if hysteresis allows
    if (std::isnan(output))
    {
        output = ec::stepwise(info, input);
        lastInput = input;
    }
    else if ((input - lastInput) > info.positiveHysteresis)
    {
        output = ec::stepwise(info, input);
        lastInput = input;
    }
    else if ((lastInput - input) > info.negativeHysteresis)
    {
        output = ec::stepwise(info, input);
        lastInput = input;
    }

    lastOutput = output;
    // Output new value
    outputProc(output);

    return;
}

std::unique_ptr<Controller> StepwiseController::createStepwiseController(
    ZoneInterface* owner, const std::string& id,
    const std::vector<std::string>& inputs, const ec::StepwiseInfo& initial)
{
    // StepwiseController requires at least 1 input
    if (inputs.empty())
    {
        throw ControllerBuildException("Stepwise controller missing inputs");
    }

    auto thermal = std::make_unique<StepwiseController>(id, inputs, owner);
    thermal->setStepwiseInfo(initial);

    return thermal;
}

double StepwiseController::inputProc(void)
{
    double value = std::numeric_limits<double>::lowest();
    std::string leaderName = _inputs.empty() ? "" : _inputs.front();
    for (const auto& in : _inputs)
    {
        double cachedValue = _owner->getCachedValue(in);
        if (cachedValue > value)
        {
            value = cachedValue;
            leaderName = in;
        }
    }

    // Update D-Bus debug interface with input value and leader
    if (!_inputs.empty())
    {
        _owner->updateThermalPowerDebugInterface(_id, leaderName, value, 0);
    }

    if (debugEnabled)
    {
        std::cerr << getID()
                  << " choose the maximum temperature value: " << value << "\n";
    }

    return value;
}

void StepwiseController::outputProc(double value)
{
    if (getStepwiseInfo().isCeiling)
    {
        _owner->addRPMCeiling(value);
    }
    else
    {
        _owner->addSetPoint(value, _id);
        if (debugEnabled)
        {
            std::cerr << getID() << " stepwise output pwm: " << value << "\n";
        }
    }
    // Update D-Bus debug interface with output value
    _owner->updateThermalPowerDebugInterface(_id, "", 0, value);
    return;
}

} // namespace pid_control
