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

#include <chrono>
#include <experimental/any>
#include <getopt.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex> /* not yet used. */
#include <thread>
#include <vector>

#include <sdbusplus/bus.hpp>

/* Configuration. */
#include "conf.hpp"

/* Misc. */
#include "util.hpp"

/* Controllers & Sensors. */
#include "interfaces.hpp"
#include "pid/zone.hpp"
#include "sensors/builder.hpp"
#include "sensors/builderconfig.hpp"
#include "sensors/manager.hpp"

/* Threads. */
#include "pid/pidthread.hpp"
#include "threads/busthread.hpp"


/* The YAML converted sensor list. */
extern std::map<std::string, struct sensor> SensorConfig;
/* The YAML converted PID list. */
extern std::map<int64_t, PIDConf> ZoneConfig;
/* The YAML converted Zone configuration. */
extern std::map<int64_t, struct zone> ZoneDetailsConfig;

int main(int argc, char* argv[])
{
    int rc = 0;
    int c;
    std::string configPath = "";

    while (1)
    {
        static struct option long_options[] =
        {
            {"conf", required_argument, 0, 'c'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "c:", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'c':
                configPath = std::string {optarg};
                break;
            default:
                /* skip garbage. */
                continue;
        }
    }

    auto ModeControlBus = sdbusplus::bus::new_default();
    SensorManager mgmr;
    std::map<int64_t, std::shared_ptr<PIDZone>> zones;

    // Create a manger for the ModeBus because we own it.
    static constexpr auto modeRoot = "/xyz/openbmc_project/settings/fanctrl";
    sdbusplus::server::manager::manager(ModeControlBus, modeRoot);

    /*
     * When building the sensors, if any of the dbus passive ones aren't on the
     * bus, it'll fail immediately.
     */
    if (configPath.length() > 0)
    {
        try
        {
            mgmr = BuildSensorsFromConfig(configPath);
            zones = BuildZonesFromConfig(configPath, mgmr, ModeControlBus);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed during building: " << e.what() << "\n";
            exit(EXIT_FAILURE); /* fatal error. */
        }
    }
    else
    {
        mgmr = BuildSensors(SensorConfig);
        zones = BuildZones(ZoneConfig, ZoneDetailsConfig, mgmr, ModeControlBus);
    }

    if (0 == zones.size())
    {
        std::cerr << "No zones defined, exiting.\n";
        return rc;
    }

    /*
     * All sensors are managed by one manager, but each zone has a pointer to
     * it.
     */

    auto& HostSensorBus = mgmr.getHostBus();
    auto& PassiveListeningBus = mgmr.getPassiveBus();

    std::cerr << "Starting threads\n";

    /* TODO(venture): Ask SensorManager if we have any passive sensors. */
    struct ThreadParams p =
    {
        std::ref(PassiveListeningBus),
        ""
    };
    std::thread l(BusThread, std::ref(p));

    /* TODO(venture): Ask SensorManager if we have any host sensors. */
    static constexpr auto hostBus = "xyz.openbmc_project.Hwmon.external";
    struct ThreadParams e =
    {
        std::ref(HostSensorBus),
        hostBus
    };
    std::thread te(BusThread, std::ref(e));

    static constexpr auto modeBus = "xyz.openbmc_project.State.FanCtrl";
    struct ThreadParams m =
    {
        std::ref(ModeControlBus),
        modeBus
    };
    std::thread tm(BusThread, std::ref(m));

    std::vector<std::thread> zoneThreads;

    /* TODO(venture): This was designed to have one thread per zone, but really
     * it could have one thread for all the zones and iterate through each
     * sequentially as it goes -- and it'd probably be fast enough to do that,
     * however, a system isn't likely going to have more than a couple zones.
     * If it only has a couple zones, then this is fine.
     */
    for (auto& i : zones)
    {
        std::cerr << "pushing zone" << std::endl;
        zoneThreads.push_back(std::thread(PIDControlThread, i.second));
    }

    l.join();
    te.join();
    tm.join();
    for (auto& t : zoneThreads)
    {
        t.join();
    }

    return rc;
}

