#pragma once

#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>
#include <unordered_map>

#include "pid/zone.hpp"
#include "sensors/manager.hpp"

std::unordered_map<int64_t, std::unique_ptr<PIDZone>> BuildZonesFromConfig(
        const std::string& path,
        SensorManager& mgr,
        sdbusplus::bus::bus& modeControlBus);
