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
#include "sensors/manager.hpp"

#include "conf.hpp"

#include <memory>
#include <string>

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
