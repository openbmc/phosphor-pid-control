// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "build_utils.hpp"

#include <string>

namespace pid_control
{

static constexpr auto external_sensor =
    "/xyz/openbmc_project/extsensors/";                         // type/
static constexpr auto openbmc_sensor = "/xyz/openbmc_project/"; // type/
static constexpr auto sysfs = "/sys/";

IOInterfaceType getWriteInterfaceType(const std::string& path)
{
    if (path.empty() || "None" == path)
    {
        return IOInterfaceType::NONE;
    }

    if (path.find(sysfs) != std::string::npos)
    {
        // A sysfs read sensor.
        return IOInterfaceType::SYSFS;
    }

    if (path.find(openbmc_sensor) != std::string::npos)
    {
        return IOInterfaceType::DBUSACTIVE;
    }

    return IOInterfaceType::UNKNOWN;
}

IOInterfaceType getReadInterfaceType(const std::string& path)
{
    if (path.empty() || "None" == path)
    {
        return IOInterfaceType::NONE;
    }

    if (path.find(external_sensor) != std::string::npos)
    {
        return IOInterfaceType::EXTERNAL;
    }

    if (path.find(openbmc_sensor) != std::string::npos)
    {
        return IOInterfaceType::DBUSPASSIVE;
    }

    if (path.find(sysfs) != std::string::npos)
    {
        return IOInterfaceType::SYSFS;
    }

    return IOInterfaceType::UNKNOWN;
}

} // namespace pid_control
