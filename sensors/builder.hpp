#pragma once

#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"

#include <map>
#include <string>

namespace pid_control
{

/**
 * Build the sensors and associate them with a SensorManager.
 */
SensorManager
    buildSensors(const std::map<std::string, struct conf::SensorConfig>& config,
                 sdbusplus::bus::bus& passive, sdbusplus::bus::bus& host);

} // namespace pid_control
