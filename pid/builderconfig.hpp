#pragma once

#include <map>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZonesFromConfig(
        std::string& path,
        std::shared_ptr<SensorManager> mgr,
        sdbusplus::bus::bus& ModeControlBus);
