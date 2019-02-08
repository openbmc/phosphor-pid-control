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

void from_json(const json& j, ControllerInfo& c)
{
    j.at("type").get_to(c.type);
    j.at("inputs").get_to(c.inputs);
    j.at("set-point").get_to(c.setpoint);

    /* TODO: We need to handle parsing other PID controller configurations.
     * We can do that by checking for different keys and making the decision
     * accordingly.
     */
    auto p = j.at("pid");
    p.at("sampleperiod").get_to(c.pidInfo.ts);
    p.at("p_coefficient").get_to(c.pidInfo.p_c);
    p.at("i_coefficient").get_to(c.pidInfo.i_c);
    p.at("ff_off_coefficient").get_to(c.pidInfo.ff_off);
    p.at("ff_gain_coefficient").get_to(c.pidInfo.ff_gain);
    p.at("i_limit_min").get_to(c.pidInfo.i_lim.min);
    p.at("i_limit_max").get_to(c.pidInfo.i_lim.max);
    p.at("out_limit_min").get_to(c.pidInfo.out_lim.min);
    p.at("out_limit_max").get_to(c.pidInfo.out_lim.max);
    p.at("slew_neg").get_to(c.pidInfo.slew_neg);
    p.at("slew_pos").get_to(c.pidInfo.slew_pos);

    auto positiveHysteresis = p.find("positiveHysteresis");
    if (positiveHysteresis == p.end()) {
        c.pidInfo.positiveHysteresis = 0.0;
    } else {
        j.at("positiveHysteresis").get_to(c.pidInfo.positiveHysteresis);
    }

    auto negativeHysteresis = p.find("negativeHysteresis");
    if (negativeHysteresis == p.end()) {
        c.pidInfo.negativeHysteresis = 0.0;
    } else {
        j.at("negativeHysteresis").get_to(c.pidInfo.negativeHysteresis);
    }
}

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
            auto name = pid["name"];
            auto item = pid.get<ControllerInfo>();

            thisZone[name] = item;
        }

        pidConfig[id] = thisZone;
        zoneConfig[id] = thisZoneConfig;
    }

    return std::make_pair(pidConfig, zoneConfig);
}
