#include "failsafe_logger.hpp"

#include <chrono>
#include <iostream>

namespace pid_control
{

void FailsafeLogger::outputFailsafeLog(
    const int64_t zoneId, const bool newFailsafeState,
    const std::string location, const std::string reason)
{
    // Remove outdated log entries.
    const auto now = std::chrono::high_resolution_clock::now();
    uint64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();
    // Limit the log output in 1 second.
    uint64_t secondInMS = 1000; // 1 second in milliseconds
    while (!_logTimestamps.empty() &&
           nowMs - _logTimestamps.front() >= secondInMS)
    {
        _logTimestamps.pop_front();
    }

    // There is a failsafe state change, clear the logs in current state.
    bool originFailsafeState = _currentFailsafeState;
    if (newFailsafeState != _currentFailsafeState)
    {
        _logsInCurrentState.clear();
        _currentFailsafeState = newFailsafeState;
    }
    // Do not output the log if the capacity is reached, or if the log is
    // already encountered in the current state.
    std::string locationReason = location + " @ " + reason;
    if (_logTimestamps.size() >= _logMaxCountPerSecond ||
        !_logsInCurrentState.contains(locationReason))
    {
        return;
    }
    _logsInCurrentState.insert(locationReason);

    // Only output the log if the zone enters, stays in, or leaves failsafe
    // mode. No need to output the log if the zone stays in non-failsafe mode.
    if (newFailsafeState)
    {
        std::cerr << "Zone `" << zoneId
                  << "` is in failsafe mode.\t\tWith update at `" << location
                  << "`: " << reason << "\n";
    }
    else if (!newFailsafeState && originFailsafeState)
    {
        std::cerr << "Zone `" << zoneId
                  << "` leaves failsafe mode.\t\tWith update at `" << location
                  << "`: " << reason << "\n";
    }

    _logTimestamps.push_back(nowMs);
}

} // namespace pid_control
