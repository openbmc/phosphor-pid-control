/**
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pid/buildjson.hpp"

#include "conf.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <tuple>

using json = nlohmann::json;

std::pair<std::map<int64_t, PIDConf>, std::map<int64_t, struct ZoneConfig>>
    buildPIDsFromJson(const json& data)
{
    // zone -> pids
    std::map<int64_t, PIDConf> pidConfig;
    // zone -> configs
    std::map<int64_t, struct ZoneConfig> zoneConfig;

    /* TODO: if zones is empty, that's invalid. */
    auto zones = data["zones"];
    for (const auto& zone : zones)
    {
        int64_t id;
        PIDConf thisZone;
        struct ZoneConfig thisZoneConfig;

        /* TODO: using at() throws a specific exception we can catch */
        id = zone["id"];
        thisZoneConfig.minthermalrpm = zone["minthermalrpm"];
        thisZoneConfig.failsafepercent = zone["failsafepercent"];

        auto pids = zone["pids"];
        for (const auto& pid : pids)
        {
        }
    }

    return std::make_pair(pidConfig, zoneConfig);
}
