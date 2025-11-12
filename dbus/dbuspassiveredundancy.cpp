/*
// Copyright (c) 2019 Intel Corporation
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

#include "dbuspassiveredundancy.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Control/FanRedundancy/common.hpp>
#include <xyz/openbmc_project/ObjectMapper/common.hpp>

#include <array>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using ObjectMapper = sdbusplus::common::xyz::openbmc_project::ObjectMapper;
using ControlFanRedundancy =
    sdbusplus::common::xyz::openbmc_project::control::FanRedundancy;

namespace pid_control
{

namespace properties
{

constexpr const char* interface = "org.freedesktop.DBus.Properties";
constexpr const char* get = "Get";
constexpr const char* getAll = "GetAll";

} // namespace properties

DbusPassiveRedundancy::DbusPassiveRedundancy(sdbusplus::bus_t& bus) :
    match(bus,
          "type='signal',member='PropertiesChanged',arg0namespace='" +
              std::string(ControlFanRedundancy::interface) + "'",

          [this](sdbusplus::message_t& message) {
              std::string objectName;
              std::unordered_map<
                  std::string,
                  std::variant<std::string, std::vector<std::string>>>
                  result;
              try
              {
                  message.read(objectName, result);
              }
              catch (const sdbusplus::exception_t&)
              {
                  std::cerr << "Error reading match data";
                  return;
              }
              auto findStatus =
                  result.find(ControlFanRedundancy::property_names::status);
              if (findStatus == result.end())
              {
                  return;
              }
              std::string status = std::get<std::string>(findStatus->second);

              auto methodCall = passiveBus.new_method_call(
                  message.get_sender(), message.get_path(),
                  properties::interface, properties::get);
              methodCall.append(
                  ControlFanRedundancy::interface,
                  ControlFanRedundancy::property_names::collection);
              std::variant<std::vector<std::string>> collection;

              try
              {
                  auto reply = passiveBus.call(methodCall);
                  reply.read(collection);
              }
              catch (const sdbusplus::exception_t&)
              {
                  std::cerr << "Error reading match data";
                  return;
              }

              auto data = std::get<std::vector<std::string>>(collection);
              if (status.rfind("Failed") != std::string::npos)
              {
                  failed.insert(data.begin(), data.end());
              }
              else
              {
                  for (const auto& d : data)
                  {
                      failed.erase(d);
                  }
              }
          }),
    passiveBus(bus)
{
    populateFailures();
}

void DbusPassiveRedundancy::populateFailures(void)
{
    auto mapper = passiveBus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_sub_tree);
    mapper.append("/", 0,
                  std::array<const char*, 1>{ControlFanRedundancy::interface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;
    try
    {
        auto resp = passiveBus.call(mapper);
        resp.read(respData);
    }
    catch (const sdbusplus::exception_t&)
    {
        std::cerr << "Populate Failures Mapper Error\n";
        return;
    }

    /*
     * The subtree response looks like:
     * {path :
     *     {busname:
     *        {interface, interface, interface, ...}
     *     }
     * }
     *
     * This loops through this structure to pre-populate the already failed
     * items
     */

    for (const auto& [path, interfaceDict] : respData)
    {
        for (const auto& [owner, _] : interfaceDict)
        {
            auto call = passiveBus.new_method_call(
                owner.c_str(), path.c_str(), properties::interface,
                properties::getAll);
            call.append(ControlFanRedundancy::interface);

            std::unordered_map<
                std::string,
                std::variant<std::string, std::vector<std::string>>>
                getAll;
            try
            {
                auto data = passiveBus.call(call);
                data.read(getAll);
            }
            catch (const sdbusplus::exception_t&)
            {
                std::cerr << "Populate Failures Mapper Error\n";
                return;
            }
            std::string status = std::get<std::string>(
                getAll[ControlFanRedundancy::property_names::status]);
            if (status.rfind("Failed") == std::string::npos)
            {
                continue;
            }
            std::vector<std::string> collection =
                std::get<std::vector<std::string>>(
                    getAll[ControlFanRedundancy::property_names::collection]);
            failed.insert(collection.begin(), collection.end());
        }
    }
}

const std::set<std::string>& DbusPassiveRedundancy::getFailed()
{
    return failed;
}

} // namespace pid_control
