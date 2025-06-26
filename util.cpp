/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include "util.hpp"

#include "conf.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

namespace pid_control
{

void debugPrint(const std::map<std::string, conf::SensorConfig>& sensorConfig,
                const std::map<int64_t, conf::PIDConf>& zoneConfig,
                const std::map<int64_t, conf::ZoneConfig>& zoneDetailsConfig)
{
    if constexpr (!conf::DEBUG)
    {
        return;
    }
    // print sensor config
    std::cout << "sensor config:\n";
    std::cout << "{\n";
    for (const auto& pair : sensorConfig)
    {
        std::cout << "\t{" << pair.first << ",\n\t\t{";
        std::cout << pair.second.type << ", ";
        std::cout << pair.second.readPath << ", ";
        std::cout << pair.second.writePath << ", ";
        std::cout << pair.second.min << ", ";
        std::cout << pair.second.max << ", ";
        std::cout << pair.second.timeout << ", ";
        std::cout << pair.second.unavailableAsFailed << "},\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneDetailsConfig\n";
    std::cout << "{\n";
    for (const auto& zone : zoneDetailsConfig)
    {
        std::cout << "\t{" << zone.first << ",\n";
        std::cout << "\t\t{" << zone.second.minThermalOutput << ", ";
        std::cout << zone.second.failsafePercent << "}\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneConfig\n";
    std::cout << "{\n";
    for (const auto& zone : zoneConfig)
    {
        std::cout << "\t{" << zone.first << "\n";
        for (const auto& pidconf : zone.second)
        {
            std::cout << "\t\t{" << pidconf.first << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.type << ",\n";
            std::cout << "\t\t\t{";
            for (const auto& input : pidconf.second.inputs)
            {
                std::cout << "\n\t\t\t" << input.name;
                if (input.convertTempToMargin)
                {
                    std::cout << "[" << input.convertMarginZero << "]";
                }
                std::cout << ",\n";
            }
            std::cout << "\t\t\t}\n";
            std::cout << "\t\t\t" << pidconf.second.setpoint << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.ts << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.proportionalCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.integralCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.feedFwdOffset
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.feedFwdGain
                      << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.integralLimit.min
                      << "," << pidconf.second.pidInfo.integralLimit.max
                      << "},\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.outLim.min << ","
                      << pidconf.second.pidInfo.outLim.max << "},\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slewNeg << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slewPos << ",\n";
            std::cout << "\t\t\t}\n\t\t}\n";
        }
        std::cout << "\t},\n";
    }
    std::cout << "}\n\n";
}

std::vector<conf::SensorInput> spliceInputs(
    const std::vector<std::string>& inputNames,
    const std::vector<double>& inputTempToMargin,
    const std::vector<std::string>& missingAcceptableNames)
{
    std::vector<conf::SensorInput> results;

    // Default to TempToMargin and MissingIsAcceptable disabled
    for (const auto& inputName : inputNames)
    {
        conf::SensorInput newInput{
            inputName, std::numeric_limits<double>::quiet_NaN(), false, false};

        results.emplace_back(newInput);
    }

    size_t resultSize = results.size();
    size_t marginSize = inputTempToMargin.size();

    for (size_t index = 0; index < resultSize; ++index)
    {
        // If fewer doubles than strings, and vice versa, ignore remainder
        if (index >= marginSize)
        {
            break;
        }

        // Both vectors have this index, combine both into SensorInput
        results[index].convertMarginZero = inputTempToMargin[index];
        results[index].convertTempToMargin = true;
    }

    std::set<std::string> acceptableSet;

    // Copy vector to set, to avoid O(n^2) runtime below
    for (const auto& name : missingAcceptableNames)
    {
        acceptableSet.emplace(name);
    }

    // Flag missingIsAcceptable true if name found in that set
    for (auto& result : results)
    {
        if (acceptableSet.find(result.name) != acceptableSet.end())
        {
            result.missingIsAcceptable = true;
        }
    }

    return results;
}

std::vector<std::string> splitNames(
    const std::vector<conf::SensorInput>& sensorInputs)
{
    std::vector<std::string> results;

    results.reserve(sensorInputs.size());
    for (const auto& sensorInput : sensorInputs)
    {
        results.emplace_back(sensorInput.name);
    }

    return results;
}

} // namespace pid_control
