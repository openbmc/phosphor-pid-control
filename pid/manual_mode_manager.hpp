#pragma once

#include <cstdint>
#include <unordered_map>

namespace pid_control
{

class ManualModeManager
{
  public:
    bool get(int64_t zoneId) const
    {
        auto it = modes.find(zoneId);
        return it != modes.end() ? it->second : false;
    }

    void set(int64_t zoneId, bool manual)
    {
        modes[zoneId] = manual;
    }

  private:
    // Map of zone ID to its manual-mode state, persisted across zone
    // rebuilds so that a zone re-created by restartControlLoops()
    // inherits the operator's last manual/auto choice.
    std::unordered_map<int64_t, bool> modes;
};

} // namespace pid_control
