#pragma once

#include "conf.hpp"
#include "pid/zone_interface.hpp"
#include "sensors/manager.hpp"

#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <memory>
#include <unordered_map>

namespace pid_control
{

std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>>
    buildZones(const std::map<int64_t, conf::PIDConf>& zonePids,
               std::map<int64_t, conf::ZoneConfig>& zoneConfigs,
               SensorManager& mgr,
               sdbusplus::asio::object_server& modeControlServer);

}
