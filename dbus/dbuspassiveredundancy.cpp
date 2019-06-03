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

#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <set>
#include <unordered_map>
#include <variant>

constexpr const char* redundancyInterface =
    "xyz.openbmc_project.Control.FanRedundancy";

DbusPassiveRedundancy::DbusPassiveRedundancy(sdbusplus::bus::bus& bus) :
    match(bus,
          "type='signal',member='PropertiesChanged',arg0namespace='" +
              std::string(redundancyInterface) + "'",
          std::move([this](sdbusplus::message::message& message) {
              std::string objectName;
              std::unordered_map<
                  std::string,
                  std::variant<std::string, std::vector<std::string>>>
                  result;
              try
              {
                  message.read(objectName, result);
              }
              catch (sdbusplus::exception_t&)
              {
                  std::cerr << "Error reading match data";
                  return;
              }
              auto findStatus = result.find("Status");
              if (findStatus == result.end())
              {
                  return;
              }
              std::string status = std::get<std::string>(findStatus->second);

              auto methodCall = passiveBus.new_method_call(
                  message.get_sender(), message.get_path(),
                  "org.freedesktop.DBus.Properties", "Get");
              methodCall.append(redundancyInterface, "Collection");
              std::variant<std::vector<std::string>> collection;

              try
              {
                  auto reply = passiveBus.call(methodCall);
                  reply.read(collection);
              }
              catch (sdbusplus::exception_t&)
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
          })),
    passiveBus(bus)
{
    populateFailures();
}

void DbusPassiveRedundancy::populateFailures(void)
{
    auto mapper = passiveBus.new_method_call(
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("/", 0, std::array<const char*, 1>{redundancyInterface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;
    try
    {
        auto resp = passiveBus.call(mapper);
        resp.read(respData);
    }
    catch (sdbusplus::exception_t&)
    {
        std::cerr << "Populate Failures Mapper Error\n";
        return;
    }

    for (const auto& [path, interfaceDict] : respData)
    {
        for (const auto& [owner, _] : interfaceDict)
        {
            auto call = passiveBus.new_method_call(
                owner.c_str(), path.c_str(), "org.freedesktop.DBus.Properties",
                "GetAll");
            call.append(redundancyInterface);

            std::unordered_map<
                std::string,
                std::variant<std::string, std::vector<std::string>>>
                getAll;
            try
            {
                auto data = passiveBus.call(call);
                data.read(getAll);
            }
            catch (sdbusplus::exception_t&)
            {
                std::cerr << "Populate Failures Mapper Error\n";
                return;
            }
            std::string status = std::get<std::string>(getAll["Status"]);
            if (status.rfind("Failed") == std::string::npos)
            {
                continue;
            }
            std::vector<std::string> collection =
                std::get<std::vector<std::string>>(getAll["Collection"]);
            failed.insert(collection.begin(), collection.end());
        }
    }
}

const std::set<std::string>& DbusPassiveRedundancy::getFailed()
{
    return failed;
}