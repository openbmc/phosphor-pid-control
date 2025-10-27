// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "config.h"

#include "fancontroller.hpp"

#include "ec/pid.hpp"
#include "fan.hpp"
#include "pidcontroller.hpp"
#include "tuning.hpp"
#include "util.hpp"
#include "zone_interface.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace pid_control
{

std::unique_ptr<PIDController> FanController::createFanPid(
    ZoneInterface* owner, const std::string& id,
    const std::vector<std::string>& inputs, const ec::pidinfo& initial)
{
    if (inputs.size() == 0)
    {
        return nullptr;
    }
    auto fan = std::make_unique<FanController>(id, inputs, owner);
    ec::pid_info_t* info = fan->getPIDInfo();

    initializePIDStruct(info, initial);

    return fan;
}

double FanController::inputProc(void)
{
    double value = 0.0;
    std::vector<double> values;
    std::vector<double>::iterator result;

    try
    {
        for (const auto& name : _inputs)
        {
            // Read the unscaled value, to correctly recover the RPM
            value = _owner->getCachedValues(name).unscaled;

            /* If we have a fan we can't read, its value will be 0 for at least
             * some boards, while others... the fan will drop off dbus (if
             * that's how it's being read and in that case its value will never
             * be updated anymore, which is relatively harmless, except, when
             * something tries to read its value through IPMI, and can't, they
             * sort of have to guess -- all the other fans are reporting, why
             * not this one?  Maybe it's unable to be read, so it's "bad."
             */
            if (!(std::isfinite(value)))
            {
                continue;
            }
            if (value <= 0.0)
            {
                continue;
            }

            values.push_back(value);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "exception on inputProc.\n";
        throw;
    }

    /* Reset the value from the above loop. */
    value = 0.0;
    if (values.size() > 0)
    {
        /* the fan PID algorithm was unstable with average, and seemed to work
         * better with minimum.  I had considered making this choice a variable
         * in the configuration, and it's a nice-to-have..
         */
        result = std::min_element(values.begin(), values.end());
        value = *result;
    }

    return value;
}

double FanController::setptProc(void)
{
    double maxRPM = _owner->getMaxSetPointRequest();

    // store for reference, and check if more or less.
    double prev = getSetpoint();

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

    setSetpoint(maxRPM);

    return (maxRPM);
}

void FanController::outputProc(double value)
{
    double percent = value;

    /* If doing tuning, don't go into failsafe mode. */
    if (!tuningEnabled)
    {
        bool failsafeCurrState = _owner->getFailSafeMode();

        // Note when failsafe state transitions happen
        if (failsafePrevState != failsafeCurrState)
        {
            failsafePrevState = failsafeCurrState;
            failsafeTransition = true;
        }

        if (failsafeCurrState)
        {
            double failsafePercent = _owner->getFailSafePercent();

#ifdef STRICT_FAILSAFE_PWM
            // Unconditionally replace the computed PWM with the
            // failsafe PWM if STRICT_FAILSAFE_PWM is defined.
            percent = failsafePercent;
#else
            // Ensure PWM is never lower than the failsafe PWM.
            // The computed PWM is still allowed to rise higher than
            // failsafe PWM if STRICT_FAILSAFE_PWM is NOT defined.
            // This is the default behavior.
            if (percent < failsafePercent)
            {
                percent = failsafePercent;
            }
#endif
        }

        // Always print if debug enabled
        if (debugEnabled)
        {
            std::cerr << "Zone " << _owner->getZoneID() << " fans, "
                      << (failsafeCurrState ? "failsafe" : "normal")
                      << " mode, output pwm: " << percent << "\n";
        }
        else
        {
            // Only print once per transition when not debugging
            if (failsafeTransition)
            {
                failsafeTransition = false;
                std::cerr << "Zone " << _owner->getZoneID() << " fans, "
                          << (failsafeCurrState ? "entering failsafe"
                                                : "returning to normal")
                          << " mode, output pwm: " << percent << "\n";

                std::map<std::string, std::pair<std::string, double>>
                    failSensorList = _owner->getFailSafeSensors();
                for (const auto& it : failSensorList)
                {
                    std::cerr << "Fail sensor: " << it.first
                              << ", reason: " << it.second.first << "\n";
                }
            }
        }
    }
    else
    {
        if (debugEnabled)
        {
            std::cerr << "Zone " << _owner->getZoneID()
                      << " fans, tuning mode, bypassing failsafe, output pwm: "
                      << percent << "\n";
        }
    }

    // value and kFanFailSafeDutyCycle are 10 for 10% so let's fix that.
    percent /= 100.0;

    // PidSensorMap for writing.
    for (const auto& it : _inputs)
    {
        auto sensor = _owner->getSensor(it);
        auto redundantWrite = _owner->getRedundantWrite();
        int64_t rawWritten = -1;
        sensor->write(percent, redundantWrite, &rawWritten);

        // The outputCache will be used later,
        // to store a record of the PWM commanded,
        // so that this information can be included during logging.
        auto unscaledWritten = static_cast<double>(rawWritten);
        _owner->setOutputCache(sensor->getName(), {percent, unscaledWritten});
    }

    return;
}

FanController::~FanController()
{
#ifdef OFFLINE_FAILSAFE_PWM
    double percent = _owner->getFailSafePercent();
    if (debugEnabled)
    {
        std::cerr << "Zone " << _owner->getZoneID()
                  << " offline fans output pwm: " << percent << "\n";
    }

    // value and kFanFailSafeDutyCycle are 10 for 10% so let's fix that.
    percent /= 100.0;

    // PidSensorMap for writing.
    for (const auto& it : _inputs)
    {
        auto sensor = _owner->getSensor(it);
        auto redundantWrite = _owner->getRedundantWrite();
        int64_t rawWritten;
        sensor->write(percent, redundantWrite, &rawWritten);

        // The outputCache will be used later,
        // to store a record of the PWM commanded,
        // so that this information can be included during logging.
        auto unscaledWritten = static_cast<double>(rawWritten);
        _owner->setOutputCache(sensor->getName(), {percent, unscaledWritten});
    }
#endif
}

} // namespace pid_control
