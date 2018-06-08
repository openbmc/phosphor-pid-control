#pragma once

#include <memory>
#include <string>

#include "sensors/manager.hpp"

/**
 * Given a configuration file, parsable by libconfig++, parse it and then pass
 * the information onto BuildSensors.
 */
std::shared_ptr<SensorManager> BuildSensorsFromConfig(std::string& path);
