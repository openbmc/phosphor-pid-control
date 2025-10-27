// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "dbus_mode.hpp"
#include "manualcmds.hpp"

#include <ipmid/iana.hpp>
#include <ipmid/oemopenbmc.hpp>
#include <ipmid/oemrouter.hpp>

#include <cstdio>
#include <functional>
#include <memory>

namespace pid_control::ipmi
{

ZoneControlIpmiHandler handler(std::make_unique<DbusZoneControl>());

} // namespace pid_control::ipmi

void setupGlobalOemFanControl() __attribute__((constructor));

void setupGlobalOemFanControl()
{
    oem::Router* router = oem::mutableRouter();

    std::fprintf(
        stderr,
        "Registering OEM:[%#08X], Cmd:[%#04X] for Manual Zone Control\n",
        oem::obmcOemNumber, oem::Cmd::fanManualCmd);

    router->registerHandler(
        oem::obmcOemNumber, oem::Cmd::fanManualCmd,
        std::bind_front(pid_control::ipmi::manualModeControl,
                        &pid_control::ipmi::handler));
}
