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

#include "dbushelper.hpp"

#include "dbushelper_interface.hpp"
#include "dbusutil.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace pid_control
{

using Property = std::string;
using Value = std::variant<int64_t, double, std::string, bool>;
using PropertyMap = std::map<Property, Value>;

using namespace phosphor::logging;

/* TODO(venture): Basically all phosphor apps need this, maybe it should be a
 * part of sdbusplus.  There is an old version in libmapper.
 */
std::string DbusHelper::getService(sdbusplus::bus::bus& bus,
                                   const std::string& intf,
                                   const std::string& path)
{
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapper.append(path);
    mapper.append(std::vector<std::string>({intf}));

    std::map<std::string, std::vector<std::string>> response;

    try
    {
        auto responseMsg = bus.call(mapper);

        responseMsg.read(response);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
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

void DbusHelper::getProperties(sdbusplus::bus::bus& bus,
                               const std::string& service,
                               const std::string& path,
                               struct SensorProperties* prop)
{
    auto pimMsg = bus.new_method_call(service.c_str(), path.c_str(),
                                      propertiesintf, "GetAll");

    pimMsg.append(sensorintf);

    PropertyMap propMap;

    try
    {
        auto valueResponseMsg = bus.call(pimMsg);
        valueResponseMsg.read(propMap);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
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
    auto findUnit = propMap.find("Unit");
    if (findUnit != propMap.end())
    {
        prop->unit = std::get<std::string>(findUnit->second);
    }
    auto findScale = propMap.find("Scale");
    auto findMax = propMap.find("MaxValue");
    auto findMin = propMap.find("MinValue");

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

    prop->value = std::visit(VariantToDoubleVisitor(), propMap["Value"]);

    return;
}

bool DbusHelper::thresholdsAsserted(sdbusplus::bus::bus& bus,
                                    const std::string& service,
                                    const std::string& path)
{

    auto critical = bus.new_method_call(service.c_str(), path.c_str(),
                                        propertiesintf, "GetAll");
    critical.append(criticalThreshInf);
    PropertyMap criticalMap;

    try
    {
        auto msg = bus.call(critical);
        msg.read(criticalMap);
    }
    catch (sdbusplus::exception_t&)
    {
        // do nothing, sensors don't have to expose critical thresholds
        return false;
    }

    auto findCriticalLow = criticalMap.find("CriticalAlarmLow");
    auto findCriticalHigh = criticalMap.find("CriticalAlarmHigh");

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
    return asserted;
}

} // namespace pid_control
