#pragma once

#include <map>
#include <memory>
#include <sdbusplus/bus.hpp>

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZones(
        std::map<int64_t, PIDConf>& ZonePids,
        std::map<int64_t, struct zone>& ZoneConfigs,
        std::shared_ptr<SensorManager> mgr,
        sdbusplus::bus::bus& ModeControlBus);
