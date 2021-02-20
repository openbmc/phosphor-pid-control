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

#include "config.h"

#include "build/buildjson.hpp"
#include "conf.hpp"
#include "dbus/dbusconfiguration.hpp"
#include "interfaces.hpp"
#include "pid/builder.hpp"
#include "pid/buildjson.hpp"
#include "pid/pidloop.hpp"
#include "pid/tuning.hpp"
#include "pid/zone.hpp"
#include "sensors/builder.hpp"
#include "sensors/buildjson.hpp"
#include "sensors/manager.hpp"
#include "util.hpp"

#include <CLI/CLI.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pid_control
{

/* The configuration converted sensor list. */
std::map<std::string, conf::SensorConfig> sensorConfig = {};
/* The configuration converted PID list. */
std::map<int64_t, conf::PIDConf> zoneConfig = {};
/* The configuration converted Zone configuration. */
std::map<int64_t, conf::ZoneConfig> zoneDetailsConfig = {};

} // namespace pid_control

/** the swampd daemon will check for the existence of this file. */
constexpr auto jsonConfigurationPath = "/usr/share/swampd/config.json";
std::string configPath = "";

/* async io context for operation */
boost::asio::io_context io;

/* buses for system control */
static sdbusplus::asio::connection modeControlBus(io);
static sdbusplus::asio::connection
    hostBus(io, sdbusplus::bus::new_system().release());
static sdbusplus::asio::connection
    passiveBus(io, sdbusplus::bus::new_system().release());

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

    if (zones.size() > 0 && zones.begin()->second.use_count() > 1)
    {
        throw std::runtime_error("wait for count back to 1");
    }
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
        if (!dbus_configuration::init(modeControlBus, reloadTimer, sensorConfig,
                                      zoneConfig, zoneDetailsConfig))
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

void tryRestartControlLoops(bool first)
{
    static int count = 0;
    static const auto initialStartTime = std::chrono::milliseconds(10);
    static const auto delayTeime = std::chrono::seconds(10);
    static boost::asio::steady_timer timer(io);
    // try to start a control loop while the loop has been scheduled.
    if (first && count != 0)
    {
        std::cerr
            << "ControlLoops has been scheduled, refresh the loop count\n";
        count = 1;
        return;
    }
    // first time of trying to restart the control loop, delay for a small
    // amount of time.
    else if (first)
    {
        timer.expires_after(initialStartTime);
    }
    // re-try control loop, set up a delay.
    else
    {
        timer.expires_after(delayTeime);
    }
    count++;
    timer.async_wait(
        [](const boost::system::error_code& error) {
            if (error == boost::asio::error::operation_aborted)
            {
                return;
            }
            try
            {
                restartControlLoops();
                // reset count after succesful restartControlLoops()
                count = 0;
            }
            catch (const std::exception& e)
            {
                if (count >= 5)
                {
                    std::cerr
                        << "Failed to restartControlLoops after multiple tries"
                        << std::endl;
                    throw std::runtime_error(e.what());
                }

                std::cerr << count
                          << " Failed during restartControlLoops, try again: "
                          << e.what() << "\n";
                tryRestartControlLoops(false);
            }
        });

    return;
}

} // namespace pid_control

int main(int argc, char* argv[])
{
    loggingPath = "";
    loggingEnabled = false;
    tuningEnabled = false;

    CLI::App app{"OpenBMC Fan Control Daemon"};

    app.add_option("-c,--conf", configPath,
                   "Optional parameter to specify configuration at run-time")
        ->check(CLI::ExistingFile);
    app.add_option("-l,--log", loggingPath,
                   "Optional parameter to specify logging folder")
        ->check(CLI::ExistingDirectory);
    app.add_flag("-t,--tuning", tuningEnabled, "Enable or disable tuning");

    CLI11_PARSE(app, argc, argv);

    static constexpr auto loggingEnablePath = "/etc/thermal.d/logging";
    static constexpr auto tuningEnablePath = "/etc/thermal.d/tuning";

    // If this file exists, enable logging at runtime
    std::ifstream fsLogging(loggingEnablePath);
    if (fsLogging)
    {
        // Unless file contents are a valid directory path, use system default
        std::getline(fsLogging, loggingPath);
        if (!(std::filesystem::exists(loggingPath)))
        {
            loggingPath = std::filesystem::temp_directory_path();
        }
        fsLogging.close();

        loggingEnabled = true;
        std::cerr << "Logging enabled: " << loggingPath << "\n";
    }

    // If this file exists, enable tuning at runtime
    if (std::filesystem::exists(tuningEnablePath))
    {
        tuningEnabled = true;
        std::cerr << "Tuning enabled\n";
    }

    static constexpr auto modeRoot = "/xyz/openbmc_project/settings/fanctrl";
    // Create a manager for the ModeBus because we own it.
    sdbusplus::server::manager::manager(
        static_cast<sdbusplus::bus::bus&>(modeControlBus), modeRoot);
    hostBus.request_name("xyz.openbmc_project.Hwmon.external");
    modeControlBus.request_name("xyz.openbmc_project.State.FanCtrl");

    /*
     * All sensors are managed by one manager, but each zone has a pointer to
     * it.
     */

    pid_control::tryRestartControlLoops();

    io.run();
    return 0;
}
