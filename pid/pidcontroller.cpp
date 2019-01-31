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

#include "pidcontroller.hpp"

#include "ec/pid.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <vector>

void PIDController::process(void)
{
    double input;
    double setpt;
    double output = lastOutput;

    // Get setpt value
    setpt = setptProc();

    // Get input value
    input = inputProc();

    auto info = getPIDInfo();

    // Calculate new output if hysteresis allows
    if (std::isnan(output))
    {
        output = ec::pid(info, input, setpt);
        lastInput = input;
    }
    else if ((input - lastInput) > info->positiveHysteresis)
    {
        output = ec::pid(info, input, setpt);
        lastInput = input;
    }
    else if ((lastInput - input) > info->negativeHysteresis)
    {
        output = ec::pid(info, input, setpt);
        lastInput = input;
    }

    lastOutput = output;

    // Output new value
    outputProc(output);

    return;
}
