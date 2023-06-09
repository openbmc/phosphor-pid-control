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
                std::cout << "\n\t\t\t" << input << ",\n";
            }
            std::cout << "\t\t\t}\n";
            std::cout << "\t\t\t" << pidconf.second.setpoint << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.ts << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.proportionalCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.integralCoeff
                      << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.derivativeCoeff
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

} // namespace pid_control
