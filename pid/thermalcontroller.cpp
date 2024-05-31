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
    if (typeString == "margin")
    {
        return ThermalType::margin;
    }
    if ((typeString == "temp") || (typeString == "power"))
    {
        return ThermalType::absolute;
    }
    if (typeString == "powersum")
    {
        return ThermalType::summation;
    }

    throw ControllerBuildException("Unrecognized PID Type/Class string");
}

bool isThermalType(const std::string& typeString)
{
    static const std::vector<std::string> thermalTypes = {"temp", "margin",
                                                          "power", "powersum"};
    return std::count(thermalTypes.begin(), thermalTypes.end(), typeString);
}

std::unique_ptr<PIDController> ThermalController::createThermalPid(
    ZoneInterface* owner, const std::string& id,
    const std::vector<pid_control::conf::SensorInput>& inputs, double setpoint,
    const ec::pidinfo& initial, const ThermalType& type)
{
    // ThermalController requires at least 1 input
    if (inputs.empty())
    {
        owner = nullptr;
        delete owner;
        throw ControllerBuildException("Thermal controller missing inputs");
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
    bool doSummation = false;

    if (type == ThermalType::margin)
    {
        value = std::numeric_limits<double>::max();
        compare = std::min<double>;
    }
    else if (type == ThermalType::absolute)
    {
        value = std::numeric_limits<double>::lowest();
        compare = std::max<double>;
    }
    else if (type == ThermalType::summation)
    {
        doSummation = true;
        value = 0.0;
    }
    else
    {
        throw ControllerBuildException("Unrecognized ThermalType");
    }

    std::string leaderName = _inputs.begin()->name;

    bool acceptable = false;
    for (const auto& in : _inputs)
    {
        double cachedValue = _owner->getCachedValue(in.name);

        // Less than 0 is perfectly OK for temperature, but must not be NAN
        if (!(std::isfinite(cachedValue)))
        {
            continue;
        }

        // Perform TempToMargin conversion before further processing
        if (type == ThermalType::margin)
        {
            if (in.convertTempToMargin)
            {
                if (!(std::isfinite(in.convertMarginZero)))
                {
                    throw ControllerBuildException("Unrecognized TempToMargin");
                }

                double marginValue = in.convertMarginZero - cachedValue;

                if (debugEnabled)
                {
                    std::cerr << "Converting temp to margin: temp "
                              << cachedValue << ", Tjmax "
                              << in.convertMarginZero << ", margin "
                              << marginValue << "\n";
                }

                cachedValue = marginValue;
            }
        }

        double oldValue = value;

        if (doSummation)
        {
            value += cachedValue;
        }
        else
        {
            value = compare(value, cachedValue);
        }

        if (oldValue != value)
        {
            leaderName = in.name;
            _owner->updateThermalPowerDebugInterface(_id, leaderName, value, 0);
        }

        acceptable = true;
    }

    if (!acceptable)
    {
        // If none of the inputs were acceptable, use the setpoint as
        // the input value. This will continue to run the PID loop, but
        // make it a no-op, as the error will be zero. This provides safe
        // behavior until the inputs become acceptable.
        value = setptProc();
    }

    if (debugEnabled)
    {
        std::cerr << getID() << " choose the temperature value: " << value
                  << " " << leaderName << "\n";
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
    _owner->addSetPoint(value, _id);
    _owner->updateThermalPowerDebugInterface(_id, "", 0, value);

    if (debugEnabled)
    {
        std::cerr << getID() << " pid output pwm: " << value << "\n";
    }

    return;
}

} // namespace pid_control
