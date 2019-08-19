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

#include <systemd/sd-journal.h>

#include <CLI/CLI.hpp>
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/vtable.hpp>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#if CONFIGURE_DBUS
#include "dbus/dbusconfiguration.hpp"
#endif

/* The configuration converted sensor list. */
std::map<std::string, struct conf::SensorConfig> sensorConfig = {};
/* The configuration converted PID list. */
std::map<int64_t, conf::PIDConf> zoneConfig = {};
/* The configuration converted Zone configuration. */
std::map<int64_t, struct conf::ZoneConfig> zoneDetailsConfig = {};

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

// Setup debug mode dbus objectPath/interface name.
static constexpr auto debugModeObjPath =
    "/xyz/openbmc_project/settings/fanctrl/debug";
static constexpr auto debugModeIntfName =
    "xyz.openbmc_project.Control.DebugMode";

// Register debug mode signal handler
void registerDebugSignalHandler()
{
    char signalSetting[512];

    memset(signalSetting, 0, sizeof(signalSetting));
    snprintf(signalSetting, sizeof(signalSetting),
             "type='signal',interface='%s',member='Open',path='%s'",
             debugModeIntfName, debugModeObjPath);
    static sdbusplus::bus::match::match openDebugMode(
        modeControlBus, signalSetting,
        [](sdbusplus::message::message& message) { debugModeEnabled = true; });

    memset(signalSetting, 0, sizeof(signalSetting));
    snprintf(signalSetting, sizeof(signalSetting),
             "type='signal',interface='%s',member='Close',path='%s'",
             debugModeIntfName, debugModeObjPath);
    static sdbusplus::bus::match::match closeDebugMode(
        modeControlBus, signalSetting,
        [](sdbusplus::message::message& message) { debugModeEnabled = false; });

    return;
}
void restartControlLoops()
{
    static SensorManager mgmr;
    static std::unordered_map<int64_t, std::unique_ptr<PIDZone>> zones;
    static std::list<boost::asio::steady_timer> timers;

    timers.clear();

#if CONFIGURE_DBUS

    static boost::asio::steady_timer reloadTimer(io);
    if (!dbus_configuration::init(modeControlBus, reloadTimer))
    {
        return; // configuration not ready
    }

#else
    const std::string& path =
        (configPath.length() > 0) ? configPath : jsonConfigurationPath;
    sd_journal_print(LOG_INFO, "json file path: %s", path.c_str());

    /*
     * When building the sensors, if any of the dbus passive ones aren't on the
     * bus, it'll fail immediately.
     */
    try
    {
        auto jsonData = parseValidateJson(path);
        sensorConfig = buildSensorsFromJson(jsonData);
        std::tie(zoneConfig, zoneDetailsConfig) = buildPIDsFromJson(jsonData);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed during building: " << e.what() << "\n";
        exit(EXIT_FAILURE); /* fatal error. */
    }
#endif

    mgmr = buildSensors(sensorConfig, passiveBus, hostBus);

    /**
     *  This loop is used to modify fan table config before building zones.
     *  Because there may have some errors when doing buildSensors.
     *  Or sensors have Tjmax value, some temeratures have to be adjusted.
     **/
    for (auto& zone : zoneConfig)
    {
        conf::PIDConf* pids = &zone.second;
        // For all profiles
        for (auto& pid : *pids)
        {
            struct conf::ControllerInfo* info = &pid.second;
            // To avoid adjust values twice.
            bool tjMaxAdjusted = false;
            for (size_t i = 0; i < info->inputs.size(); ++i)
            {
                /**
                 *  If sensor is not created and place in mgmr then,
                 *  getSensor throw a out_of_range error from map.at().
                 *  Because this sensor name is not found from the map.
                 *  To ensure pid process operate as usual,
                 *  erase this sensor name from pid inputs.
                 **/
                try
                {
                    auto sensor = mgmr.getSensor(info->inputs[i]);

                    /**
                     *  If tjMax is defined, adjust setpoint value and
                     *  stepwise reading point.
                     **/
                    double tjMax = sensor->getTjMax();
                    if ((tjMax != 0) && (tjMaxAdjusted == false))
                    {
                        tjMaxAdjusted = true;
                        info->setpoint = tjMax + info->setpoint;

                        if ((info->type == "stepwise") ||
                            (info->type == "linear"))
                        {
                            for (auto& point : info->stepwiseInfo.reading)
                            {
                                point = tjMax + point;
                            }
                        }
                    }
                }
                catch (const std::out_of_range& oor)
                {
                    info->inputs.erase(info->inputs.begin() + i);
                    /**
                     * To keep loop working correctly, index has to be reduced
                     * by one when erasing an element of a vecotr in the loop.
                     **/
                    i = i - 1;
                }
            }
        }
    }

    zones = buildZones(zoneConfig, zoneDetailsConfig, mgmr, modeControlBus);

    if (0 == zones.size())
    {
        std::cerr << "No zones defined, exiting.\n";
        std::exit(EXIT_FAILURE);
    }

    for (const auto& i : zones)
    {
        auto& timer = timers.emplace_back(io);
        std::cerr << "pushing zone " << i.first << "\n";
        pidControlLoop(i.second.get(), timer);
    }
}

// If process receive sigint/sigterm write 60% pwm to all pwm files.
void writePwmFailsafeHandler(int signum)
{
    std::string sigStr;
    switch (signum)
    {
        case SIGINT:
            sigStr = "SIGINT";
            break;
        case SIGTERM:
            sigStr = "SIGTERM";
            break;
        default:
            // Only register SIGINT/SIGTERM, process should not reach here.
            sigStr = "UNKNOWN";
            break;
    }
    sd_journal_print(LOG_INFO,
                     "Fan control receive %s. Try to "
                     "write 60%% pwm to all pwm files.\n",
                     sigStr.c_str());

    std::string cmd = "/usr/sbin/writePwmFailsafe.sh";
    std::array<char, 1024> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipePtr(popen(cmd.c_str(), "r"),
                                                     pclose);
    if (pipePtr == nullptr)
    {
        sd_journal_print(LOG_ERR, "Fan control failed to create popen. "
                                  "Failed to write 60%% pwm to pwm files.\n");
    }
    else
    {
        while (fgets(buffer.data(), buffer.size(), pipePtr.get()) != nullptr)
        {
            sd_journal_print(LOG_INFO, "%s", buffer.data());
        }
        if (ferror(pipePtr.get()))
        {
            sd_journal_print(LOG_ERR,
                             "Fan control failed to fgets. Can't not get "
                             "return string from write pwm script.\n");
        }
    }

    exit(signum);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, writePwmFailsafeHandler);
    signal(SIGTERM, writePwmFailsafeHandler);

    loggingPath = "";
    loggingEnabled = false;
    tuningEnabled = false;
    debugModeEnabled = false;

    CLI::App app{"OpenBMC Fan Control Daemon"};

    app.add_option("-c,--conf", configPath,
                   "Optional parameter to specify configuration at run-time")
        ->check(CLI::ExistingFile);
    app.add_option("-l,--log", loggingPath,
                   "Optional parameter to specify logging folder")
        ->check(CLI::ExistingDirectory);
    app.add_flag("-t,--tuning", tuningEnabled, "Enable or disable tuning");

    CLI11_PARSE(app, argc, argv);

    loggingEnabled = (!loggingPath.empty());

    static constexpr auto modeRoot = "/xyz/openbmc_project/settings/fanctrl";
    // Create a manager for the ModeBus because we own it.
    sdbusplus::server::manager::manager(
        static_cast<sdbusplus::bus::bus&>(modeControlBus), modeRoot);
    hostBus.request_name("xyz.openbmc_project.Hwmon.external");
    modeControlBus.request_name("xyz.openbmc_project.State.FanCtrl");

    // Create debug/manual mode object.
    std::shared_ptr<sdbusplus::asio::connection> modeCtrlPtr(&modeControlBus);
    sdbusplus::asio::object_server modeCtrlServer(modeCtrlPtr);
    modeCtrlServer.add_interface(debugModeObjPath, debugModeIntfName);

    // Create debug/manual mode signal
    const sd_bus_vtable modeVtable[] = {
        sdbusplus::vtable::start(), sdbusplus::vtable::signal("Open", "", 0),
        sdbusplus::vtable::signal("Close", "", 0), sdbusplus::vtable::end()};
    sdbusplus::server::interface::interface createDebugModeVtable(
        static_cast<sdbusplus::bus::bus&>(modeControlBus), debugModeObjPath,
        debugModeIntfName, modeVtable, NULL);

    registerDebugSignalHandler();

    /*
     * All sensors are managed by one manager, but each zone has a pointer to
     * it.
     */

    restartControlLoops();

    io.run();
    return 0;
}
