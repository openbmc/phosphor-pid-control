// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

/* Configuration. */
#include "sensors/manager.hpp"

#include "sensor.hpp"

#include <memory>
#include <string>
#include <utility>

namespace pid_control
{

void SensorManager::addSensor(const std::string& type, const std::string& name,
                              std::unique_ptr<Sensor> sensor)
{
    _sensorMap[name] = std::move(sensor);

    auto entry = _sensorTypeList.find(type);
    if (entry == _sensorTypeList.end())
    {
        _sensorTypeList[type] = {};
    }

    _sensorTypeList[type].push_back(name);
}

} // namespace pid_control
