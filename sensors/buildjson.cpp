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

#include "sensors/buildjson.hpp"

#include "conf.hpp"
#include "sensors/sensor.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void from_json(const json& j, SensorConfig& s)
{
    j.at("type").get_to(s.type);
    j.at("readPath").get_to(s.readPath);

    /* The writePath field is optional in a configuration */
    auto writePath = j.find("writePath");
    if (writePath == j.end())
    {
        s.writePath = "";
    }
    else
    {
        j.at("writePath").get_to(s.writePath);
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

    /* TODO: If no sensors, this is invalid, and we should except here or during
     * parsing.
     */
    for (const auto& sensor : sensors)
    {
        config[sensor["name"]] = sensor.get<struct SensorConfig>();
    }

    return config;
}
