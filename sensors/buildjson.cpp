// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2019 Google Inc

#include "sensors/buildjson.hpp"

#include "conf.hpp"
#include "sensors/sensor.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <map>

using json = nlohmann::json;

namespace pid_control
{
namespace conf
{
void from_json(const json& j, conf::SensorConfig& s)
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

    /* Default to not ignore dbus MinValue/MaxValue - only used by passive
     * sensors.
     */
    s.ignoreDbusMinMax = false;
    s.unavailableAsFailed = true;
    s.ignoreFailIfHostOff = false;
    s.min = 0;
    s.max = 0;

    auto ignore = j.find("ignoreDbusMinMax");
    if (ignore != j.end())
    {
        j.at("ignoreDbusMinMax").get_to(s.ignoreDbusMinMax);
    }

    auto findunAsF = j.find("unavailableAsFailed");
    if (findunAsF != j.end())
    {
        j.at("unavailableAsFailed").get_to(s.unavailableAsFailed);
    }

    auto findIgnoreIfHostOff = j.find("ignoreFailIfHostOff");
    if (findIgnoreIfHostOff != j.end())
    {
        j.at("ignoreFailIfHostOff").get_to(s.ignoreFailIfHostOff);
    }

    /* The min field is optional in a configuration. */
    auto min = j.find("min");
    if (min != j.end())
    {
        if (s.type == "fan")
        {
            j.at("min").get_to(s.min);
        }
        else
        {
            std::fprintf(stderr, "Non-fan types ignore min value specified\n");
        }
    }

    /* The max field is optional in a configuration. */
    auto max = j.find("max");
    if (max != j.end())
    {
        if (s.type == "fan")
        {
            j.at("max").get_to(s.max);
        }
        else
        {
            std::fprintf(stderr, "Non-fan types ignore max value specified\n");
        }
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
} // namespace conf

std::map<std::string, conf::SensorConfig> buildSensorsFromJson(const json& data)
{
    std::map<std::string, conf::SensorConfig> config;
    auto sensors = data["sensors"];

    /* TODO: If no sensors, this is invalid, and we should except here or during
     * parsing.
     */
    for (const auto& sensor : sensors)
    {
        config[sensor["name"]] = sensor.get<conf::SensorConfig>();
    }

    return config;
}
} // namespace pid_control
