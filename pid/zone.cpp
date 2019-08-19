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
#include "config.h"

#include "zone.hpp"

#include "conf.hpp"
#include "pid/controller.hpp"
#include "pid/ec/pid.hpp"
#include "pid/fancontroller.hpp"
#include "pid/stepwisecontroller.hpp"
#include "pid/thermalcontroller.hpp"
#include "pid/tuning.hpp"

#include <systemd/sd-journal.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

double PIDZone::getMaxRPMRequest(void) const
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
    /* If fail safe fans are more than the maximum number that project
     * defined(default = 1) or any temperature sensors in fail safe, Enter fail
     * safe mode. If MAX_FAN_REDUNDANCY == 0, process will not enter failsafe
     * mode beacause of fan failures.
     */
    if ((MAX_FAN_REDUNDANCY != 0) &&
        (_failSafeFans.size() >= MAX_FAN_REDUNDANCY))
    {
        return true;
    }
    // If any CPU or DIMM sensors are failed, enter fail safe mode.
    for (auto& it : _failSafeTemps)
    {
        if ((it.find("CPU") != std::string::npos) || (it.find("DIMM") != std::string::npos))
        {
            return true;
        }
    }

    return false;
}

int64_t PIDZone::getZoneID(void) const
{
    return _zoneId;
}

void PIDZone::addRPMSetPoint(double setpoint)
{
    _RPMSetPoints.push_back(setpoint);
}

void PIDZone::addRPMCeiling(double ceiling)
{
    _RPMCeilings.push_back(ceiling);
}

void PIDZone::clearRPMCeilings(void)
{
    _RPMCeilings.clear();
}

void PIDZone::clearRPMSetPoints(void)
{
    _RPMSetPoints.clear();
}

double PIDZone::getFailSafePercent(void) const
{
    return _failSafePercent;
}

double PIDZone::getMinThermalRPMSetpoint(void) const
{
    return _minThermalOutputSetPt;
}

uint64_t PIDZone::getCycleTimeBase(void) const
{
    return _cycleTimeBase;
}

uint64_t PIDZone::getCheckFanFailuresCycle(void) const
{
    return _checkFanFailuresCycle;
}

uint64_t PIDZone::getUpdateThermalsCycle(void) const
{
    return _updateThermalsCycle;
}

void PIDZone::setCheckFanFailuresFlag(bool value)
{
    _checkFanFailuresFlag = value;
}

bool PIDZone::getCheckFanFailuresFlag(void) const
{
    return _checkFanFailuresFlag;
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
    double max = 0;
    std::vector<double>::iterator result;

    if (_RPMSetPoints.size() > 0)
    {
        result = std::max_element(_RPMSetPoints.begin(), _RPMSetPoints.end());
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
    max = std::max(getMinThermalRPMSetpoint(), max);

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

    _maximumRPMSetPt = max;
    return;
}

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
    tstamp now = std::chrono::high_resolution_clock::now();
    if (loggingEnabled)
    {
        _log << std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();
        _log << "," << _maximumRPMSetPt;
    }

    for (const auto& f : _fanInputs)
    {
        auto sensor = _mgr.getSensor(f);
        ReadReturn r = sensor->read();
        _cachedValuesByName[f] = r.value;
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
            _log << "," << r.value;
        }

        // check if fan fail.
        if (sensor->getFailed())
        {
            sd_journal_print(LOG_INFO, "%s fan sensor getfailed", f.c_str());
            _failSafeFans.insert(f);
        }
        else if (timeout != 0 && duration >= period)
        {
            sd_journal_print(LOG_INFO, "%s fan sensor timeout, duration: %lld",
                             f.c_str(), duration);
            _failSafeFans.insert(f);
        }
        else
        {
            // Check if it's in there: remove it.
            auto kt = _failSafeFans.find(f);
            /* Avoid erasing failed fans from failSafeFans set.
             * Because checkFanFailures detect that this fan is failed.
             */
            if ((kt != _failSafeFans.end()) && (_isFanFailure[f] == false))
            {
                sd_journal_print(LOG_INFO, "erase %s fan sensor from failsafe",
                                 f.c_str());
                _failSafeFans.erase(kt);
            }
        }
    }

    if (loggingEnabled)
    {
        for (const auto& t : _thermalInputs)
        {
            _log << "," << _cachedValuesByName[t];
        }
    }

    return;
}

void PIDZone::checkFanFailures(void)
{
    std::map<std::string, double> fanSpeeds;
    double firstLargestFanTach = 0;
    double secondLargestFanTach = 0;
    double value = 0;
    double twoLargestAverage = 0;

    // Get the fan speeds
    for (const auto& name : _fanInputs)
    {
        value = _cachedValuesByName[name];
        if (value > 0)
        {
            fanSpeeds[name] = value;
        }
        // If the reading value is under 0 set the fan speed to 0
        else
        {
            fanSpeeds[name] = 0;
        }

        // Find the two largest fan speeds
        if (value > secondLargestFanTach)
        {
            if (value > firstLargestFanTach)
            {
                firstLargestFanTach = value;
            }
            else
            {
                secondLargestFanTach = value;
            }
        }
    }

    twoLargestAverage = (firstLargestFanTach + secondLargestFanTach) / 2;

    /* If a fan tachometer value is 25% below the twoLargestAverage
     * log a SEL to indicate a suspected failure on this fan
     */
    for (auto& it : fanSpeeds)
    {
        if (it.second < (twoLargestAverage * 0.75))
        {
            sd_journal_print(LOG_ERR, "%s is 25%% below the average",
                             it.first.c_str());
            _failSafeFans.insert(it.first);
            _isFanFailure[it.first] = true;
        }
        else
        {
            /* Another sides place fans in failSafeFans.
             * Do not erase them from set.
             */
            if (_isFanFailure[it.first] == true)
            {
                _failSafeFans.erase(it.first);
            }
            _isFanFailure[it.first] = false;
        }
    }

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
        if (debugModeEnabled)
        {
            sd_journal_print(LOG_INFO, "%s temperature sensor reading: %lg",
                             t.c_str(), r.value);
        }
        _cachedValuesByName[t] = r.value;
        tstamp then = r.updated;

        auto duration = duration_cast<std::chrono::seconds>(now - then).count();
        auto period = std::chrono::seconds(timeout).count();

        if (sensor->getFailed())
        {
            sd_journal_print(LOG_INFO, "%s temperature sensor getfailed",
                             t.c_str());
            _failSafeTemps.insert(t);
        }
        else if (timeout != 0 && duration >= period)
        {
            // std::cerr << "Entering fail safe mode.\n";
            sd_journal_print(LOG_INFO,
                             "%s temperature sensor timeout, duration: %lld",
                             t.c_str(), duration);
            _failSafeTemps.insert(t);
        }
        else if ((r.value == 0) && ((t.find("CPU") != std::string::npos) || (t.find("DIMM") != std::string::npos)))
        {
            auto method = _bus.new_method_call("xyz.openbmc_project.CPUSensor",
                                        ("/xyz/openbmc_project/sensors/temperature/" + t).c_str(),
                                        "org.freedesktop.DBus.Properties", "Get");
            method.append("xyz.openbmc_project.Sensor.Value", "ReadingState");

            auto reply = _bus.call(method);
            if (reply.is_method_error())
            {
                sd_journal_print(LOG_INFO, "Failed to get ReadingState from D-Bus: %s",
                    t.c_str());
                continue;
            }

            bool readingState;
            reply.read(readingState);

            // ReadingState is true means that this value is invalid.
            if (readingState == true)
            {
                // The interval of pid control update sensor values.
                sensorFailuresTimer[t] += (getUpdateThermalsCycle() * getCycleTimeBase());

                /**
                 *  If BMC can’t get the correct response from CPU or DIMM over 30 seconds.
                 *  Add these CPU and DIMM sensors to fail safe set.
                 *  When BMC can't get sensor value, it will try 10 times(about 10 seconds)
                 *  before it changes sensor state. So when PID control receives that
                 *  state has changed, it’s been 10 seconds already.
                 */
                if (sensorFailuresTimer[t] >= 20000)
                {
                    _failSafeTemps.insert(t);
                }
            }
            else
            {
                // Reset sensor failure time counter.
                sensorFailuresTimer[t] = 0;
            }
        }
        else
        {
            // Reset sensor failure time counter.
            sensorFailuresTimer[t] = 0;

            // Check if it's in there: remove it.
            auto kt = _failSafeTemps.find(t);
            if (kt != _failSafeTemps.end())
            {
                sd_journal_print(LOG_INFO,
                                 "erase %s temperature sensor from failsafe",
                                 t.c_str());
                _failSafeTemps.erase(kt);
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

        // Start all fans in fail-safe mode.
        _failSafeFans.insert(f);
    }

    for (const auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = 0;

        // Start all sensors in fail-safe mode.
        _failSafeTemps.insert(t);
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
