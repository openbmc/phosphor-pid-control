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
#pragma once

#include "conf.hpp"

#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using DbusVariantType =
    std::variant<uint64_t, int64_t, double, std::string,
                 std::vector<std::string>, std::vector<double>>;

using ManagedObjectType = std::unordered_map<
    sdbusplus::message::object_path,
    std::unordered_map<std::string,
                       std::unordered_map<std::string, DbusVariantType>>>;

namespace pid_control
{
namespace dbus_configuration
{
bool init(sdbusplus::bus::bus& bus, boost::asio::steady_timer& timer);
} // namespace dbus_configuration
} // namespace pid_control
