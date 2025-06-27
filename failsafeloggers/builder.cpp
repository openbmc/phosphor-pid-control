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

#include "failsafeloggers/builder.hpp"

#include "failsafeloggers/failsafe_logger.hpp"
#include "failsafeloggers/failsafe_logger_utility.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace pid_control
{

void buildFailsafeLoggers(
    const std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>>& zones,
    const size_t logMaxCountPerSecond /* = 20 */)
{
    zoneIdToFailsafeLogger =
        std::unordered_map<int64_t,
                           std::shared_ptr<pid_control::FailsafeLogger>>();
    sensorNameToZoneId =
        std::unordered_map<std::string, std::vector<int64_t>>();
    for (const auto& zoneIdToZone : zones)
    {
        int64_t zoneId = zoneIdToZone.first;
        // Create a failsafe logger for each zone.
        zoneIdToFailsafeLogger[zoneId] = std::make_shared<FailsafeLogger>(
            logMaxCountPerSecond, zoneIdToZone.second->getFailSafeMode());
        // Build the sensor-zone topology map.
        std::vector<std::string> sensorNames =
            zoneIdToZone.second->getSensorNames();
        for (const std::string& sensorName : sensorNames)
        {
            if (std::find(sensorNameToZoneId[sensorName].begin(),
                          sensorNameToZoneId[sensorName].end(), zoneId) ==
                sensorNameToZoneId[sensorName].end())
            {
                sensorNameToZoneId[sensorName].push_back(zoneId);
            }
        }
        std::cerr << "Build failsafe logger for Zone " << zoneId
                  << " with initial "
                  << "failsafe mode: " << zoneIdToZone.second->getFailSafeMode()
                  << "\n";
    }
}

} // namespace pid_control
