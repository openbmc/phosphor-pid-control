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

#include <iostream>
#include <libconfig.h++>
#include <string>
#include <unordered_map>

/* Configuration. */
#include "conf.hpp"

#include "sensors/builder.hpp"
#include "sensors/manager.hpp"

/*
 * If there's a configuration file, we build from that, and it requires special
 * parsing.  I should just ditch the compile-time version to reduce the
 * probability of sync bugs.
 */
SensorManager BuildSensorsFromConfig(const std::string& path)
{
    using namespace libconfig;

    std::map<std::string, struct sensor> config;
    Config cfg;

    std::cerr << "entered BuildSensorsFromConfig\n";

    /* The load was modeled after the example source provided. */
    try
    {
        cfg.readFile(path.c_str());
    }
    catch (const FileIOException& fioex)
    {
        std::cerr << "I/O error while reading file: " << fioex.what() << std::endl;
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

        /* Grab the list of sensors and create them all */
        const Setting& sensors = root["sensors"];
        int count = sensors.getLength();

        for (int i = 0; i < count; ++i)
        {
            const Setting& sensor = sensors[i];

            std::string name;
            struct sensor thisOne;

            /* Not a super fan of using this library for run-time configuration. */
            name = sensor.lookup("name").c_str();
            thisOne.type = sensor.lookup("type").c_str();
            thisOne.readpath = sensor.lookup("readpath").c_str();
            thisOne.writepath = sensor.lookup("writepath").c_str();

            /* TODO: Document why this is wonky.  The library probably doesn't
             * like int64_t
             */
            int min = sensor.lookup("min");
            thisOne.min = static_cast<int64_t>(min);
            int max = sensor.lookup("max");
            thisOne.max = static_cast<int64_t>(max);
            int timeout = sensor.lookup("timeout");
            thisOne.timeout = static_cast<int64_t>(timeout);

            // leaving for verification for now.  and yea the above is
            // necessary.
            std::cerr << "min: " << min
                    << " max: " << max
                    << " savedmin: " << thisOne.min
                    << " savedmax: " << thisOne.max
                    << " timeout: " << thisOne.timeout
                    << std::endl;

            config[name] = thisOne;
        }
    }
    catch (const SettingTypeException &setex)
    {
        std::cerr << "Setting '" << setex.getPath()
                  << "' type exception!" << std::endl;
        throw;
    }
    catch (const SettingNotFoundException& snex)
    {
        std::cerr << "Setting not found!" << std::endl;
        throw;
    }

    return BuildSensors(config);
}

