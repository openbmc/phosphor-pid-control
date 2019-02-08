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

#include "buildjson.hpp"

#include "conf.hpp"
#include "sensors/sensor.hpp"

#include <cstdio>
#include <fstream>
#include <nlohmann/json.hpp>

static const auto logging = "/tmp/swampd.log";

using json = nlohmann::json;

void from_json(const json& j, SensorConfig& s)
{
    j.at("type").get_to(s.type);
    j.at("readpath").get_to(s.readpath);

    /* The writepath field is optional in a configuration */
    auto writepath = j.find("writepath");
    if (writepath == j.end())
    {
        s.writepath = "";
    }
    else
    {
        j.at("writepath").get_to(s.writepath);
    }

    /* The min field is optional in a configuration. */
    auto min = j.find("min");
    if (min == j.end())
    {
        s.min = 0;
    }
    else
    {
        j.at("min").get_to(s.min);
    }

    /* The max field is optional in a configuration. */
    auto max = j.find("max");
    if (max == j.end())
    {
        s.max = 0;
    }
    else
    {
        j.at("max").get_to(s.max);
    }

    /* The timeout field is optional in a configuration. */
    auto timeout = j.find("timeout");
    if (timeout == j.end())
    {
        s.timeout = Sensor::getDefaultTimeout(s.type);
    }
    else
    {
        j.at("timeout").get_to(s.timeout);
    }
}

std::map<std::string, struct SensorConfig>
    buildSensorsFromJson(const json& data)
{
    std::map<std::string, struct SensorConfig> config;
    auto sensors = data["sensors"];

    std::ofstream log(logging);

    for (const auto& sensor : sensors)
    {
        log << "name: " << sensor["name"] << std::endl;

        /* TODO: There is a mechanism for just dumping from json to the object,
         * do this.
         */
        auto item = sensor.get<struct SensorConfig>();
        log << "\ttype: " << item.type << std::endl;
        log << "\treadpath: " << item.readpath << std::endl;
        log << "\twritepath: " << item.writepath << std::endl;
        log << "\tmin: " << item.min << std::endl;
        log << "\tmax: " << item.max << std::endl;
        log << "\ttimeout: " << item.timeout << std::endl;
    }

    return config;
}

void buildConfigJson(const std::string& path)
{
    std::ofstream log(logging);
    log << "json path: " << path << std::endl;

    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        log << "file doesn't exist" << std::endl;
        return;
    }

    json data = json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        log << "failed to parse json contents" << std::endl;
        return;
    }

    auto sensors = data.find("sensors");
    auto zones = data.find("zones");
    if (sensors == data.end())
    {
        log << "sensors not found!" << std::endl;
        return;
    }
    if (zones == data.end())
    {
        log << "zones not found!" << std::endl;
        return;
    }

    log.close();

    buildSensorsFromJson(data);
}
