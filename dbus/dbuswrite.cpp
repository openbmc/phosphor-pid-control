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

#include "dbus/dbuswrite.hpp"

#include <iostream>
#include <sdbusplus/bus.hpp>

// this bus object is treated as a singleton because the class is constructed in
// a different thread than it is used, and as bus objects are relatively
// expensive we'd prefer to only have one
std::unique_ptr<sdbusplus::bus::bus> writeBus = nullptr;

void initBus()
{
    if (writeBus == nullptr)
    {
        writeBus = std::make_unique<sdbusplus::bus::bus>(
            sdbusplus::bus::new_default());
    }
}

void DbusWritePercent::write(double value)
{
    float minimum = getMin();
    float maximum = getMax();

    float range = maximum - minimum;
    float offset = range * value;
    float ovalue = offset + minimum;
    initBus();
    auto mesg =
        writeBus->new_method_call(connectionName.c_str(), path.c_str(),
                                  "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                sdbusplus::message::variant<uint64_t>(ovalue));
    auto resp = writeBus->call(mesg);
    if (resp.is_method_error())
    {
        std::cerr << "Error sending message to " << path << "\n";
    }
    return;
}

void DbusWrite::write(double value)
{
    initBus();
    auto mesg =
        writeBus->new_method_call(connectionName.c_str(), path.c_str(),
                                  "org.freedesktop.DBus.Properties", "Set");
    mesg.append(pwmInterface, "Target",
                sdbusplus::message::variant<uint64_t>(value));
    auto resp = writeBus->call(mesg);
    if (resp.is_method_error())
    {
        std::cerr << "Error sending message to " << path << "\n";
    }

    return;
}
