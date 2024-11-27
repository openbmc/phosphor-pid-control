#pragma once

#include "conf.hpp"
#include "failsafeloggers/failsafe_logger.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/** Map of the zone ID to its failsafe logger.
 */
extern std::unordered_map<int64_t, std::shared_ptr<pid_control::FailsafeLogger>>
    zoneIdToFailsafeLogger;

/** Map of the sensor name/ID to its corresponding zone IDs.
 */
extern std::unordered_map<std::string, std::vector<int64_t>> sensorNameToZoneId;

namespace pid_control
{

/** Given a sensor name, attempt to output entering/leaving-failsafe-mode
 * logs for its corresponding zones.
 */
inline void outputFailsafeLogWithSensor(
    const std::string sensorName, const bool newFailsafeState,
    const std::string location, const std::string reason)
{
    for (const int64_t zoneId : sensorNameToZoneId[sensorName])
    {
        if (zoneIdToFailsafeLogger.count(zoneId))
        {
            zoneIdToFailsafeLogger[zoneId]->outputFailsafeLog(
                zoneId, newFailsafeState, location, reason);
        }
    }
}

/** Given a zone ID, attempt to output entering/leaving-failsafe-mode
 * logs for its corresponding zones.
 */
inline void outputFailsafeLogWithZone(
    const int64_t zoneId, const bool newFailsafeState,
    const std::string location, const std::string reason)
{
    if (zoneIdToFailsafeLogger.count(zoneId))
    {
        zoneIdToFailsafeLogger[zoneId]->outputFailsafeLog(
            zoneId, newFailsafeState, location, reason);
    }
}
} // namespace pid_control
