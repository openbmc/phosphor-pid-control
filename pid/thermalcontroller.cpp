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

#include "errors/exception.hpp"
#include "tuning.hpp"
#include "util.hpp"
#include "zone.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace pid_control
{

ThermalType getThermalType(const std::string& typeString)
{
    /* Currently it only supports the two types. */
    return (typeString == "temp") ? ThermalType::absolute : ThermalType::margin;
}

bool isThermalType(const std::string& typeString)
{
    static const std::vector<std::string> thermalTypes = {"temp", "margin"};
    return std::count(thermalTypes.begin(), thermalTypes.end(), typeString);
}

std::unique_ptr<PIDController> ThermalController::createThermalPid(
    ZoneInterface* owner, const std::string& id,
    const std::vector<std::string>& inputs, double setpoint,
    const ec::pidinfo& initial, const ThermalType& type)
{
    // ThermalController requires at least 1 input
    if (inputs.empty())
    {
        throw ControllerBuildException("Thermal controller missing inputs");
        return nullptr;
    }

    auto thermal = std::make_unique<ThermalController>(id, inputs, type, owner);

    ec::pid_info_t* info = thermal->getPIDInfo();
    thermal->setSetpoint(setpoint);

    initializePIDStruct(info, initial);

    return thermal;
}

// bmc_host_sensor_value_double
double ThermalController::inputProc(void)
{
    double value;
    const double& (*compare)(const double&, const double&);
    if (type == ThermalType::margin)
    {
        value = std::numeric_limits<double>::max();
        compare = std::min<double>;
    }
    else
    {
        value = std::numeric_limits<double>::lowest();
        compare = std::max<double>;
    }

    bool acceptable = false;
    for (const auto& in : _inputs)
    {
        double cachedValue = _owner->getCachedValue(in);

        // Less than 0 is perfectly OK for temperature, but must not be NAN
        if (!(std::isfinite(cachedValue)))
        {
            continue;
        }

        value = compare(value, cachedValue);
        acceptable = true;
    }

    if (!acceptable)
    {
        // While not optimal, zero is better than garbage
        value = 0;
    }

    // The optional negateEnabled feature can make PID math easier
    if (type == ThermalType::margin)
    {
        if (negateEnabled)
        {
            // Negate the computed worst-margin value
            value = 0.0 - value;
        }
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

    // The optional negateEnabled feature can make PID math easier
    if (type == ThermalType::margin)
    {
        if (negateEnabled)
        {
            // Negate the setpoint value, to match the worst-margin
            setpoint = 0.0 - setpoint;
        }
    }

    return setpoint;
}

// bmc_set_pid_output
void ThermalController::outputProc(double value)
{
    _owner->addSetPoint(value);

    return;
}

} // namespace pid_control
