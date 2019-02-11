/**
 * Copyright 2017 Google Inc.
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

#include "pid/builderconfig.hpp"

#include "conf.hpp"
#include "pid/builder.hpp"

#include <fstream>
#include <iostream>
#include <libconfig.h++>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>
#include <unordered_map>

std::unordered_map<int64_t, std::unique_ptr<PIDZone>>
    buildZonesFromConfig(const std::string& path, SensorManager& mgr,
                         sdbusplus::bus::bus& modeControlBus)
{
    using namespace libconfig;
    // zone -> pids
    std::map<int64_t, PIDConf> pidConfig;
    // zone -> configs
    std::map<int64_t, struct ZoneConfig> zoneConfig;

    std::cerr << "entered BuildZonesFromConfig\n";

    Config cfg;

    /* The load was modeled after the example source provided. */
    try
    {
        cfg.readFile(path.c_str());
    }
    catch (const FileIOException& fioex)
    {
        std::cerr << "I/O error while reading file: " << fioex.what()
                  << std::endl;
        throw;
    }
    catch (const ParseException& pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        throw;
    }

    try
    {
        const Setting& root = cfg.getRoot();
        const Setting& zones = root["zones"];
        int count = zones.getLength();

        /* For each zone. */
        for (int i = 0; i < count; ++i)
        {
            const Setting& zoneSettings = zones[i];

            int id;
            PIDConf thisZone;
            struct ZoneConfig thisZoneConfig;

            zoneSettings.lookupValue("id", id);

            thisZoneConfig.minThermalRpm = zoneSettings.lookup("minThermalRpm");
            thisZoneConfig.failsafePercent =
                zoneSettings.lookup("failsafePercent");

            const Setting& pids = zoneSettings["pids"];
            int pidCount = pids.getLength();

            for (int j = 0; j < pidCount; ++j)
            {
                const Setting& pid = pids[j];

                std::string name;
                struct ControllerInfo info;

                /*
                 * Mysteriously if you use lookupValue on these, and the type
                 * is double.  It won't work right.
                 *
                 * If the configuration file value doesn't look explicitly like
                 * a double it won't let you assign it to one.
                 */
                name = pid.lookup("name").c_str();
                info.type = pid.lookup("type").c_str();
                /* set-point is only required to be set for thermal. */
                /* TODO(venture): Verify this works optionally here. */
                info.setpoint = pid.lookup("set-point");
                info.pidInfo.ts = pid.lookup("pid.samplePeriod");
                info.pidInfo.proportionalCoeff =
                    pid.lookup("pid.proportionalCoeff");
                info.pidInfo.integralCoeff = pid.lookup("pid.integralCoeff");
                info.pidInfo.feedFwdOffset =
                    pid.lookup("pid.feedFwdOffOffsetCoeff");
                info.pidInfo.feedFwdGain = pid.lookup("pid.feedFwdGainCoeff");
                info.pidInfo.integralLimit.min =
                    pid.lookup("pid.integralCoeff.min");
                info.pidInfo.integralLimit.max =
                    pid.lookup("pid.integralCoeff.max");
                info.pidInfo.outLim.min = pid.lookup("pid.outLimit.min");
                info.pidInfo.outLim.max = pid.lookup("pid.outLimit.max");
                info.pidInfo.slewNeg = pid.lookup("pid.slewNeg");
                info.pidInfo.slewPos = pid.lookup("pid.slewPos");

                std::cerr << "outLim.min: " << info.pidInfo.outLim.min << "\n";
                std::cerr << "outLim.max: " << info.pidInfo.outLim.max << "\n";

                const Setting& inputs = pid["inputs"];
                int icount = inputs.getLength();

                for (int z = 0; z < icount; ++z)
                {
                    std::string v;
                    v = pid["inputs"][z].c_str();
                    info.inputs.push_back(v);
                }

                thisZone[name] = info;
            }

            pidConfig[static_cast<int64_t>(id)] = thisZone;
            zoneConfig[static_cast<int64_t>(id)] = thisZoneConfig;
        }
    }
    catch (const SettingTypeException& setex)
    {
        std::cerr << "Setting '" << setex.getPath() << "' type exception!"
                  << std::endl;
        throw;
    }
    catch (const SettingNotFoundException& snex)
    {
        std::cerr << "Setting '" << snex.getPath() << "' not found!"
                  << std::endl;
        throw;
    }

    return buildZones(pidConfig, zoneConfig, mgr, modeControlBus);
}
