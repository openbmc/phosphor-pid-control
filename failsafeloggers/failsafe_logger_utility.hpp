#pragma once

#include "failsafeloggers/failsafe_logger.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

/** Map of the zone ID to its failsafe logger.
 */
extern std::unordered_map<int64_t, std::shared_ptr<pid_control::FailsafeLogger>>
    zoneIdToFailsafeLogger;

namespace pid_control
{

/** Given a zone ID, attempt to output entering/leaving-failsafe-mode
 * logs for its corresponding zones.
 */
inline void outputFailsafeLogWithZone(
    const int64_t zoneId, const bool newFailsafeState,
    const std::string& location, const std::string& reason)
{
    if (zoneIdToFailsafeLogger.count(zoneId))
    {
        zoneIdToFailsafeLogger[zoneId]->outputFailsafeLog(
            zoneId, newFailsafeState, location, reason);
    }
}
} // namespace pid_control
