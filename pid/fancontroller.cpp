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

#include <algorithm>
#include <iostream>

#include "fancontroller.hpp"
#include "util.hpp"
#include "zone.hpp"

std::unique_ptr<PIDController> FanController::CreateFanPid(
    ZoneInterface* owner,
    const std::string& id,
    std::vector<std::string>& inputs,
    ec::pidinfo initial)
{
    auto fan = std::make_unique<FanController>(id, inputs, owner);
    ec::pid_info_t* info = fan->get_pid_info();

    InitializePIDStruct(info, &initial);

    return fan;
}

float FanController::input_proc(void)
{
    float sum = 0;
    double value = 0;
    std::vector<int64_t> values;
    std::vector<int64_t>::iterator result;

    try
    {
        for (const auto& name : _inputs)
        {
            value = _owner->getCachedValue(name);
            /* If we have a fan we can't read, its value will be 0 for at least
             * some boards, while others... the fan will drop off dbus (if
             * that's how it's being read and in that case its value will never
             * be updated anymore, which is relatively harmless, except, when
             * something tries to read its value through IPMI, and can't, they
             * sort of have to guess -- all the other fans are reporting, why
             * not this one?  Maybe it's unable to be read, so it's "bad."
             */
            if (value > 0)
            {
                values.push_back(value);
                sum += value;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "exception on input_proc.\n";
        throw;
    }

    if (values.size() > 0)
    {
        /* When this is a configuration option in a later update, this code
         * will be updated.
         */
        //value = sum / _inputs.size();

        /* the fan PID algorithm was unstable with average, and seemed to work
         * better with minimum.  I had considered making this choice a variable
         * in the configuration, and I will.
         */
        result = std::min_element(values.begin(), values.end());
        value = *result;
    }

    return static_cast<float>(value);
}

float FanController::setpt_proc(void)
{
    float maxRPM = _owner->getMaxRPMRequest();

    // store for reference, and check if more or less.
    float prev = get_setpoint();

    if (maxRPM > prev)
    {
        setFanDirection(FanSpeedDirection::UP);
    }
    else if (prev > maxRPM)
    {
        setFanDirection(FanSpeedDirection::DOWN);
    }
    else
    {
        setFanDirection(FanSpeedDirection::NEUTRAL);
    }

    set_setpoint(maxRPM);

    return (maxRPM);
}

void FanController::output_proc(float value)
{
    float percent = value;

    /* If doing tuning logging, don't go into failsafe mode. */
#ifndef __TUNING_LOGGING__
    if (_owner->getFailSafeMode())
    {
        /* In case it's being set to 100% */
        if (percent < _owner->getFailSafePercent())
        {
            percent = _owner->getFailSafePercent();
        }
    }
#endif

    // value and kFanFailSafeDutyCycle are 10 for 10% so let's fix that.
    percent /= 100;

    // PidSensorMap for writing.
    for (auto& it : _inputs)
    {
        auto sensor = _owner->getSensor(it);
        sensor->write(static_cast<double>(percent));
    }

    return;
}

