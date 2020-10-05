/**
 * Copyright 2020 Google Inc.
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

#include "control.hpp"

#include <filesystem>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

namespace pid_control
{

void restartControlLoops()
{
    static SensorManager mgmr;
    static std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> zones;
    static std::vector<std::shared_ptr<boost::asio::steady_timer>> timers;

    for (const auto timer : timers)
    {
        timer->cancel();
    }

    timers.clear();
    zones.clear();

    const std::string& path =
        (configPath.length() > 0) ? configPath : jsonConfigurationPath;

    if (std::filesystem::exists(path))
    {
        /*
         * When building the sensors, if any of the dbus passive ones aren't on
         * the bus, it'll fail immediately.
         */
        try
        {
            auto jsonData = parseValidateJson(path);
            sensorConfig = buildSensorsFromJson(jsonData);
            std::tie(zoneConfig, zoneDetailsConfig) =
                buildPIDsFromJson(jsonData);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed during building: " << e.what() << "\n";
            exit(EXIT_FAILURE); /* fatal error. */
        }
    }
    else
    {
        static boost::asio::steady_timer reloadTimer(io);
        if (!dbus_configuration::init(modeControlBus, reloadTimer))
        {
            return; // configuration not ready
        }
    }

    mgmr = buildSensors(sensorConfig, passiveBus, hostBus);
    zones = buildZones(zoneConfig, zoneDetailsConfig, mgmr, modeControlBus);

    if (0 == zones.size())
    {
        std::cerr << "No zones defined, exiting.\n";
        std::exit(EXIT_FAILURE);
    }

    for (const auto& i : zones)
    {
        std::shared_ptr<boost::asio::steady_timer> timer = timers.emplace_back(
            std::make_shared<boost::asio::steady_timer>(io));
        std::cerr << "pushing zone " << i.first << "\n";
        pidControlLoop(i.second, timer);
    }
}

void tryRestartControlLoops()
{
    int count = 0;
    for (count = 0; count <= 5; count++)
    {
        try
        {
            restartControlLoops();
            break;
        }
        catch (const std::exception& e)
        {
            std::cerr << count
                      << " Failed during restartControlLoops, try again: "
                      << e.what() << "\n";
            if (count >= 5)
            {
                throw std::runtime_error(e.what());
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return;
}

}
