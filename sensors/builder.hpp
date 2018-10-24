#pragma once

#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"

#include <map>
#include <string>

/**
 * Build the sensors and associate them with a SensorManager.
 */
SensorManager buildSensors(const std::map<std::string, struct sensor>& config);
