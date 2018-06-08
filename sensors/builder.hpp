#pragma once

#include <map>
#include <string>

#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"

/**
 * Build the sensors and associate them with a SensorManager.
 */
SensorManager BuildSensors(
    const std::map<std::string, struct sensor>& config);

