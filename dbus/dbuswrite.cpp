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

#include "dbuswrite.hpp"

#include "dbushelper_interface.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

namespace pid_control
{

constexpr const char* pwmInterface = "xyz.openbmc_project.Control.FanPwm";

// Because the PWM interface is treated as write-only,
// it has no way of knowing the fan PWM was changed by another D-Bus user,
// perhaps somebody setting the fans into manual mode,
// which can be done externally over IPMI.
// Because we cache the last written PWM setpoint,
// this means that we could be fooled into thinking the PWM was already set,
// when in fact, the fan is at a different PWM.
// This would cause us to appear to get stuck,
// never updating the fan PWM,
// even though it needed to be updated.
// So, as a workaround, every so often, make a redundant PWM write,
// but not so often that it generates excessive D-Bus traffic.
constexpr int maxDupes = 150;

using namespace phosphor::logging;

std::unique_ptr<WriteInterface> DbusWritePercent::createDbusWrite(
    const std::string& path, int64_t min, int64_t max,
    std::unique_ptr<DbusHelperInterface> helper)
{
    std::string connectionName;

    try
    {
        connectionName = helper->getService(pwmInterface, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    return std::make_unique<DbusWritePercent>(path, min, max, connectionName);
}

void DbusWritePercent::write(double value)
{
    double minimum = getMin();
    double maximum = getMax();

    double range = maximum - minimum;
    double offset = range * value;
    double ovalue = offset + minimum;

    if (oldValue == static_cast<int64_t>(ovalue))
    {
        ++dupeCount;
        if (dupeCount < maxDupes)
        {
            return;
        }
    }

    auto writeBus = sdbusplus::bus::new_default();
    auto mesg =
        writeBus.new_method_call(connectionName.c_str(), path.c_str(),
                                 "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                std::variant<uint64_t>(static_cast<uint64_t>(ovalue)));

    try
    {
        // TODO: if we don't use the reply, call_noreply()
        auto resp = writeBus.call(mesg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Dbus Call Failure", entry("PATH=%s", path.c_str()),
                        entry("WHAT=%s", ex.what()));
        return;
    }

    oldValue = static_cast<int64_t>(ovalue);
    dupeCount = 0;
}

std::unique_ptr<WriteInterface>
    DbusWrite::createDbusWrite(const std::string& path, int64_t min,
                               int64_t max,
                               std::unique_ptr<DbusHelperInterface> helper)
{
    std::string connectionName;

    try
    {
        connectionName = helper->getService(pwmInterface, path);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }

    return std::make_unique<DbusWrite>(path, min, max, connectionName);
}

void DbusWrite::write(double value)
{
    if (oldValue == static_cast<int64_t>(value))
    {
        ++dupeCount;
        if (dupeCount < maxDupes)
        {
            return;
        }
    }

    auto writeBus = sdbusplus::bus::new_default();
    auto mesg =
        writeBus.new_method_call(connectionName.c_str(), path.c_str(),
                                 "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                std::variant<uint64_t>(static_cast<uint64_t>(value)));

    try
    {
        // TODO: consider call_noreplly
        auto resp = writeBus.call(mesg);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("Dbus Call Failure", entry("PATH=%s", path.c_str()),
                        entry("WHAT=%s", ex.what()));
        return;
    }

    oldValue = static_cast<int64_t>(value);
    dupeCount = 0;
}

} // namespace pid_control
