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

    void clear()
    {
        modes.clear();
    }

  private:
    std::unordered_map<int64_t, bool> modes;
};

} // namespace pid_control
