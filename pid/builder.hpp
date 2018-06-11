#pragma once

#include <memory>
#include <sdbusplus/bus.hpp>
#include <unordered_map>

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

std::unordered_map<int64_t, std::shared_ptr<PIDZone>> BuildZones(
        std::map<int64_t, PIDConf>& zonePids,
        std::map<int64_t, struct zone>& zoneConfigs,
        SensorManager& mgr,
        sdbusplus::bus::bus& modeControlBus);
