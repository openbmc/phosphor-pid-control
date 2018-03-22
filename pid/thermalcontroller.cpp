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

#include "thermalcontroller.hpp"
#include "util.hpp"
#include "zone.hpp"

std::unique_ptr<PIDController> ThermalController::CreateThermalPid(
    std::shared_ptr<PIDZone> owner, const std::string& id,
    std::vector<std::string>& inputs, float setpoint, ec::pidinfo initial)
{
    auto thermal = std::make_unique<ThermalController>(id, inputs, owner);

    ec::pid_info_t* info = thermal->get_pid_info();
    thermal->set_setpoint(setpoint);

    InitializePIDStruct(info, &initial);

    return thermal;
}

// bmc_host_sensor_value_float
float ThermalController::input_proc(void)
{
    /*
     * This only supports one thermal input because it doesn't yet know how to
     * handle merging them, probably max?
     */
    double value = _owner->getCachedValue(_inputs.at(0));
    return static_cast<float>(value);
}

// bmc_get_setpt
float ThermalController::setpt_proc(void)
{
    float setpoint = get_setpoint();

    /* TODO(venture): Thermal setpoint invalid? */
#if 0
    if (-1 == setpoint)
    {
        return 0.0f;
    }
    else
    {
        return setpoint;
    }
#endif
    return setpoint;
}

// bmc_set_pid_output
void ThermalController::output_proc(float value)
{
    _owner->addRPMSetPoint(value);

    return;
}
