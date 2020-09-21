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

/* Configuration. */
#include "zone.hpp"

#include "conf.hpp"
#include "pid/controller.hpp"
#include "pid/ec/pid.hpp"
#include "pid/fancontroller.hpp"
#include "pid/stepwisecontroller.hpp"
#include "pid/thermalcontroller.hpp"
#include "pid/tuning.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace pid_control
{

using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

double DbusPidZone::getMaxSetPointRequest(void) const
{
    return _maximumSetPoint;
}

bool DbusPidZone::getManualMode(void) const
{
    return _manualMode;
}

void DbusPidZone::setManualMode(bool mode)
{
    _manualMode = mode;
}

bool DbusPidZone::getFailSafeMode(void) const
{
    // If any keys are present at least one sensor is in fail safe mode.
    return !_failSafeSensors.empty();
}

int64_t DbusPidZone::getZoneID(void) const
{
    return _zoneId;
}

void DbusPidZone::addSetPoint(double setpoint)
{
    _SetPoints.push_back(setpoint);
}

void DbusPidZone::addRPMCeiling(double ceiling)
{
    _RPMCeilings.push_back(ceiling);
}

void DbusPidZone::clearRPMCeilings(void)
{
    _RPMCeilings.clear();
}

void DbusPidZone::clearSetPoints(void)
{
    _SetPoints.clear();
}

double DbusPidZone::getFailSafePercent(void) const
{
    return _failSafePercent;
}

double DbusPidZone::getMinThermalSetpoint(void) const
{
    return _minThermalOutputSetPt;
}

void DbusPidZone::addFanPID(std::unique_ptr<Controller> pid)
{
    _fans.push_back(std::move(pid));
}

void DbusPidZone::addThermalPID(std::unique_ptr<Controller> pid)
{
    _thermals.push_back(std::move(pid));
}

double DbusPidZone::getCachedValue(const std::string& name)
{
    return _cachedValuesByName.at(name).first;
}

std::pair<double, double> DbusPidZone::getCachedValues(const std::string& name)
{
    return _cachedValuesByName.at(name);
}

void DbusPidZone::setOutputCache(const std::string& name,
                                 std::pair<double, double> values)
{
    _cachedFanOutputs[name] = values;
}

void DbusPidZone::addFanInput(const std::string& fan)
{
    _fanInputs.push_back(fan);
}

void DbusPidZone::addThermalInput(const std::string& therm)
{
    _thermalInputs.push_back(therm);
}

void DbusPidZone::determineMaxSetPointRequest(void)
{
    double max = 0;
    std::vector<double>::iterator result;

    if (_SetPoints.size() > 0)
    {
        result = std::max_element(_SetPoints.begin(), _SetPoints.end());
        max = *result;
    }

    if (_RPMCeilings.size() > 0)
    {
        result = std::min_element(_RPMCeilings.begin(), _RPMCeilings.end());
        max = std::min(max, *result);
    }

    /*
     * If the maximum RPM setpoint output is below the minimum RPM
     * setpoint, set it to the minimum.
     */
    max = std::max(getMinThermalSetpoint(), max);

    if (tuningEnabled)
    {
        /*
         * We received no setpoints from thermal sensors.
         * This is a case experienced during tuning where they only specify
         * fan sensors and one large fan PID for all the fans.
         */
        static constexpr auto setpointpath = "/etc/thermal.d/setpoint";
        try
        {
            std::ifstream ifs;
            ifs.open(setpointpath);
            if (ifs.good())
            {
                int value;
                ifs >> value;

                /* expecting RPM setpoint, not pwm% */
                max = static_cast<double>(value);
            }
        }
        catch (const std::exception& e)
        {
            /* This exception is uninteresting. */
            std::cerr << "Unable to read from '" << setpointpath << "'\n";
        }
    }

    _maximumSetPoint = max;
    return;
}

void DbusPidZone::initializeLog(void)
{
    /* Print header for log file:
     * epoch_ms,setpt,fan1,fan1_raw,fan1_pwm,fan1_pwm_raw,fan2,fan2_raw,fan2_pwm,fan2_pwm_raw,fanN,fanN_raw,fanN_pwm,fanN_pwm_raw,sensor1,sensor1_raw,sensor2,sensor2_raw,sensorN,sensorN_raw,failsafe
     */

    _log << "epoch_ms,setpt";

    for (const auto& f : _fanInputs)
    {
        _log << "," << f << "," << f << "_raw";
        _log << "," << f << "_pwm," << f << "_pwm_raw";
    }
    for (const auto& t : _thermalInputs)
    {
        _log << "," << t << "," << t << "_raw";
    }

    _log << ",failsafe";
    _log << std::endl;
}

void DbusPidZone::writeLog(const std::string& value)
{
    _log << value;
}

/*
 * TODO(venture) This is effectively updating the cache and should check if the
 * values they're using to update it are new or old, or whatnot.  For instance,
 * if we haven't heard from the host in X time we need to detect this failure.
 *
 * I haven't decided if the Sensor should have a lastUpdated method or whether
 * that should be for the ReadInterface or etc...
 */

/**
 * We want the PID loop to run with values cached, so this will get all the
 * fan tachs for the loop.
 */
void DbusPidZone::updateFanTelemetry(void)
{
    /* TODO(venture): Should I just make _log point to /dev/null when logging
     * is disabled?  I think it's a waste to try and log things even if the
     * data is just being dropped though.
     */
    tstamp now = std::chrono::high_resolution_clock::now();
    if (loggingEnabled)
    {
        _log << std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();
        _log << "," << _maximumSetPoint;
    }

    for (const auto& f : _fanInputs)
    {
        auto sensor = _mgr.getSensor(f);
        ReadReturn r = sensor->read();
        _cachedValuesByName[f] = std::make_pair(r.value, r.unscaled);
        int64_t timeout = sensor->getTimeout();
        tstamp then = r.updated;

        auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(now - then)
                .count();
        auto period = std::chrono::seconds(timeout).count();
        /*
         * TODO(venture): We should check when these were last read.
         * However, these are the fans, so if I'm not getting updated values
         * for them... what should I do?
         */
        if (loggingEnabled)
        {
            const auto& v = getCachedValues(f);
            _log << "," << v.first << "," << v.second;
            const auto& p = _cachedFanOutputs[f];
            _log << "," << p.first << "," << p.second;
        }

        // check if fan fail.
        if (sensor->getFailed())
        {
            _failSafeSensors.insert(f);
        }
        else if (timeout != 0 && duration >= period)
        {
            _failSafeSensors.insert(f);
        }
        else
        {
            // Check if it's in there: remove it.
            auto kt = _failSafeSensors.find(f);
            if (kt != _failSafeSensors.end())
            {
                _failSafeSensors.erase(kt);
            }
        }
    }

    if (loggingEnabled)
    {
        for (const auto& t : _thermalInputs)
        {
            const auto& v = getCachedValues(t);
            _log << "," << v.first << "," << v.second;
        }
    }

    return;
}

void DbusPidZone::updateSensors(void)
{
    using namespace std::chrono;
    /* margin and temp are stored as temp */
    tstamp now = high_resolution_clock::now();

    for (const auto& t : _thermalInputs)
    {
        auto sensor = _mgr.getSensor(t);
        ReadReturn r = sensor->read();
        int64_t timeout = sensor->getTimeout();

        _cachedValuesByName[t] = std::make_pair(r.value, r.unscaled);
        tstamp then = r.updated;

        auto duration = duration_cast<std::chrono::seconds>(now - then).count();
        auto period = std::chrono::seconds(timeout).count();

        if (sensor->getFailed())
        {
            _failSafeSensors.insert(t);
        }
        else if (timeout != 0 && duration >= period)
        {
            // std::cerr << "Entering fail safe mode.\n";
            _failSafeSensors.insert(t);
        }
        else
        {
            // Check if it's in there: remove it.
            auto kt = _failSafeSensors.find(t);
            if (kt != _failSafeSensors.end())
            {
                _failSafeSensors.erase(kt);
            }
        }
    }

    return;
}

void DbusPidZone::initializeCache(void)
{
    for (const auto& f : _fanInputs)
    {
        _cachedValuesByName[f] = std::make_pair(0, 0);
        _cachedFanOutputs[f] = std::make_pair(0, 0);

        // Start all fans in fail-safe mode.
        _failSafeSensors.insert(f);
    }

    for (const auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = std::make_pair(0, 0);

        // Start all sensors in fail-safe mode.
        _failSafeSensors.insert(t);
    }
}

void DbusPidZone::dumpCache(void)
{
    std::cerr << "Cache values now: \n";
    for (const auto& [name, value] : _cachedValuesByName)
    {
        std::cerr << name << ": " << value.first << " " << value.second << "\n";
    }

    std::cerr << "Fan outputs now: \n";
    for (const auto& [name, value] : _cachedFanOutputs)
    {
        std::cerr << name << ": " << value.first << " " << value.second << "\n";
    }
}

void DbusPidZone::processFans(void)
{
    for (auto& p : _fans)
    {
        p->process();
    }
}

void DbusPidZone::processThermals(void)
{
    for (auto& p : _thermals)
    {
        p->process();
    }
}

Sensor* DbusPidZone::getSensor(const std::string& name)
{
    return _mgr.getSensor(name);
}

bool DbusPidZone::manual(bool value)
{
    std::cerr << "manual: " << value << std::endl;
    setManualMode(value);
    return ModeObject::manual(value);
}

bool DbusPidZone::failSafe() const
{
    return getFailSafeMode();
}

} // namespace pid_control
