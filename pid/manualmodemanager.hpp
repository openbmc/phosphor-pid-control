#pragma once

#include <unordered_map>
#include <string>
#include <iostream>

namespace pid_control
{

class ManualModeManager {
public:
    void saveManualMode(int64_t zoneId, bool manualMode) {
        manualModes[zoneId] = manualMode;
    }

    bool restoreManualMode(int64_t zoneId) const {
        auto it = manualModes.find(zoneId);
        if (it != manualModes.end()) {
            return it->second;
        }
        return false;
    }

private:
    std::unordered_map<int64_t, bool> manualModes;
};

}// namespace pid_control

