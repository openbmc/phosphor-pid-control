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
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <sdbusplus/bus.hpp>
#include <string>

#include "dbuspassive.hpp"
#include "dbus/util.hpp"

std::unique_ptr<ReadInterface> DbusPassive::CreateDbusPassive(
    sdbusplus::bus::bus& bus, const std::string& type,
    const std::string& id, DbusHelperInterface *helper)
{
    if (helper == nullptr)
    {
        return nullptr;
    }
    if (!ValidType(type))
    {
        return nullptr;
    }

    return std::make_unique<DbusPassive>(bus, type, id, helper);
}

DbusPassive::DbusPassive(
    sdbusplus::bus::bus& bus,
    const std::string& type,
    const std::string& id,
    DbusHelperInterface *helper)
    : ReadInterface(),
      _bus(bus),
      _signal(bus, GetMatch(type, id).c_str(), DbusHandleSignal, this),
      _id(id),
      _helper(helper)
{
    /* Need to get the scale and initial value */
    auto tempBus = sdbusplus::bus::new_default();
    /* service == busname */
    std::string path = GetSensorPath(type, id);
    std::string service = _helper->GetService(tempBus, sensorintf, path);

    struct SensorProperties settings;
    _helper->GetProperties(tempBus, service, path, &settings);

    _scale = settings.scale;
    _value = settings.value * pow(10, _scale);
    _updated = std::chrono::high_resolution_clock::now();
}

ReadReturn DbusPassive::read(void)
{
    std::lock_guard<std::mutex> guard(_lock);

    struct ReadReturn r = {
        _value,
        _updated
    };

    return r;
}

void DbusPassive::setValue(double value)
{
    std::lock_guard<std::mutex> guard(_lock);

    _value = value;
    _updated = std::chrono::high_resolution_clock::now();
}

int64_t DbusPassive::getScale(void)
{
    return _scale;
}

std::string DbusPassive::getId(void)
{
    return _id;
}

int HandleSensorValue(sdbusplus::message::message& msg, DbusPassive* owner)
{
    std::string msgSensor;
    std::map<std::string, sdbusplus::message::variant<int64_t>> msgData;

    msg.read(msgSensor, msgData);

    if (msgSensor == "xyz.openbmc_project.Sensor.Value")
    {
        auto valPropMap = msgData.find("Value");
        if (valPropMap != msgData.end())
        {
            int64_t rawValue = sdbusplus::message::variant_ns::get<int64_t>(
                valPropMap->second);

            double value = rawValue * std::pow(10, owner->getScale());

            owner->setValue(value);
        }
    }

    return 0;
}

int DbusHandleSignal(sd_bus_message* msg, void* usrData, sd_bus_error* err)
{
    auto sdbpMsg = sdbusplus::message::message(msg);
    DbusPassive* obj = static_cast<DbusPassive*>(usrData);

    return HandleSensorValue(sdbpMsg, obj);
}
