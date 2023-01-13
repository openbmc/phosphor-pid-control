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
#include "util.hpp"

#include <nlohmann/json.hpp>

#include <iostream>
#include <limits>
#include <map>
#include <tuple>

namespace pid_control
{

using json = nlohmann::json;

namespace conf
{

void from_json(const json& j, conf::ControllerInfo& c)
{
    std::vector<std::string> inputNames;

    j.at("type").get_to(c.type);
    j.at("inputs").get_to(inputNames);
    j.at("setpoint").get_to(c.setpoint);

    std::vector<double> inputTempToMargin;

    auto findTempToMargin = j.find("tempToMargin");
    if (findTempToMargin != j.end())
    {
        findTempToMargin->get_to(inputTempToMargin);
    }

    c.inputs = spliceInputs(inputNames, inputTempToMargin);

    /* TODO: We need to handle parsing other PID controller configurations.
     * We can do that by checking for different keys and making the decision
     * accordingly.
     */
    auto p = j.at("pid");

    auto positiveHysteresis = p.find("positiveHysteresis");
    auto negativeHysteresis = p.find("negativeHysteresis");
    auto derivativeCoeff = p.find("derivativeCoeff");
    auto positiveHysteresisValue = 0.0;
    auto negativeHysteresisValue = 0.0;
    auto derivativeCoeffValue = 0.0;
    if (positiveHysteresis != p.end())
    {
        positiveHysteresis->get_to(positiveHysteresisValue);
    }
    if (negativeHysteresis != p.end())
    {
        negativeHysteresis->get_to(negativeHysteresisValue);
    }
    if (derivativeCoeff != p.end())
    {
        derivativeCoeff->get_to(derivativeCoeffValue);
    }

    if (c.type != "stepwise")
    {
        p.at("samplePeriod").get_to(c.pidInfo.ts);
        p.at("proportionalCoeff").get_to(c.pidInfo.proportionalCoeff);
        p.at("integralCoeff").get_to(c.pidInfo.integralCoeff);
        p.at("feedFwdOffsetCoeff").get_to(c.pidInfo.feedFwdOffset);
        p.at("feedFwdGainCoeff").get_to(c.pidInfo.feedFwdGain);
        p.at("integralLimit_min").get_to(c.pidInfo.integralLimit.min);
        p.at("integralLimit_max").get_to(c.pidInfo.integralLimit.max);
        p.at("outLim_min").get_to(c.pidInfo.outLim.min);
        p.at("outLim_max").get_to(c.pidInfo.outLim.max);
        p.at("slewNeg").get_to(c.pidInfo.slewNeg);
        p.at("slewPos").get_to(c.pidInfo.slewPos);

        // Unlike other coefficients, treat derivativeCoeff as an optional
        // parameter, as support for it is fairly new, to avoid breaking
        // existing configurations in the field that predate it.
        c.pidInfo.positiveHysteresis = positiveHysteresisValue;
        c.pidInfo.negativeHysteresis = negativeHysteresisValue;
        c.pidInfo.derivativeCoeff = derivativeCoeffValue;
    }
    else
    {
        p.at("samplePeriod").get_to(c.stepwiseInfo.ts);
        p.at("isCeiling").get_to(c.stepwiseInfo.isCeiling);

        for (size_t i = 0; i < ec::maxStepwisePoints; i++)
        {
            c.stepwiseInfo.reading[i] =
                std::numeric_limits<double>::quiet_NaN();
            c.stepwiseInfo.output[i] = std::numeric_limits<double>::quiet_NaN();
        }

        auto reading = p.find("reading");
        if (reading != p.end())
        {
            auto r = p.at("reading");
            for (size_t i = 0; i < ec::maxStepwisePoints; i++)
            {
                auto n = r.find(std::to_string(i));
                if (n != r.end())
                {
                    r.at(std::to_string(i)).get_to(c.stepwiseInfo.reading[i]);
                }
            }
        }

        auto output = p.find("output");
        if (output != p.end())
        {
            auto o = p.at("output");
            for (size_t i = 0; i < ec::maxStepwisePoints; i++)
            {
                auto n = o.find(std::to_string(i));
                if (n != o.end())
                {
                    o.at(std::to_string(i)).get_to(c.stepwiseInfo.output[i]);
                }
            }
        }

        c.stepwiseInfo.positiveHysteresis = positiveHysteresisValue;
        c.stepwiseInfo.negativeHysteresis = negativeHysteresisValue;
    }
}

} // namespace conf

inline void getCycleTimeSetting(const auto& zone, const int id,
                                const std::string& attributeName,
                                uint64_t& value)
{
    auto findAttributeName = zone.find(attributeName);
    if (findAttributeName != zone.end())
    {
        uint64_t tmpAttributeValue = 0;
        findAttributeName->get_to(tmpAttributeValue);
        if (tmpAttributeValue >= 1)
        {
            value = tmpAttributeValue;
        }
        else
        {
            std::cerr << "Zone " << id << ": " << attributeName
                      << " is invalid. Use default " << value << " ms\n";
        }
    }
    else
    {
        std::cerr << "Zone " << id << ": " << attributeName
                  << " cannot find setting. Use default " << value << " ms\n";
    }
}

std::pair<std::map<int64_t, conf::PIDConf>, std::map<int64_t, conf::ZoneConfig>>
    buildPIDsFromJson(const json& data)
{
    // zone -> pids
    std::map<int64_t, conf::PIDConf> pidConfig;
    // zone -> configs
    std::map<int64_t, conf::ZoneConfig> zoneConfig;

    /* TODO: if zones is empty, that's invalid. */
    auto zones = data["zones"];
    for (const auto& zone : zones)
    {
        int64_t id;
        conf::PIDConf thisZone;
        conf::ZoneConfig thisZoneConfig;

        /* TODO: using at() throws a specific exception we can catch */
        id = zone["id"];
        thisZoneConfig.minThermalOutput = zone["minThermalOutput"];
        thisZoneConfig.failsafePercent = zone["failsafePercent"];

        getCycleTimeSetting(zone, id, "cycleIntervalTimeMS",
                            thisZoneConfig.cycleTime.cycleIntervalTimeMS);
        getCycleTimeSetting(zone, id, "updateThermalsTimeMS",
                            thisZoneConfig.cycleTime.updateThermalsTimeMS);

        auto pids = zone["pids"];
        for (const auto& pid : pids)
        {
            auto name = pid["name"];
            auto item = pid.get<conf::ControllerInfo>();

            thisZone[name] = item;
        }

        pidConfig[id] = thisZone;
        zoneConfig[id] = thisZoneConfig;
    }

    return std::make_pair(pidConfig, zoneConfig);
}

} // namespace pid_control
