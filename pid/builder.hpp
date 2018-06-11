#pragma once

#include <map>
#include <memory>
#include <sdbusplus/bus.hpp>

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

std::map<int64_t, std::shared_ptr<PIDZone>> BuildZones(
        std::map<int64_t, PIDConf>& zonePids,
        std::map<int64_t, struct zone>& zoneConfigs,
        std::shared_ptr<SensorManager> mgr,
        sdbusplus::bus::bus& modeControlBus);
