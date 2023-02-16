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

#include "buildjson/buildjson.hpp"
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
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>

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

using path = std::filesystem::path;

namespace pid_control
{

/* The configuration converted sensor list. */
std::map<std::string, conf::SensorConfig> sensorConfig = {};
/* The configuration converted PID list. */
std::map<int64_t, conf::PIDConf> zoneConfig = {};
/* The configuration converted Zone configuration. */
std::map<int64_t, conf::ZoneConfig> zoneDetailsConfig = {};

} // namespace pid_control

std::string configPath = "";

/* async io context for operation */
boost::asio::io_context io;
/* async signal_set for signal handling */
boost::asio::signal_set signals(io, SIGHUP);

/* buses for system control */
static sdbusplus::asio::connection modeControlBus(io);
static sdbusplus::asio::connection
    hostBus(io, sdbusplus::bus::new_system().release());
static sdbusplus::asio::connection
    passiveBus(io, sdbusplus::bus::new_system().release());

namespace pid_control
{

path searchConfigurationPath()
{
    static constexpr auto name = "config.json";

    for (auto pathSeg : {std::filesystem::current_path(),
                         path{"/var/lib/swampd"}, path{"/usr/share/swampd"}})
    {
        auto file = pathSeg / name;
        if (std::filesystem::exists(file))
        {
            return file;
        }
    }

    return name;
}

void restartControlLoops()
{
    static SensorManager mgmr;
    static std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> zones;
    static std::vector<std::shared_ptr<boost::asio::steady_timer>> timers;
    static bool isCanceling = false;

    for (const auto& timer : timers)
    {
        timer->cancel();
    }
    isCanceling = true;
    timers.clear();

    if (zones.size() > 0 && zones.begin()->second.use_count() > 1)
    {
        throw std::runtime_error("wait for count back to 1");
    }
    zones.clear();
    isCanceling = false;

    const std::string& path = (configPath.length() > 0)
                                  ? configPath
                                  : searchConfigurationPath().string();

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
        pidControlLoop(i.second, timer, &isCanceling);
    }
}

void tryRestartControlLoops(bool first)
{
    static int count = 0;
    static const auto delayTime = std::chrono::seconds(10);
    static boost::asio::steady_timer timer(io);
    // try to start a control loop while the loop has been scheduled.
    if (first && count != 0)
    {
        std::cerr
            << "ControlLoops has been scheduled, refresh the loop count\n";
        count = 1;
        return;
    }

    auto restartLbd = [](const boost::system::error_code& error) {
        if (error == boost::asio::error::operation_aborted)
        {
            return;
        }

        // for the last loop, don't elminate the failure of restartControlLoops.
        if (count >= 5)
        {
            restartControlLoops();
            // reset count after succesful restartControlLoops()
            count = 0;
            return;
        }

        // retry when restartControlLoops() has some failure.
        try
        {
            restartControlLoops();
            // reset count after succesful restartControlLoops()
            count = 0;
        }
        catch (const std::exception& e)
        {
            std::cerr << count
                      << " Failed during restartControlLoops, try again: "
                      << e.what() << "\n";
            tryRestartControlLoops(false);
        }
    };
    count++;
    // first time of trying to restart the control loop without a delay
    if (first)
    {
        boost::asio::post(io,
                          std::bind(restartLbd, boost::system::error_code()));
    }
    // re-try control loop, set up a delay.
    else
    {
        timer.expires_after(delayTime);
        timer.async_wait(restartLbd);
    }

    return;
}

} // namespace pid_control

void sighupHandler(const boost::system::error_code& error, int signal_number)
{
    static boost::asio::steady_timer timer(io);

    if (error)
    {
        std::cout << "Signal " << signal_number
                  << " handler error: " << error.message() << "\n";
        return;
    }

    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait([](const boost::system::error_code ec) {
        if (ec)
        {
            std::cout << "Signal timer error: " << ec.message() << "\n";
            return;
        }

        std::cout << "reloading configuration\n";
        pid_control::tryRestartControlLoops();
    });
    signals.async_wait(sighupHandler);
}

int main(int argc, char* argv[])
{
    loggingPath = "";
    loggingEnabled = false;
    tuningEnabled = false;
    debugEnabled = false;
    coreLoggingEnabled = false;

    CLI::App app{"OpenBMC Fan Control Daemon"};

    app.add_option("-c,--conf", configPath,
                   "Optional parameter to specify configuration at run-time")
        ->check(CLI::ExistingFile);
    app.add_option("-l,--log", loggingPath,
                   "Optional parameter to specify logging folder")
        ->check(CLI::ExistingDirectory);
    app.add_flag("-t,--tuning", tuningEnabled, "Enable or disable tuning");
    app.add_flag("-d,--debug", debugEnabled, "Enable or disable debug mode");
    app.add_flag("-g,--corelogging", coreLoggingEnabled,
                 "Enable or disable logging of core PID loop computations");

    CLI11_PARSE(app, argc, argv);

    static constexpr auto loggingEnablePath = "/etc/thermal.d/logging";
    static constexpr auto tuningEnablePath = "/etc/thermal.d/tuning";
    static constexpr auto debugEnablePath = "/etc/thermal.d/debugging";
    static constexpr auto coreLoggingEnablePath = "/etc/thermal.d/corelogging";

    // Set up default logging path, preferring command line if it was given
    std::string defLoggingPath(loggingPath);
    if (defLoggingPath.empty())
    {
        defLoggingPath = std::filesystem::temp_directory_path();
    }
    else
    {
        // Enable logging, if user explicitly gave path on command line
        loggingEnabled = true;
    }

    // If this file exists, enable logging at runtime
    std::ifstream fsLogging(loggingEnablePath);
    if (fsLogging)
    {
        // Allow logging path to be changed by file content
        std::string altPath;
        std::getline(fsLogging, altPath);
        fsLogging.close();

        if (std::filesystem::exists(altPath))
        {
            loggingPath = altPath;
        }

        loggingEnabled = true;
    }
    if (loggingEnabled)
    {
        std::cerr << "Logging enabled: " << loggingPath << "\n";
    }

    // If this file exists, enable tuning at runtime
    if (std::filesystem::exists(tuningEnablePath))
    {
        tuningEnabled = true;
    }
    if (tuningEnabled)
    {
        std::cerr << "Tuning enabled\n";
    }

    // If this file exists, enable debug mode at runtime
    if (std::filesystem::exists(debugEnablePath))
    {
        debugEnabled = true;
    }

    if (debugEnabled)
    {
        std::cerr << "Debug mode enabled\n";
    }

    // If this file exists, enable core logging at runtime
    if (std::filesystem::exists(coreLoggingEnablePath))
    {
        coreLoggingEnabled = true;
    }
    if (coreLoggingEnabled)
    {
        std::cerr << "Core logging enabled\n";
    }

    static constexpr auto modeRoot = "/xyz/openbmc_project/settings/fanctrl";
    // Create a manager for the ModeBus because we own it.
    sdbusplus::server::manager_t(static_cast<sdbusplus::bus_t&>(modeControlBus),
                                 modeRoot);
    hostBus.request_name("xyz.openbmc_project.Hwmon.external");
    modeControlBus.request_name("xyz.openbmc_project.State.FanCtrl");
    sdbusplus::server::manager_t objManager(modeControlBus, modeRoot);

    // Enable SIGHUP handling to reload JSON config
    signals.async_wait(sighupHandler);

    /*
     * All sensors are managed by one manager, but each zone has a pointer to
     * it.
     */

    pid_control::tryRestartControlLoops();

    io.run();
    return 0;
}
