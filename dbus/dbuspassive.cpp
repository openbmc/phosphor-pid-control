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
#include "config.h"

#include "dbuspassive.hpp"

#include "conf.hpp"
#include "dbushelper_interface.hpp"
#include "dbuspassiveredundancy.hpp"
#include "dbusutil.hpp"
#include "failsafeloggers/failsafe_logger_utility.hpp"
#include "interfaces.hpp"
#include "util.hpp"

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <variant>

#include "failsafeloggers/failsafe_logger.cpp"

namespace pid_control
{

std::unique_ptr<ReadInterface> DbusPassive::createDbusPassive(
    sdbusplus::bus_t& bus, const std::string& type, const std::string& id,
    std::unique_ptr<DbusHelperInterface> helper, const conf::SensorConfig* info,
    const std::shared_ptr<DbusPassiveRedundancy>& redundancy)
{
    if (helper == nullptr)
    {
        return nullptr;
    }
    if (!validType(type))
    {
        return nullptr;
    }

    /* Need to get the scale and initial value */
    /* service == busname */
    std::string path;
    if (info->readPath.empty())
    {
        path = getSensorPath(type, id);
    }
    else
    {
        path = info->readPath;
    }

    SensorProperties settings;
    bool failed;
    bool objectMissing = false;
    std::string service;

    try
    {
        service = helper->getService(sensorintf, path);
    }
    catch (const std::exception& e)
    {
#ifndef HANDLE_MISSING_OBJECT_PATHS
        return nullptr;
#else
        // CASE1: The sensor is not on DBus, but as it is not in the
        // MissingIsAcceptable list, the sensor should be built with a failed
        // state to send the zone to failsafe mode. Everything will recover if
        // all important sensors are back to DBus. swampd will be informed
        // through InterfacesAdded signals and the sensors will be built again.

        // CASE2: The sensor is on D-Bus (getService succeeds) but getProperties
        // fails (e.g., D-Bus error or property fetch failure). In this case,
        // handle-missing-object-paths does not apply. The sensor build fails,
        // and the control loop will keep restarting until getProperties
        // succeeds.

        // Only CASE1 may send the zone to failsafe mode if the sensor is not
        // in MissingIsAcceptable. CASE2 results in continuous restart until
        // recovery.

        failed = true;
        objectMissing = true;
        settings.value = std::numeric_limits<double>::quiet_NaN();
        settings.unit = getSensorUnit(type);
        settings.available = false;
        settings.unavailableAsFailed = true;
        if (info->ignoreDbusMinMax)
        {
            settings.min = 0;
            settings.max = 0;
        }
        std::cerr << "DbusPassive: Sensor " << path
                  << " is missing from D-Bus, build this sensor as failed\n";
        return std::make_unique<DbusPassive>(
            bus, type, id, std::move(helper), settings, failed, objectMissing,
            path, redundancy);
#endif
    }

    try
    {
        helper->getProperties(service, path, &settings);
        failed = helper->thresholdsAsserted(service, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    /* if these values are zero, they're ignored. */
    if (info->ignoreDbusMinMax)
    {
        settings.min = 0;
        settings.max = 0;
    }

    settings.unavailableAsFailed = info->unavailableAsFailed;

    return std::make_unique<DbusPassive>(
        bus, type, id, std::move(helper), settings, failed, objectMissing, path,
        redundancy);
}

DbusPassive::DbusPassive(
    sdbusplus::bus_t& bus, const std::string& type, const std::string& id,
    std::unique_ptr<DbusHelperInterface> helper,
    const SensorProperties& settings, bool failed, bool objectMissing,
    const std::string& path,
    const std::shared_ptr<DbusPassiveRedundancy>& redundancy) :
    ReadInterface(), _signal(bus, getMatch(path), dbusHandleSignal, this),
    _id(id), _helper(std::move(helper)), _failed(failed),
    _objectMissing(objectMissing), path(path), redundancy(redundancy)

{
    _scale = settings.scale;
    _min = settings.min * std::pow(10.0, _scale);
    _max = settings.max * std::pow(10.0, _scale);
    _available = settings.available;
    _unavailableAsFailed = settings.unavailableAsFailed;

    // Cache this type knowledge, to avoid repeated string comparison
    _typeMargin = (type == "margin");
    _typeFan = (type == "fan");

    // Force value to be stored, otherwise member would be uninitialized
    updateValue(settings.value, true);
}

ReadReturn DbusPassive::read(void)
{
    std::lock_guard<std::mutex> guard(_lock);

    ReadReturn r = {_value, _updated, _unscaled};

    return r;
}

void DbusPassive::setValue(double value, double unscaled)
{
    std::lock_guard<std::mutex> guard(_lock);

    _value = value;
    _unscaled = unscaled;
    _updated = std::chrono::high_resolution_clock::now();
}

void DbusPassive::setValue(double value)
{
    // First param is scaled, second param is unscaled, assume same here
    setValue(value, value);
}

bool DbusPassive::getFailed(void) const
{
    if (redundancy)
    {
        const std::set<std::string>& failures = redundancy->getFailed();
        if (failures.find(path) != failures.end())
        {
            outputFailsafeLogWithSensor(_id, true, _id,
                                        "The sensor path is marked redundant.");
            return true;
        }
    }

    /*
     * If handle-missing-object-paths is enabled, and the expected D-Bus object
     * path is not exported, this sensor is created to represent that condition.
     * Indicate this sensor has failed so the zone enters failSafe mode.
     */
    if (_objectMissing)
    {
        outputFailsafeLogWithSensor(_id, true, _id,
                                    "The sensor D-Bus object is missing.");
        return true;
    }

    /*
     * Unavailable thermal sensors, who are not present or
     * power-state-not-matching, should not trigger the failSafe mode. For
     * example, when a system stays at a powered-off state, its CPU Temp
     * sensors will be unavailable, these unavailable sensors should not be
     * treated as failed and trigger failSafe.
     * This is important for systems whose Fans are always on.
     */
    if (!_typeFan && !_available && !_unavailableAsFailed)
    {
        return false;
    }

    // If a reading has came in,
    // but its value bad in some way (determined by sensor type),
    // indicate this sensor has failed,
    // until another value comes in that is no longer bad.
    // This is different from the overall _failed flag,
    // which is set and cleared by other causes.
    if (_badReading)
    {
        outputFailsafeLogWithSensor(_id, true, _id,
                                    "The sensor has bad readings.");
        return true;
    }

    // If a reading has came in, and it is not a bad reading,
    // but it indicates there is no more thermal margin left,
    // that is bad, something is wrong with the PID loops,
    // they are not cooling the system, enable failsafe mode also.
    if (_marginHot)
    {
        outputFailsafeLogWithSensor(_id, true, _id,
                                    "The sensor has no thermal margin left.");
        return true;
    }

    if (_failed)
    {
        outputFailsafeLogWithSensor(
            _id, true, _id, "The sensor has failed with a critical issue.");
        return true;
    }

    if (!_available)
    {
        outputFailsafeLogWithSensor(_id, true, _id,
                                    "The sensor is unavailable.");
        return true;
    }

    if (!_functional)
    {
        outputFailsafeLogWithSensor(_id, true, _id,
                                    "The sensor is not functional.");
        return true;
    }

    outputFailsafeLogWithSensor(_id, false, _id, "The sensor has recovered.");

    return false;
}

std::string DbusPassive::getFailReason(void) const
{
    if (_objectMissing)
    {
        return "Sensor D-Bus object missing";
    }
    if (_badReading)
    {
        return "Sensor reading bad";
    }
    if (_marginHot)
    {
        return "Margin hot";
    }
    if (_failed)
    {
        return "Sensor threshold asserted";
    }
    if (!_available)
    {
        return "Sensor unavailable";
    }
    if (!_functional)
    {
        return "Sensor not functional";
    }
    return "Unknown";
}

void DbusPassive::setFailed(bool value)
{
    _failed = value;
}

void DbusPassive::setFunctional(bool value)
{
    _functional = value;
}

void DbusPassive::setAvailable(bool value)
{
    _available = value;
}

int64_t DbusPassive::getScale(void)
{
    return _scale;
}

std::string DbusPassive::getID(void)
{
    return _id;
}

double DbusPassive::getMax(void)
{
    return _max;
}

double DbusPassive::getMin(void)
{
    return _min;
}

void DbusPassive::updateValue(double value, bool force)
{
    _badReading = false;

    // Do not let a NAN, or other floating-point oddity, be used to update
    // the value, as that indicates the sensor has no valid reading.
    if (!(std::isfinite(value)))
    {
        _badReading = true;

        // Do not continue with a bad reading, unless caller forcing
        if (!force)
        {
            return;
        }
    }

    value *= std::pow(10.0, _scale);

    auto unscaled = value;
    scaleSensorReading(_min, _max, value);

    if (_typeMargin)
    {
        _marginHot = false;

        // Unlike an absolute temperature sensor,
        // where 0 degrees C is a good reading,
        // a value received of 0 (or negative) margin is worrisome,
        // and should be flagged.
        // Either it indicates margin not calculated properly,
        // or somebody forgot to set the margin-zero setpoint,
        // or the system is really overheating that much.
        // This is a different condition from _failed
        // and _badReading, so it merits its own flag.
        // The sensor has not failed, the reading is good, but the zone
        // still needs to know that it should go to failsafe mode.
        if (unscaled <= 0.0)
        {
            _marginHot = true;
        }
    }

    setValue(value, unscaled);
}

int handleSensorValue(sdbusplus::message_t& msg, DbusPassive* owner)
{
    std::string msgSensor;
    std::map<std::string, std::variant<int64_t, double, bool>> msgData;

    msg.read(msgSensor, msgData);

    if (msgSensor == "xyz.openbmc_project.Sensor.Value")
    {
        auto valPropMap = msgData.find("Value");
        if (valPropMap != msgData.end())
        {
            double value =
                std::visit(VariantToDoubleVisitor(), valPropMap->second);

            owner->updateValue(value, false);
        }
    }
    else if (msgSensor == "xyz.openbmc_project.Sensor.Threshold.Critical")
    {
        auto criticalAlarmLow = msgData.find("CriticalAlarmLow");
        auto criticalAlarmHigh = msgData.find("CriticalAlarmHigh");
        if (criticalAlarmHigh == msgData.end() &&
            criticalAlarmLow == msgData.end())
        {
            return 0;
        }

        bool asserted = false;
        if (criticalAlarmLow != msgData.end())
        {
            asserted = std::get<bool>(criticalAlarmLow->second);
        }

        // checking both as in theory you could de-assert one threshold and
        // assert the other at the same moment
        if (!asserted && criticalAlarmHigh != msgData.end())
        {
            asserted = std::get<bool>(criticalAlarmHigh->second);
        }
        owner->setFailed(asserted);
    }
#ifdef UNC_FAILSAFE
    else if (msgSensor == "xyz.openbmc_project.Sensor.Threshold.Warning")
    {
        auto warningAlarmHigh = msgData.find("WarningAlarmHigh");
        if (warningAlarmHigh == msgData.end())
        {
            return 0;
        }

        bool asserted = false;
        if (warningAlarmHigh != msgData.end())
        {
            asserted = std::get<bool>(warningAlarmHigh->second);
        }
        owner->setFailed(asserted);
    }
#endif
    else if (msgSensor == "xyz.openbmc_project.State.Decorator.Availability")
    {
        auto available = msgData.find("Available");
        if (available == msgData.end())
        {
            return 0;
        }
        bool asserted = std::get<bool>(available->second);
        owner->setAvailable(asserted);
        if (!asserted)
        {
            // A thermal controller will continue its PID calculation and not
            // trigger a 'failsafe' when some inputs are unavailable.
            // So, forced to clear the value here to prevent a historical
            // value to participate in a latter PID calculation.
            owner->updateValue(std::numeric_limits<double>::quiet_NaN(), true);
        }
    }
    else if (msgSensor ==
             "xyz.openbmc_project.State.Decorator.OperationalStatus")
    {
        auto functional = msgData.find("Functional");
        if (functional == msgData.end())
        {
            return 0;
        }
        bool asserted = std::get<bool>(functional->second);
        owner->setFunctional(asserted);
    }

    return 0;
}

int dbusHandleSignal(sd_bus_message* msg, void* usrData,
                     [[maybe_unused]] sd_bus_error* err)
{
    auto sdbpMsg = sdbusplus::message_t(msg);
    DbusPassive* obj = static_cast<DbusPassive*>(usrData);

    return handleSensorValue(sdbpMsg, obj);
}

} // namespace pid_control
