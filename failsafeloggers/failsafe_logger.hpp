#pragma once

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pid_control
{

/**
 * Log the reason for a zone to enter and leave the failsafe mode.
 *
 * Particularly, for entering the failsafe mode:
 *   1. A sensor is specified in thermal config as an input but missed in DBus
 *   2. A sensor has null readings in DBus
 *   3. A sensor is abnormal in DBus (not functional, not enabled, etc)
 *   4. A sensor's reading is above upper critical (UC) limit
 *
 * Among the above reasons:
 *   1 excludes 2, 3, 4.
 *   2 excludes 1, 4.
 *   3 excludes 1.
 *   4 excludes 1, 2.
 *
 * Note that this log is at the zone level, not the sensor level.
 */
class FailsafeLogger
{
  public:
    FailsafeLogger(size_t logMaxCountPerSecond = 20,
                   bool currentFailsafeState = false) :
        _logMaxCountPerSecond(logMaxCountPerSecond),
        _currentFailsafeState(currentFailsafeState)
    {}
    ~FailsafeLogger() = default;

    /** Attempt to output an entering/leaving-failsafe-mode log.
     */
    void outputFailsafeLog(int64_t zoneId, bool newFailsafeState,
                           const std::string& location,
                           const std::string& reason);

  private:
    // The maximum number of log entries to be output within 1 second.
    size_t _logMaxCountPerSecond;
    // Whether the zone is currently in the failsafe mode.
    bool _currentFailsafeState;
    // The timestamps of the log entries.
    std::deque<size_t> _logTimestamps;
    // The logs already encountered in the current state.
    std::unordered_set<std::string> _logsInCurrentState;
};

} // namespace pid_control
