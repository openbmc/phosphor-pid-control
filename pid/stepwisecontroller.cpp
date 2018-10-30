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

#include "ec/stepwise.hpp"
#include "util.hpp"
#include "zone.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <vector>

void StepwiseController::process(void)
{
    // Get input value
    float input = inputProc();

    ec::StepwiseInfo info = get_stepwise_info();

    float output = lastOutput;

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
    // StepwiseController currently only supports precisely one input.
    if (inputs.size() != 1)
    {
        return nullptr;
    }

    auto thermal = std::make_unique<StepwiseController>(id, inputs, owner);

    ec::StepwiseInfo& info = thermal->get_stepwise_info();

    info = initial;

    return thermal;
}

float StepwiseController::inputProc(void)
{
    double value = _owner->getCachedValue(_inputs.at(0));
    return static_cast<float>(value);
}

void StepwiseController::outputProc(float value)
{
    _owner->addRPMSetPoint(value);

    return;
}
