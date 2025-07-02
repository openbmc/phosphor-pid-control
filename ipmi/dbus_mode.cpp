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

#include "dbus_mode.hpp"

#include <ipmid/api-types.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <variant>

namespace pid_control::ipmi
{

static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";
static constexpr auto busName = "xyz.openbmc_project.State.FanCtrl";
static constexpr auto intf = "xyz.openbmc_project.Control.Mode";
static constexpr auto propertiesintf = "org.freedesktop.DBus.Properties";

using Property = std::string;
using Value = std::variant<bool>;
using PropertyMap = std::map<Property, Value>;

/* The following was copied directly from my manual thread handler. */
static std::string getControlPath(int8_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

uint8_t DbusZoneControl::getFanCtrlProperty(uint8_t zoneId, bool* value,
                                            const std::string& property)
{
    std::string path = getControlPath(zoneId);

    auto propertyReadBus = sdbusplus::bus::new_system();
    auto pimMsg = propertyReadBus.new_method_call(busName, path.c_str(),
                                                  propertiesintf, "GetAll");
    pimMsg.append(intf);

    try
    {
        PropertyMap propMap;

        /* a method could error but the call not error. */
        auto valueResponseMsg = propertyReadBus.call(pimMsg);

        valueResponseMsg.read(propMap);

        *value = std::get<bool>(propMap[property]);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        return ::ipmi::ccInvalidCommand;
    }

    return ::ipmi::ccSuccess;
}

uint8_t DbusZoneControl::setFanCtrlProperty(uint8_t zoneId, bool value,
                                            const std::string& property)
{
    using Value = std::variant<bool>;
    Value v{value};

    std::string path = getControlPath(zoneId);

    auto PropertyWriteBus = sdbusplus::bus::new_system();
    auto pimMsg = PropertyWriteBus.new_method_call(busName, path.c_str(),
                                                   propertiesintf, "Set");
    pimMsg.append(intf);
    pimMsg.append(property);
    pimMsg.append(v);

    try
    {
        PropertyWriteBus.call_noreply(pimMsg);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        return ::ipmi::ccInvalidCommand;
    }

    /* TODO(venture): Should sanity check the result. */
    return ::ipmi::ccSuccess;
}

} // namespace pid_control::ipmi
