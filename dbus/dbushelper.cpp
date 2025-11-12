// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "config.h"

#include "dbushelper.hpp"

#include "dbushelper_interface.hpp"
#include "dbusutil.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/ObjectMapper/common.hpp>
#include <xyz/openbmc_project/Sensor/Value/client.hpp>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using ObjectMapper = sdbusplus::common::xyz::openbmc_project::ObjectMapper;
using SensorValue = sdbusplus::common::xyz::openbmc_project::sensor::Value;

namespace pid_control
{

using Property = std::string;
using Value = std::variant<int64_t, double, std::string, bool>;
using PropertyMap = std::map<Property, Value>;

using namespace phosphor::logging;

/* TODO(venture): Basically all phosphor apps need this, maybe it should be a
 * part of sdbusplus.  There is an old version in libmapper.
 */
std::string DbusHelper::getService(const std::string& intf,
                                   const std::string& path)
{
    auto mapper = _bus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_object);

    mapper.append(path);
    mapper.append(std::vector<std::string>({intf}));

    std::map<std::string, std::vector<std::string>> response;

    try
    {
        auto responseMsg = _bus.call(mapper);

        responseMsg.read(response);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        log<level::ERR>("ObjectMapper call failure",
                        entry("WHAT=%s", ex.what()));
        throw;
    }

    if (response.begin() == response.end())
    {
        throw std::runtime_error("Unable to find Object: " + path);
    }

    return response.begin()->first;
}

void DbusHelper::getProperties(const std::string& service,
                               const std::string& path, SensorProperties* prop)
{
    auto pimMsg = _bus.new_method_call(service.c_str(), path.c_str(),
                                       propertiesintf, "GetAll");

    pimMsg.append(SensorValue::interface);

    PropertyMap propMap;

    try
    {
        auto valueResponseMsg = _bus.call(pimMsg);
        valueResponseMsg.read(propMap);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        log<level::ERR>("GetAll Properties Failed",
                        entry("WHAT=%s", ex.what()));
        throw;
    }

    // The PropertyMap returned will look like this because it's always
    // reading a Sensor.Value interface.
    // a{sv} 3:
    // "Value" x 24875
    // "Unit" s "xyz.openbmc_project.Sensor.Value.Unit.DegreesC"
    // "Scale" x -3

    // If no error was set, the values should all be there.
    auto findUnit = propMap.find(SensorValue::property_names::unit);
    if (findUnit != propMap.end())
    {
        prop->unit = std::get<std::string>(findUnit->second);
    }
    // TODO: in PDI there is no such 'Scale' property on the Sensor.Value
    // interface
    auto findScale = propMap.find("Scale");
    auto findMax = propMap.find(SensorValue::property_names::max_value);
    auto findMin = propMap.find(SensorValue::property_names::min_value);

    prop->min = 0;
    prop->max = 0;
    prop->scale = 0;
    if (findScale != propMap.end())
    {
        prop->scale = std::get<int64_t>(findScale->second);
    }
    if (findMax != propMap.end())
    {
        prop->max = std::visit(VariantToDoubleVisitor(), findMax->second);
    }
    if (findMin != propMap.end())
    {
        prop->min = std::visit(VariantToDoubleVisitor(), findMin->second);
    }

    prop->value = std::visit(VariantToDoubleVisitor(),
                             propMap[SensorValue::property_names::value]);

    bool available = true;
    try
    {
        getProperty(service, path, StateDecoratorAvailability::interface,
                    StateDecoratorAvailability::property_names::available,
                    available);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        // unsupported Available property, leaving reading at 'True'
    }
    prop->available = available;

    return;
}

bool DbusHelper::thresholdsAsserted(const std::string& service,
                                    const std::string& path)
{
    auto critical = _bus.new_method_call(service.c_str(), path.c_str(),
                                         propertiesintf, "GetAll");
    critical.append(SensorThresholdCritical::interface);
    PropertyMap criticalMap;

    try
    {
        auto msg = _bus.call(critical);
        msg.read(criticalMap);
    }
    catch (const sdbusplus::exception_t&)
    {
        // do nothing, sensors don't have to expose critical thresholds
#ifndef UNC_FAILSAFE
        return false;
#endif
    }

    auto findCriticalLow = criticalMap.find(
        SensorThresholdCritical::property_names::critical_alarm_low);
    auto findCriticalHigh = criticalMap.find(
        SensorThresholdCritical::property_names::critical_alarm_high);

    bool asserted = false;
    if (findCriticalLow != criticalMap.end())
    {
        asserted = std::get<bool>(findCriticalLow->second);
    }

    // as we are catching properties changed, a sensor could theoretically jump
    // from one threshold to the other in one event, so check both thresholds
    if (!asserted && findCriticalHigh != criticalMap.end())
    {
        asserted = std::get<bool>(findCriticalHigh->second);
    }
#ifdef UNC_FAILSAFE
    if (!asserted)
    {
        auto warning = _bus.new_method_call(service.c_str(), path.c_str(),
                                            propertiesintf, "GetAll");
        warning.append(warningThreshInf);
        PropertyMap warningMap;

        try
        {
            auto msg = _bus.call(warning);
            msg.read(warningMap);
        }
        catch (const sdbusplus::exception_t&)
        {
            // sensors don't have to expose non-critical thresholds
            return false;
        }
        auto findWarningHigh = warningMap.find(
            SensorThresholdWarning::property_names::warning_alarm_high);

        if (findWarningHigh != warningMap.end())
        {
            asserted = std::get<bool>(findWarningHigh->second);
        }
    }
#endif
    return asserted;
}

} // namespace pid_control
