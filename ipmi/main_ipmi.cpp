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

#include "dbus_mode.hpp"
#include "manualcmds.hpp"

#include <ipmid/iana.hpp>
#include <ipmid/oemopenbmc.hpp>
#include <ipmid/oemrouter.hpp>

#include <cstdio>
#include <functional>
#include <memory>

namespace pid_control_ipmi
{

ZoneControlIpmiHandler handler(std::make_unique<DbusZoneControl>());

} // namespace pid_control_ipmi

void setupGlobalOemFanControl() __attribute__((constructor));

void setupGlobalOemFanControl()
{
    oem::Router* router = oem::mutableRouter();

    std::fprintf(
        stderr,
        "Registering OEM:[%#08X], Cmd:[%#04X] for Manual Zone Control\n",
        oem::obmcOemNumber, oem::Cmd::fanManualCmd);

    router->registerHandler(oem::obmcOemNumber, oem::Cmd::fanManualCmd,
                            std::bind_front(pid_control_ipmi::manualModeControl,
                                            &pid_control_ipmi::handler));
}
