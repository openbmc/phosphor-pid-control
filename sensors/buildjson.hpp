#pragma once

#include "conf.hpp"

#include <nlohmann/json.hpp>

#include <map>
#include <string>

using json = nlohmann::json;

namespace pid_control
{

/**
 * Given a json object generated from a configuration file, build the sensor
 * configuration representation. This expecteds the json configuration to be
 * valid.
 *
 * @param[in] data - the json data
 * @return a map of sensors.
 */
std::map<std::string, conf::SensorConfig> buildSensorsFromJson(
    const json& data);

} // namespace pid_control
