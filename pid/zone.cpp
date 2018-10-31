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

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <libconfig.h++>
#include <memory>

using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

float PIDZone::getMaxRPMRequest(void) const
{
    return _maximumRPMSetPt;
}

bool PIDZone::getManualMode(void) const
{
    return _manualMode;
}

void PIDZone::setManualMode(bool mode)
{
    _manualMode = mode;
}

bool PIDZone::getFailSafeMode(void) const
{
    // If any keys are present at least one sensor is in fail safe mode.
    return !_failSafeSensors.empty();
}

int64_t PIDZone::getZoneID(void) const
{
    return _zoneId;
}

void PIDZone::addRPMSetPoint(float setpoint)
{
    _RPMSetPoints.push_back(setpoint);
}

void PIDZone::clearRPMSetPoints(void)
{
    _RPMSetPoints.clear();
}

float PIDZone::getFailSafePercent(void) const
{
    return _failSafePercent;
}

float PIDZone::getMinThermalRpmSetPt(void) const
{
    return _minThermalRpmSetPt;
}

void PIDZone::addFanPID(std::unique_ptr<Controller> pid)
{
    _fans.push_back(std::move(pid));
}

void PIDZone::addThermalPID(std::unique_ptr<Controller> pid)
{
    _thermals.push_back(std::move(pid));
}

double PIDZone::getCachedValue(const std::string& name)
{
    return _cachedValuesByName.at(name);
}

void PIDZone::addFanInput(const std::string& fan)
{
    _fanInputs.push_back(fan);
}

void PIDZone::addThermalInput(const std::string& therm)
{
    _thermalInputs.push_back(therm);
}

void PIDZone::determineMaxRPMRequest(void)
{
    float max = 0;
    std::vector<float>::iterator result;

    if (_RPMSetPoints.size() > 0)
    {
        result = std::max_element(_RPMSetPoints.begin(), _RPMSetPoints.end());
        max = *result;
    }

    /*
     * If the maximum RPM set-point output is below the minimum RPM
     * set-point, set it to the minimum.
     */
    max = std::max(getMinThermalRpmSetPt(), max);

#ifdef __TUNING_LOGGING__
    /*
     * We received no set-points from thermal sensors.
     * This is a case experienced during tuning where they only specify
     * fan sensors and one large fan PID for all the fans.
     */
    static constexpr auto setpointpath = "/etc/thermal.d/set-point";
    try
    {
        std::ifstream ifs;
        ifs.open(setpointpath);
        if (ifs.good())
        {
            int value;
            ifs >> value;

            /* expecting RPM set-point, not pwm% */
            max = static_cast<float>(value);
        }
    }
    catch (const std::exception& e)
    {
        /* This exception is uninteresting. */
        std::cerr << "Unable to read from '" << setpointpath << "'\n";
    }
#endif

    _maximumRPMSetPt = max;
    return;
}

#ifdef __TUNING_LOGGING__
void PIDZone::initializeLog(void)
{
    /* Print header for log file:
     * epoch_ms,setpt,fan1,fan2,fanN,sensor1,sensor2,sensorN,failsafe
     */

    _log << "epoch_ms,setpt";

    for (const auto& f : _fanInputs)
    {
        _log << "," << f;
    }
    for (const auto& t : _thermalInputs)
    {
        _log << "," << t;
    }
    _log << ",failsafe";
    _log << std::endl;

    return;
}

std::ofstream& PIDZone::getLogHandle(void)
{
    return _log;
}
#endif

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
void PIDZone::updateFanTelemetry(void)
{
    /* TODO(venture): Should I just make _log point to /dev/null when logging
     * is disabled?  I think it's a waste to try and log things even if the
     * data is just being dropped though.
     */
#ifdef __TUNING_LOGGING__
    tstamp now = std::chrono::high_resolution_clock::now();
    _log << std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch())
                .count();
    _log << "," << _maximumRPMSetPt;
#endif

    for (const auto& f : _fanInputs)
    {
        auto sensor = _mgr.getSensor(f);
        ReadReturn r = sensor->read();
        _cachedValuesByName[f] = r.value;

        /*
         * TODO(venture): We should check when these were last read.
         * However, these are the fans, so if I'm not getting updated values
         * for them... what should I do?
         */
#ifdef __TUNING_LOGGING__
        _log << "," << r.value;
#endif
    }

#ifdef __TUNING_LOGGING__
    for (const auto& t : _thermalInputs)
    {
        _log << "," << _cachedValuesByName[t];
    }
#endif

    return;
}

void PIDZone::updateSensors(void)
{
    using namespace std::chrono;
    /* margin and temp are stored as temp */
    tstamp now = high_resolution_clock::now();

    for (const auto& t : _thermalInputs)
    {
        auto sensor = _mgr.getSensor(t);
        ReadReturn r = sensor->read();
        int64_t timeout = sensor->getTimeout();

        _cachedValuesByName[t] = r.value;
        tstamp then = r.updated;

        if (sensor->getFailed())
        {
            _failSafeSensors.insert(t);
        }
        /* Only go into failsafe if the timeout is set for
         * the sensor.
         */
        else if (timeout > 0)
        {
            auto duration =
                duration_cast<std::chrono::seconds>(now - then).count();
            auto period = std::chrono::seconds(timeout).count();
            if (duration >= period)
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
    }

    return;
}

void PIDZone::initializeCache(void)
{
    for (const auto& f : _fanInputs)
    {
        _cachedValuesByName[f] = 0;
    }

    for (const auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = 0;

        // Start all sensors in fail-safe mode.
        _failSafeSensors.insert(t);
    }
}

void PIDZone::dumpCache(void)
{
    std::cerr << "Cache values now: \n";
    for (const auto& k : _cachedValuesByName)
    {
        std::cerr << k.first << ": " << k.second << "\n";
    }
}

void PIDZone::processFans(void)
{
    for (auto& p : _fans)
    {
        p->process();
    }
}

void PIDZone::processThermals(void)
{
    for (auto& p : _thermals)
    {
        p->process();
    }
}

Sensor* PIDZone::getSensor(const std::string& name)
{
    return _mgr.getSensor(name);
}

bool PIDZone::manual(bool value)
{
    std::cerr << "manual: " << value << std::endl;
    setManualMode(value);
    return ModeObject::manual(value);
}

bool PIDZone::failSafe() const
{
    return getFailSafeMode();
}
