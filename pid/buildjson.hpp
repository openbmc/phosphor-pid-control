#pragma once

#include "conf.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <utility>

namespace pid_control
{

using json = nlohmann::json;

/**
 * Given the json "zones" data, create the map of PIDs and the map of zones.
 *
 * @param[in] data - the json data
 * @return the pidConfig, and the zoneConfig
 */
std::pair<std::map<int64_t, conf::PIDConf>, std::map<int64_t, conf::ZoneConfig>>
    buildPIDsFromJson(const json& data);

} // namespace pid_control
