#pragma once

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>
#include <unordered_map>

std::unordered_map<int64_t, std::unique_ptr<PIDZone>>
    buildZonesFromConfig(const std::string& path, SensorManager& mgr,
                         sdbusplus::bus::bus& modeControlBus);
