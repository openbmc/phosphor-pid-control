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

std::unique_ptr<PIDController> ThermalController::createThermalPid(
    ZoneInterface* owner, const std::string& id,
    const std::vector<std::string>& inputs, double setpoint,
    const ec::pidinfo& initial)
{
    // ThermalController requires at least 1 input
    if (inputs.empty())
    {
        throw std::runtime_error("Thermal controller missing inputs");
        return nullptr;
    }

    auto thermal = std::make_unique<ThermalController>(id, inputs, owner);

    ec::pid_info_t* info = thermal->getPIDInfo();
    thermal->setSetpoint(setpoint);

    initializePIDStruct(info, initial);

    return thermal;
}

// bmc_host_sensor_value_double
double ThermalController::inputProc(void)
{
    double value = std::numeric_limits<double>::lowest();
    for (const auto& in : _inputs)
    {
        value = std::max(value, _owner->getCachedValue(in));
    }
    return value;
}

// bmc_get_setpt
double ThermalController::setptProc(void)
{
    double setpoint = getSetpoint();

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
void ThermalController::outputProc(double value)
{
    _owner->addRPMSetPoint(value);

    return;
}
