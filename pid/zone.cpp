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
#include <sstream>
#include <string>

using tstamp = std::chrono::high_resolution_clock::time_point;
using namespace std::literals::chrono_literals;

// Enforces minimum duration between events
// Rreturns true if event should be allowed, false if disallowed
bool allowThrottle(const tstamp& now, const std::chrono::seconds& pace)
{
    static tstamp then;
    static bool first = true;

    if (first)
    {
        // Special case initialization
        then = now;
        first = false;

        // Initialization, always allow
        return true;
    }

    auto elapsed = now - then;
    if (elapsed < pace)
    {
        // Too soon since last time, disallow
        return false;
    }

    // It has been long enough, allow
    then = now;
    return true;
}

namespace pid_control
{

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

    // If returning to automatic mode, need to restore PWM from PID loop
    if (!mode)
    {
        _redundantWrite = true;
    }
}

bool DbusPidZone::getFailSafeMode(void) const
{
    // If any keys are present at least one sensor is in fail safe mode.
    return !_failSafeSensors.empty();
}

std::map<std::string, std::pair<std::string, double>>
    DbusPidZone::getFailSafeSensors(void) const
{
    return _failSafeSensors;
}

void DbusPidZone::markSensorMissing(const std::string& name,
                                    const std::string& failReason)
{
    if (_missingAcceptable.find(name) != _missingAcceptable.end())
    {
        // Disallow sensors in MissingIsAcceptable list from causing failsafe
        return;
    }

    if (_sensorFailSafePercent[name] == 0)
    {
        _failSafeSensors[name] = std::pair(failReason, _zoneFailSafePercent);
    }
    else
    {
        _failSafeSensors[name] =
            std::pair(failReason, _sensorFailSafePercent[name]);
    }

    if (debugEnabled)
    {
        std::cerr << "Sensor " << name << " marked missing\n";
    }
}

int64_t DbusPidZone::getZoneID(void) const
{
    return _zoneId;
}

void DbusPidZone::addSetPoint(double setPoint, const std::string& name)
{
    /* exclude disabled pidloop from _maximumSetPoint calculation*/
    if (!isPidProcessEnabled(name))
    {
        return;
    }

    auto profileName = name;
    if (getAccSetPoint())
    {
        /*
         * If the name of controller is Linear_Temp_CPU0.
         * The profile name will be Temp_CPU0.
         */
        profileName = name.substr(name.find("_") + 1);
        _SetPoints[profileName] += setPoint;
    }
    else
    {
        if (_SetPoints[profileName] < setPoint)
        {
            _SetPoints[profileName] = setPoint;
        }
    }

    /*
     * if there are multiple thermal controllers with the same
     * value, pick the first one in the iterator
     */
    if (_maximumSetPoint < _SetPoints[profileName])
    {
        _maximumSetPoint = _SetPoints[profileName];
        _maximumSetPointName = profileName;
    }
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
    _maximumSetPoint = 0;
    _maximumSetPointName.clear();
}

double DbusPidZone::getFailSafePercent(void)
{
    std::map<std::string, std::pair<std::string, double>>::iterator maxData =
        std::max_element(
            _failSafeSensors.begin(), _failSafeSensors.end(),
            [](const std::pair<std::string, std::pair<std::string, double>>
                   firstData,
               const std::pair<std::string, std::pair<std::string, double>>
                   secondData) {
                return firstData.second.second < secondData.second.second;
            });

    // In dbus/dbusconfiguration.cpp, the default sensor failsafepercent is 0 if
    // there is no setting in json.
    // Therfore, if the max failsafe duty in _failSafeSensors is 0, set final
    // failsafe duty to _zoneFailSafePercent.
    if ((*maxData).second.second == 0)
    {
        return _zoneFailSafePercent;
    }
    else
    {
        return (*maxData).second.second;
    }
}

double DbusPidZone::getMinThermalSetPoint(void) const
{
    return _minThermalOutputSetPt;
}

uint64_t DbusPidZone::getCycleIntervalTime(void) const
{
    return _cycleTime.cycleIntervalTimeMS;
}

uint64_t DbusPidZone::getUpdateThermalsCycle(void) const
{
    return _cycleTime.updateThermalsTimeMS;
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
    return _cachedValuesByName.at(name).scaled;
}

ValueCacheEntry DbusPidZone::getCachedValues(const std::string& name)
{
    return _cachedValuesByName.at(name);
}

void DbusPidZone::setOutputCache(std::string_view name,
                                 const ValueCacheEntry& values)
{
    _cachedFanOutputs[std::string{name}] = values;
}

void DbusPidZone::addFanInput(const std::string& fan, bool missingAcceptable)
{
    _fanInputs.push_back(fan);

    if (missingAcceptable)
    {
        _missingAcceptable.emplace(fan);
    }
}

void DbusPidZone::addThermalInput(const std::string& therm,
                                  bool missingAcceptable)
{
    /*
     * One sensor may have stepwise and PID at the same time.
     * Searching the sensor name before inserting it to avoid duplicated sensor
     * names.
     */
    if (std::find(_thermalInputs.begin(), _thermalInputs.end(), therm) ==
        _thermalInputs.end())
    {
        _thermalInputs.push_back(therm);
    }

    if (missingAcceptable)
    {
        _missingAcceptable.emplace(therm);
    }
}

// Updates desired RPM setpoint from optional text file
// Returns true if rpmValue updated, false if left unchanged
static bool fileParseRpm(const std::string& fileName, double& rpmValue)
{
    static constexpr std::chrono::seconds throttlePace{3};

    std::string errText;

    try
    {
        std::ifstream ifs;
        ifs.open(fileName);
        if (ifs)
        {
            int value;
            ifs >> value;

            if (value <= 0)
            {
                errText = "File content could not be parsed to a number";
            }
            else if (value <= 100)
            {
                errText = "File must contain RPM value, not PWM value";
            }
            else
            {
                rpmValue = static_cast<double>(value);
                return true;
            }
        }
    }
    catch (const std::exception& e)
    {
        errText = "Exception: ";
        errText += e.what();
    }

    // The file is optional, intentionally not an error if file not found
    if (!(errText.empty()))
    {
        tstamp now = std::chrono::high_resolution_clock::now();
        if (allowThrottle(now, throttlePace))
        {
            std::cerr << "Unable to read from '" << fileName << "': " << errText
                      << "\n";
        }
    }

    return false;
}

void DbusPidZone::determineMaxSetPointRequest(void)
{
    std::vector<double>::iterator result;
    double minThermalThreshold = getMinThermalSetPoint();

    if (_RPMCeilings.size() > 0)
    {
        result = std::min_element(_RPMCeilings.begin(), _RPMCeilings.end());
        // if Max set point is larger than the lowest ceiling, reset to lowest
        // ceiling.
        if (*result < _maximumSetPoint)
        {
            _maximumSetPoint = *result;
            // When using lowest ceiling, controller name is ceiling.
            _maximumSetPointName = "Ceiling";
        }
    }

    /*
     * Combine the maximum SetPoint Name if the controllers have same profile
     * name. e.g., PID_BB_INLET_TEMP_C + Stepwise_BB_INLET_TEMP_C.
     */
    if (getAccSetPoint())
    {
        auto profileName = _maximumSetPointName;
        _maximumSetPointName = "";

        for (auto& p : _thermals)
        {
            auto controllerID = p->getID();
            auto found = controllerID.find(profileName);
            if (found != std::string::npos)
            {
                if (_maximumSetPointName.empty())
                {
                    _maximumSetPointName = controllerID;
                }
                else
                {
                    _maximumSetPointName += " + " + controllerID;
                }
            }
        }
    }

    /*
     * If the maximum RPM setpoint output is below the minimum RPM
     * setpoint, set it to the minimum.
     */
    if (minThermalThreshold >= _maximumSetPoint)
    {
        _maximumSetPoint = minThermalThreshold;
        _maximumSetPointName = "Minimum";
    }
    else if (_maximumSetPointName.compare(_maximumSetPointNamePrev))
    {
        std::cerr << "PID Zone " << _zoneId << " max SetPoint "
                  << _maximumSetPoint << " requested by "
                  << _maximumSetPointName;
        for (const auto& sensor : _failSafeSensors)
        {
            if (sensor.first.find("Fan") == std::string::npos)
            {
                std::cerr << " " << sensor.first;
            }
        }
        std::cerr << "\n";
        _maximumSetPointNamePrev.assign(_maximumSetPointName);
    }
    if (tuningEnabled)
    {
        /*
         * We received no setpoints from thermal sensors.
         * This is a case experienced during tuning where they only specify
         * fan sensors and one large fan PID for all the fans.
         */
        static constexpr auto setpointpath = "/etc/thermal.d/setpoint";

        fileParseRpm(setpointpath, _maximumSetPoint);

        // Allow per-zone setpoint files to override overall setpoint file
        std::ostringstream zoneSuffix;
        zoneSuffix << ".zone" << _zoneId;
        std::string zoneSetpointPath = setpointpath + zoneSuffix.str();

        fileParseRpm(zoneSetpointPath, _maximumSetPoint);
    }
    return;
}

void DbusPidZone::initializeLog(void)
{
    /* Print header for log file:
     * epoch_ms,setpt,fan1,fan1_raw,fan1_pwm,fan1_pwm_raw,fan2,fan2_raw,fan2_pwm,fan2_pwm_raw,fanN,fanN_raw,fanN_pwm,fanN_pwm_raw,sensor1,sensor1_raw,sensor2,sensor2_raw,sensorN,sensorN_raw,failsafe
     */

    _log << "epoch_ms,setpt,requester";

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
    const auto now = std::chrono::high_resolution_clock::now();
    if (loggingEnabled)
    {
        _log << std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();
        _log << "," << _maximumSetPoint;
        _log << "," << _maximumSetPointName;
    }

    processSensorInputs</* fanSensorLogging */ true>(_fanInputs, now);

    if (loggingEnabled)
    {
        for (const auto& t : _thermalInputs)
        {
            const auto& v = _cachedValuesByName[t];
            _log << "," << v.scaled << "," << v.unscaled;
        }
    }

    return;
}

void DbusPidZone::updateSensors(void)
{
    processSensorInputs</* fanSensorLogging */ false>(
        _thermalInputs, std::chrono::high_resolution_clock::now());

    return;
}

void DbusPidZone::initializeCache(void)
{
    auto nan = std::numeric_limits<double>::quiet_NaN();

    for (const auto& f : _fanInputs)
    {
        _cachedValuesByName[f] = {nan, nan};
        _cachedFanOutputs[f] = {nan, nan};

        // Start all fans in fail-safe mode.
        markSensorMissing(f, "");
    }

    for (const auto& t : _thermalInputs)
    {
        _cachedValuesByName[t] = {nan, nan};

        // Start all sensors in fail-safe mode.
        markSensorMissing(t, "");
    }
}

void DbusPidZone::dumpCache(void)
{
    std::cerr << "Cache values now: \n";
    for (const auto& [name, value] : _cachedValuesByName)
    {
        std::cerr << name << ": " << value.scaled << " " << value.unscaled
                  << "\n";
    }

    std::cerr << "Fan outputs now: \n";
    for (const auto& [name, value] : _cachedFanOutputs)
    {
        std::cerr << name << ": " << value.scaled << " " << value.unscaled
                  << "\n";
    }
}

void DbusPidZone::processFans(void)
{
    for (auto& p : _fans)
    {
        p->process();
    }

    if (_redundantWrite)
    {
        // This is only needed once
        _redundantWrite = false;
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

bool DbusPidZone::getRedundantWrite(void) const
{
    return _redundantWrite;
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

void DbusPidZone::addPidControlProcess(std::string name, std::string type,
                                       double setpoint, sdbusplus::bus_t& bus,
                                       std::string objPath, bool defer)
{
    _pidsControlProcess[name] = std::make_unique<ProcessObject>(
        bus, objPath.c_str(),
        defer ? ProcessObject::action::defer_emit
              : ProcessObject::action::emit_object_added);
    // Default enable setting = true
    _pidsControlProcess[name]->enabled(true);
    _pidsControlProcess[name]->setpoint(setpoint);

    if (type == "temp")
    {
        _pidsControlProcess[name]->classType("Temperature");
    }
    else if (type == "margin")
    {
        _pidsControlProcess[name]->classType("Margin");
    }
    else if (type == "power")
    {
        _pidsControlProcess[name]->classType("Power");
    }
    else if (type == "powersum")
    {
        _pidsControlProcess[name]->classType("PowerSum");
    }
}

bool DbusPidZone::isPidProcessEnabled(std::string name)
{
    return _pidsControlProcess[name]->enabled();
}

void DbusPidZone::addPidFailSafePercent(std::vector<std::string> inputs,
                                        double percent)
{
    for (const auto& sensorName : inputs)
    {
        if (_sensorFailSafePercent.find(sensorName) !=
            _sensorFailSafePercent.end())
        {
            _sensorFailSafePercent[sensorName] =
                std::max(_sensorFailSafePercent[sensorName], percent);
            if (debugEnabled)
            {
                std::cerr << "Sensor " << sensorName
                          << " failsafe percent updated to "
                          << _sensorFailSafePercent[sensorName] << "\n";
            }
        }
        else
        {
            _sensorFailSafePercent[sensorName] = percent;
            if (debugEnabled)
            {
                std::cerr << "Sensor " << sensorName
                          << " failsafe percent set to " << percent << "\n";
            }
        }
    }
}

std::string DbusPidZone::leader() const
{
    return _maximumSetPointName;
}

void DbusPidZone::updateThermalPowerDebugInterface(
    std::string pidName, std::string leader, double input, double output)
{
    if (leader.empty())
    {
        _pidsControlProcess[pidName]->output(output);
    }
    else
    {
        _pidsControlProcess[pidName]->leader(leader);
        _pidsControlProcess[pidName]->input(input);
    }
}

bool DbusPidZone::getAccSetPoint(void) const
{
    return _accumulateSetPoint;
}

} // namespace pid_control
