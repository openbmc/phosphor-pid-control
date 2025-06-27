#pragma once

#include "conf.hpp"
#include "sensors/manager.hpp"

#include <sdbusplus/bus.hpp>

#include <map>
#include <string>

namespace pid_control
{

/**
 * Build the sensors and associate them with a SensorManager.
 */
SensorManager buildSensors(
    const std::map<std::string, conf::SensorConfig>& config,
    sdbusplus::bus_t& passive, sdbusplus::bus_t& host);

} // namespace pid_control
