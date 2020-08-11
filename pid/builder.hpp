#pragma once

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <unordered_map>

namespace pid_control
{

std::unordered_map<int64_t, std::unique_ptr<DbusPidZone>>
    buildZones(const std::map<int64_t, conf::PIDConf>& zonePids,
               std::map<int64_t, struct conf::ZoneConfig>& zoneConfigs,
               SensorManager& mgr, sdbusplus::bus::bus& modeControlBus);

}
