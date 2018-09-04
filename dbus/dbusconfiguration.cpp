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

#include <chrono>
#include <conf.hpp>
#include <dbus/util.hpp>
#include <functional>
#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <set>
#include <thread>
#include <unordered_map>

static constexpr bool DEBUG = false; // enable to print found configuration

std::map<std::string, struct sensor> SensorConfig = {};
std::map<int64_t, PIDConf> ZoneConfig = {};
std::map<int64_t, struct zone> ZoneDetailsConfig = {};

constexpr const char* pidConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid";
constexpr const char* objectManagerInterface =
    "org.freedesktop.DBus.ObjectManager";
constexpr const char* pidZoneConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
constexpr const char* sensorInterface = "xyz.openbmc_project.Sensor.Value";
constexpr const char* pwmInterface = "xyz.openbmc_project.Control.FanPwm";

namespace dbus_configuration
{

bool findSensor(const std::unordered_map<std::string, std::string>& sensors,
                const std::string& search,
                std::pair<std::string, std::string>& sensor)
{
    for (const auto& s : sensors)
    {
        if (s.first.find(search) != std::string::npos)
        {
            sensor = s;
            return true;
        }
    }
    return false;
}

// this function prints the configuration into a form similar to the cpp
// generated code to help in verification, should be turned off during normal
// use
void debugPrint(void)
{
    // print sensor config
    std::cout << "sensor config:\n";
    std::cout << "{\n";
    for (auto& pair : SensorConfig)
    {

        std::cout << "\t{" << pair.first << ",\n\t\t{";
        std::cout << pair.second.type << ", ";
        std::cout << pair.second.readpath << ", ";
        std::cout << pair.second.writepath << ", ";
        std::cout << pair.second.min << ", ";
        std::cout << pair.second.max << ", ";
        std::cout << pair.second.timeout << "},\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneDetailsConfig\n";
    std::cout << "{\n";
    for (auto& zone : ZoneDetailsConfig)
    {
        std::cout << "\t{" << zone.first << ",\n";
        std::cout << "\t\t{" << zone.second.minthermalrpm << ", ";
        std::cout << zone.second.failsafepercent << "}\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneConfig\n";
    std::cout << "{\n";
    for (auto& zone : ZoneConfig)
    {
        std::cout << "\t{" << zone.first << "\n";
        for (auto& pidconf : zone.second)
        {
            std::cout << "\t\t{" << pidconf.first << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.type << ",\n";
            std::cout << "\t\t\t{";
            for (auto& input : pidconf.second.inputs)
            {
                std::cout << "\n\t\t\t" << input << ",\n";
            }
            std::cout << "\t\t\t}\n";
            std::cout << "\t\t\t" << pidconf.second.setpoint << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.info.ts << ",\n";
            std::cout << "\t\t\t" << pidconf.second.info.p_c << ",\n";
            std::cout << "\t\t\t" << pidconf.second.info.i_c << ",\n";
            std::cout << "\t\t\t" << pidconf.second.info.ff_off << ",\n";
            std::cout << "\t\t\t" << pidconf.second.info.ff_gain << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.info.i_lim.min << ","
                      << pidconf.second.info.i_lim.max << "},\n";
            std::cout << "\t\t\t{" << pidconf.second.info.out_lim.min << ","
                      << pidconf.second.info.out_lim.max << "},\n";
            std::cout << "\t\t\t" << pidconf.second.info.slew_neg << ",\n";
            std::cout << "\t\t\t" << pidconf.second.info.slew_pos << ",\n";
            std::cout << "\t\t\t}\n\t\t}\n";
        }
        std::cout << "\t},\n";
    }
    std::cout << "}\n\n";
}

void init(sdbusplus::bus::bus& bus)
{
    using ManagedObjectType = std::unordered_map<
        sdbusplus::message::object_path,
        std::unordered_map<
            std::string,
            std::unordered_map<std::string,
                               sdbusplus::message::variant<
                                   uint64_t, int64_t, double, std::string,
                                   std::vector<std::string>>>>>;

    // install watch for properties changed
    std::function<void(sdbusplus::message::message & message)> eventHandler =
        [](const sdbusplus::message::message&) {
            // do a brief sleep as we tend to get a bunch of these events at
            // once
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "New configuration detected, restarting\n.";
            std::exit(EXIT_SUCCESS); // service file should make us restart
        };

    static sdbusplus::bus::match::match match(
        bus,
        "type='signal',member='PropertiesChanged',arg0namespace='" +
            std::string(pidConfigurationInterface) + "'",
        eventHandler);

    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("", 0,
                  std::array<const char*, 5>{objectManagerInterface,
                                             pidConfigurationInterface,
                                             pidZoneConfigurationInterface,
                                             sensorInterface, pwmInterface});
    auto resp = bus.call(mapper);
    if (resp.is_method_error())
    {
        throw std::runtime_error("ObjectMapper Call Failure");
    }
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;

    resp.read(respData);
    if (respData.empty())
    {
        throw std::runtime_error("No configuration data available from Mapper");
    }
    // create a map of pair of <has pid configuration, ObjectManager path>
    std::unordered_map<std::string, std::pair<bool, std::string>> owners;
    // and a map of <path, interface> for sensors
    std::unordered_map<std::string, std::string> sensors;
    for (const auto& objectPair : respData)
    {
        for (const auto& ownerPair : objectPair.second)
        {
            auto& owner = owners[ownerPair.first];
            for (const std::string& interface : ownerPair.second)
            {

                if (interface == objectManagerInterface)
                {
                    owner.second = objectPair.first;
                }
                if (interface == pidConfigurationInterface ||
                    interface == pidZoneConfigurationInterface)
                {
                    owner.first = true;
                }
                if (interface == sensorInterface || interface == pwmInterface)
                {
                    // we're not interested in pwm sensors, just pwm control
                    if (interface == sensorInterface &&
                        objectPair.first.find("pwm") != std::string::npos)
                    {
                        continue;
                    }
                    sensors[objectPair.first] = interface;
                }
            }
        }
    }
    ManagedObjectType configurations;
    for (const auto& owner : owners)
    {
        // skip if no pid configuration (means probably a sensor)
        if (!owner.second.first)
        {
            continue;
        }
        auto endpoint = bus.new_method_call(
            owner.first.c_str(), owner.second.second.c_str(),
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        auto responce = bus.call(endpoint);
        if (responce.is_method_error())
        {
            throw std::runtime_error("Error getting managed objects from " +
                                     owner.first);
        }
        ManagedObjectType configuration;
        responce.read(configuration);
        for (auto& pathPair : configuration)
        {
            if (pathPair.second.find(pidConfigurationInterface) !=
                    pathPair.second.end() ||
                pathPair.second.find(pidZoneConfigurationInterface) !=
                    pathPair.second.end())
            {
                configurations.emplace(pathPair);
            }
        }
    }
    for (const auto& configuration : configurations)
    {
        auto findZone =
            configuration.second.find(pidZoneConfigurationInterface);
        if (findZone != configuration.second.end())
        {
            const auto& zone = findZone->second;
            auto& details =
                ZoneDetailsConfig[sdbusplus::message::variant_ns::get<uint64_t>(
                    zone.at("Index"))];
            details.minthermalrpm = mapbox::util::apply_visitor(
                VariantToFloatVisitor(), zone.at("MinThermalRpm"));
            details.failsafepercent = mapbox::util::apply_visitor(
                VariantToFloatVisitor(), zone.at("FailSafePercent"));
        }
        auto findBase = configuration.second.find(pidConfigurationInterface);
        if (findBase == configuration.second.end())
        {
            continue;
        }
        // if the base configuration is found, these are required
        const auto& base = configuration.second.at(pidConfigurationInterface);
        const auto& iLim = configuration.second.at(pidConfigurationInterface +
                                                   std::string(".ILimit"));
        const auto& outLim = configuration.second.at(pidConfigurationInterface +
                                                     std::string(".OutLimit"));
        PIDConf& conf =
            ZoneConfig[sdbusplus::message::variant_ns::get<uint64_t>(
                base.at("Index"))];
        struct controller_info& info =
            conf[sdbusplus::message::variant_ns::get<std::string>(
                base.at("Name"))];
        info.type =
            sdbusplus::message::variant_ns::get<std::string>(base.at("Class"));
        // todo: auto generation yaml -> c script seems to discard this value
        // for fans, verify this is okay
        if (info.type == "fan")
        {
            info.setpoint = 0;
        }
        else
        {
            info.setpoint = mapbox::util::apply_visitor(VariantToFloatVisitor(),
                                                        base.at("SetPoint"));
        }
        info.info.ts = 1.0; // currently unused
        info.info.p_c = mapbox::util::apply_visitor(VariantToFloatVisitor(),
                                                    base.at("PCoefficient"));
        info.info.i_c = mapbox::util::apply_visitor(VariantToFloatVisitor(),
                                                    base.at("ICoefficient"));
        info.info.ff_off = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), base.at("FFOffCoefficient"));
        info.info.ff_gain = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), base.at("FFGainCoefficient"));
        auto value = mapbox::util::apply_visitor(VariantToFloatVisitor(),
                                                 iLim.at("Max"));
        info.info.i_lim.max = value;
        info.info.i_lim.min = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), iLim.at("Min"));
        info.info.out_lim.max = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), outLim.at("Max"));
        info.info.out_lim.min = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), outLim.at("Min"));
        info.info.slew_neg = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), base.at("SlewNeg"));
        info.info.slew_pos = mapbox::util::apply_visitor(
            VariantToFloatVisitor(), base.at("SlewPos"));

        std::pair<std::string, std::string> sensorPathIfacePair;
        std::vector<std::string> sensorNames =
            sdbusplus::message::variant_ns::get<std::vector<std::string>>(
                base.at("Inputs"));

        for (const std::string& sensorName : sensorNames)
        {
            std::string name = sensorName;
            // replace spaces with underscores to be legal on dbus
            std::replace(name.begin(), name.end(), ' ', '_');

            if (!findSensor(sensors, name, sensorPathIfacePair))
            {
                throw std::runtime_error(
                    "Could not map configuration to sensor " + name);
            }
            if (sensorPathIfacePair.second == sensorInterface)
            {
                info.inputs.push_back(name);
                auto& config = SensorConfig[name];
                config.type = sdbusplus::message::variant_ns::get<std::string>(
                    base.at("Class"));
                config.readpath = sensorPathIfacePair.first;
                // todo: maybe un-hardcode this if we run into slower timeouts
                // with sensors
                if (config.type == "temp")
                {
                    config.timeout = 500;
                }
            }
            if (sensorPathIfacePair.second == pwmInterface)
            {
                // copy so we can modify it
                for (std::string otherSensor : sensorNames)
                {
                    if (otherSensor == sensorName)
                    {
                        continue;
                    }
                    std::replace(otherSensor.begin(), otherSensor.end(), ' ',
                                 '_');
                    auto& config = SensorConfig[otherSensor];
                    config.writepath = sensorPathIfacePair.first;
                    // todo: un-hardcode this if there are fans with different
                    // ranges
                    config.max = 255;
                    config.min = 0;
                }
            }
        }
    }
    if (DEBUG)
    {
        debugPrint();
    }
}
} // namespace dbus_configuration
