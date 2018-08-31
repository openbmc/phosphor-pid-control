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
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include <vector>
void PIDController::process(void)
{
    float input;
    float setpt;
    float output;

    // Get setpt value
    setpt = setpt_proc();

    // Get input value
    input = input_proc();

    // Calculate new output
    output = ec::pid(get_pid_info(), input, setpt);

    // Output new value
    output_proc(output);

    return;
}
