// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

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
