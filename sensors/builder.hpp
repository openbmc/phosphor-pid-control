#pragma once

#include <map>
#include <memory>
#include <string>

#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"

/**
 * Build the sensors and associate them with a SensorManager.
 */
std::shared_ptr<SensorManager> BuildSensors(
    const std::map<std::string, struct sensor>& config);

