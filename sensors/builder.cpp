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
#include <map>
#include <memory>
#include <string>
#include <utility>

/* Configuration. */
#include "conf.hpp"
#include "dbus/dbushelper.hpp"
#include "dbus/dbuspassive.hpp"
#include "dbus/dbuswrite.hpp"
#include "dbuspassiveredundancy.hpp"
#include "errors/exception.hpp"
#include "interfaces.hpp"
#include "notimpl/readonly.hpp"
#include "notimpl/writeonly.hpp"
#include "sensors/build_utils.hpp"
#include "sensors/builder.hpp"
#include "sensors/host.hpp"
#include "sensors/manager.hpp"
#include "sensors/pluggable.hpp"
#include "sysfs/sysfsread.hpp"
#include "sysfs/sysfswrite.hpp"

#include <sdbusplus/bus.hpp>

namespace pid_control
{

static constexpr bool deferSignals = true;

SensorManager buildSensors(
    const std::map<std::string, conf::SensorConfig>& config,
    sdbusplus::bus_t& passive, sdbusplus::bus_t& host)
{
    SensorManager mgmr{passive, host};
    auto& hostSensorBus = mgmr.getHostBus();
    auto& passiveListeningBus = mgmr.getPassiveBus();

    for (const auto& it : config)
    {
        std::unique_ptr<ReadInterface> ri;
        std::unique_ptr<WriteInterface> wi;

        std::string name = it.first;
        const conf::SensorConfig* info = &it.second;

        std::cerr << "Sensor: " << name << " " << info->type << " ";
        std::cerr << info->readPath << " " << info->writePath << "\n";

        IOInterfaceType rtype = getReadInterfaceType(info->readPath);
        IOInterfaceType wtype = getWriteInterfaceType(info->writePath);

        // fan sensors can be ready any way and written others.
        // fan sensors are the only sensors this is designed to write.
        // Nothing here should be write-only, although, in theory a fan could
        // be. I'm just not sure how that would fit together.
        // TODO(venture): It should check with the ObjectMapper to check if
        // that sensor exists on the Dbus.
        switch (rtype)
        {
            case IOInterfaceType::DBUSPASSIVE:
                // we only need to make one match based on the dbus object
                static std::shared_ptr<DbusPassiveRedundancy> redundancy =
                    std::make_shared<DbusPassiveRedundancy>(
                        passiveListeningBus);

                if (info->type == "fan")
                {
                    ri = DbusPassive::createDbusPassive(
                        passiveListeningBus, info->type, name,
                        std::make_unique<DbusHelper>(passiveListeningBus), info,
                        redundancy);
                }
                else
                {
                    ri = DbusPassive::createDbusPassive(
                        passiveListeningBus, info->type, name,
                        std::make_unique<DbusHelper>(passiveListeningBus), info,
                        nullptr);
                }
                if (ri == nullptr)
                {
                    throw SensorBuildException(
                        "Failed to create dbus passive sensor: " + name +
                        " of type: " + info->type);
                }
                break;
            case IOInterfaceType::EXTERNAL:
                // These are a special case for read-only.
                break;
            case IOInterfaceType::SYSFS:
                ri = std::make_unique<SysFsRead>(info->readPath);
                break;
            default:
                ri = std::make_unique<WriteOnly>();
                break;
        }

        if (info->type == "fan")
        {
            switch (wtype)
            {
                case IOInterfaceType::SYSFS:
                    if (info->max > 0)
                    {
                        wi = std::make_unique<SysFsWritePercent>(
                            info->writePath, info->min, info->max);
                    }
                    else
                    {
                        wi = std::make_unique<SysFsWrite>(info->writePath,
                                                          info->min, info->max);
                    }

                    break;
                case IOInterfaceType::DBUSACTIVE:
                    if (info->max > 0)
                    {
                        wi = DbusWritePercent::createDbusWrite(
                            info->writePath, info->min, info->max,
                            std::make_unique<DbusHelper>(passiveListeningBus));
                    }
                    else
                    {
                        wi = DbusWrite::createDbusWrite(
                            info->writePath, info->min, info->max,
                            std::make_unique<DbusHelper>(passiveListeningBus));
                    }

                    if (wi == nullptr)
                    {
                        throw SensorBuildException(
                            "Unable to create write dbus interface for path: " +
                            info->writePath);
                    }

                    break;
                default:
                    wi = std::make_unique<ReadOnlyNoExcept>();
                    break;
            }

            auto sensor = std::make_unique<PluggableSensor>(
                name, info->timeout, std::move(ri), std::move(wi),
                info->ignoreFailIfHostOff);
            mgmr.addSensor(info->type, name, std::move(sensor));
        }
        else if (info->type == "temp" || info->type == "margin" ||
                 info->type == "power" || info->type == "powersum")
        {
            // These sensors are read-only, but only for this application
            // which only writes to fan sensors.
            std::cerr << info->type << " readPath: " << info->readPath << "\n";

            if (IOInterfaceType::EXTERNAL == rtype)
            {
                std::cerr << "Creating HostSensor: " << name
                          << " path: " << info->readPath << "\n";

                /*
                 * The reason we handle this as a HostSensor is because it's
                 * not quite pluggable; but maybe it could be.
                 */
                auto sensor = HostSensor::createTemp(
                    name, info->timeout, hostSensorBus, info->readPath.c_str(),
                    deferSignals);
                mgmr.addSensor(info->type, name, std::move(sensor));
            }
            else
            {
                wi = std::make_unique<ReadOnlyNoExcept>();
                auto sensor = std::make_unique<PluggableSensor>(
                    name, info->timeout, std::move(ri), std::move(wi),
                    info->ignoreFailIfHostOff);
                mgmr.addSensor(info->type, name, std::move(sensor));
            }
        }
    }

    return mgmr;
}

} // namespace pid_control
