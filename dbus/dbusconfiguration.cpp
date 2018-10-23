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

#include "conf.hpp"
#include "dbus/util.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
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
constexpr const char* stepwiseConfigurationInterface =
    "xyz.openbmc_project.Configuration.Stepwise";
constexpr const char* sensorInterface = "xyz.openbmc_project.Sensor.Value";
constexpr const char* pwmInterface = "xyz.openbmc_project.Control.FanPwm";

namespace dbus_configuration
{

namespace variant_ns = sdbusplus::message::variant_ns;

bool findSensor(const std::unordered_map<std::string, std::string>& sensors,
                const std::string& search,
                std::pair<std::string, std::string>& sensor)
{
    auto found =
        std::find_if(sensors.begin(), sensors.end(), [&search](const auto& s) {
            return (s.first.find(search) != std::string::npos);
        });
    if (found != sensors.end())
    {
        sensor = *found;
        return true;
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
    for (const auto& pair : SensorConfig)
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
    for (const auto& zone : ZoneDetailsConfig)
    {
        std::cout << "\t{" << zone.first << ",\n";
        std::cout << "\t\t{" << zone.second.minthermalrpm << ", ";
        std::cout << zone.second.failsafepercent << "}\n\t},\n";
    }
    std::cout << "}\n\n";
    std::cout << "ZoneConfig\n";
    std::cout << "{\n";
    for (const auto& zone : ZoneConfig)
    {
        std::cout << "\t{" << zone.first << "\n";
        for (const auto& pidconf : zone.second)
        {
            std::cout << "\t\t{" << pidconf.first << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.type << ",\n";
            std::cout << "\t\t\t{";
            for (const auto& input : pidconf.second.inputs)
            {
                std::cout << "\n\t\t\t" << input << ",\n";
            }
            std::cout << "\t\t\t}\n";
            std::cout << "\t\t\t" << pidconf.second.setpoint << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.ts << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.p_c << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.i_c << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.ff_off << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.ff_gain << ",\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.i_lim.min << ","
                      << pidconf.second.pidInfo.i_lim.max << "},\n";
            std::cout << "\t\t\t{" << pidconf.second.pidInfo.out_lim.min << ","
                      << pidconf.second.pidInfo.out_lim.max << "},\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slew_neg << ",\n";
            std::cout << "\t\t\t" << pidconf.second.pidInfo.slew_pos << ",\n";
            std::cout << "\t\t\t}\n\t\t}\n";
        }
        std::cout << "\t},\n";
    }
    std::cout << "}\n\n";
}

int eventHandler(sd_bus_message*, void*, sd_bus_error*)
{
    // do a brief sleep as we tend to get a bunch of these events at
    // once
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "New configuration detected, restarting\n.";
    std::exit(EXIT_SUCCESS); // service file should make us restart
    return 1;
}

void init(sdbusplus::bus::bus& bus)
{
    using DbusVariantType =
        sdbusplus::message::variant<uint64_t, int64_t, double, std::string,
                                    std::vector<std::string>,
                                    std::vector<double>>;

    using ManagedObjectType = std::unordered_map<
        sdbusplus::message::object_path,
        std::unordered_map<std::string,
                           std::unordered_map<std::string, DbusVariantType>>>;

    // restart on configuration properties changed
    static sdbusplus::bus::match::match configMatch(
        bus,
        "type='signal',member='PropertiesChanged',arg0namespace='" +
            std::string(pidConfigurationInterface) + "'",
        eventHandler);

    // restart on sensors changed
    static sdbusplus::bus::match::match sensorAdded(
        bus,
        "type='signal',member='InterfacesAdded',arg0path='/xyz/openbmc_project/"
        "sensors/'",
        eventHandler);

    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("", 0,
                  std::array<const char*, 6>{objectManagerInterface,
                                             pidConfigurationInterface,
                                             pidZoneConfigurationInterface,
                                             stepwiseConfigurationInterface,
                                             sensorInterface, pwmInterface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;
    try
    {
        auto resp = bus.call(mapper);
        if (resp.is_method_error())
        {
            throw std::runtime_error("ObjectMapper Call Failure");
        }
        resp.read(respData);
    }
    catch (sdbusplus::exception_t&)
    {
        // can't do anything without mapper call data
        throw std::runtime_error("ObjectMapper Call Failure");
    }

    if (respData.empty())
    {
        // can't do anything without mapper call data
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
                    interface == pidZoneConfigurationInterface ||
                    interface == stepwiseConfigurationInterface)
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
        ManagedObjectType configuration;
        try
        {
            auto responce = bus.call(endpoint);
            if (responce.is_method_error())
            {
                throw std::runtime_error("Error getting managed objects from " +
                                         owner.first);
            }
            responce.read(configuration);
        }
        catch (sdbusplus::exception_t&)
        {
            // this shouldn't happen, probably means daemon crashed
            throw std::runtime_error("Error getting managed objects from " +
                                     owner.first);
        }

        for (auto& pathPair : configuration)
        {
            if (pathPair.second.find(pidConfigurationInterface) !=
                    pathPair.second.end() ||
                pathPair.second.find(pidZoneConfigurationInterface) !=
                    pathPair.second.end() ||
                pathPair.second.find(stepwiseConfigurationInterface) !=
                    pathPair.second.end())
            {
                configurations.emplace(pathPair);
            }
        }
    }

    // on dbus having an index field is a bit strange, so randomly
    // assign index based on name property
    std::vector<std::string> zoneIndex;
    for (const auto& configuration : configurations)
    {
        auto findZone =
            configuration.second.find(pidZoneConfigurationInterface);
        if (findZone != configuration.second.end())
        {
            const auto& zone = findZone->second;
            size_t index = 1;
            const std::string& name =
                variant_ns::get<std::string>(zone.at("Name"));
            auto it = std::find(zoneIndex.begin(), zoneIndex.end(), name);
            if (it == zoneIndex.end())
            {
                zoneIndex.emplace_back(name);
                index = zoneIndex.size();
            }
            else
            {
                index = zoneIndex.end() - it;
            }

            auto& details = ZoneDetailsConfig[index];
            details.minthermalrpm = variant_ns::apply_visitor(
                VariantToFloatVisitor(), zone.at("MinThermalRpm"));
            details.failsafepercent = variant_ns::apply_visitor(
                VariantToFloatVisitor(), zone.at("FailSafePercent"));
        }
        auto findBase = configuration.second.find(pidConfigurationInterface);
        if (findBase != configuration.second.end())
        {

            const auto& base =
                configuration.second.at(pidConfigurationInterface);
            const std::vector<std::string>& zones =
                variant_ns::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                auto it = std::find(zoneIndex.begin(), zoneIndex.end(), zone);
                size_t index = 1;
                if (it == zoneIndex.end())
                {
                    zoneIndex.emplace_back(zone);
                    index = zoneIndex.size();
                }
                else
                {
                    index = zoneIndex.end() - it;
                }
                PIDConf& conf = ZoneConfig[index];

                std::vector<std::string> sensorNames =
                    variant_ns::get<std::vector<std::string>>(
                        base.at("Inputs"));
                auto findOutputs =
                    base.find("Outputs"); // currently only fans have outputs
                if (findOutputs != base.end())
                {
                    std::vector<std::string> outputs =
                        variant_ns::get<std::vector<std::string>>(
                            findOutputs->second);
                    sensorNames.insert(sensorNames.end(), outputs.begin(),
                                       outputs.end());
                }
                bool sensorsAvailable = sensorNames.size();
                std::vector<std::string> inputs;
                for (const std::string& sensorName : sensorNames)
                {
                    std::string name = sensorName;
                    // replace spaces with underscores to be legal on dbus
                    std::replace(name.begin(), name.end(), ' ', '_');
                    std::pair<std::string, std::string> sensorPathIfacePair;

                    if (!findSensor(sensors, name, sensorPathIfacePair))
                    {
                        sensorsAvailable = false;
                        break;
                    }
                    if (sensorPathIfacePair.second == sensorInterface)
                    {
                        inputs.push_back(name);
                        auto& config = SensorConfig[name];
                        config.type =
                            variant_ns::get<std::string>(base.at("Class"));
                        config.readpath = sensorPathIfacePair.first;
                        // todo: maybe un-hardcode this if we run into slower
                        // timeouts with sensors
                        if (config.type == "temp")
                        {
                            config.timeout = 500;
                        }
                    }
                    else if (sensorPathIfacePair.second == pwmInterface)
                    {
                        // copy so we can modify it
                        for (std::string otherSensor : sensorNames)
                        {
                            if (otherSensor == sensorName)
                            {
                                continue;
                            }
                            std::replace(otherSensor.begin(), otherSensor.end(),
                                         ' ', '_');
                            auto& config = SensorConfig[otherSensor];
                            config.writepath = sensorPathIfacePair.first;
                            // todo: un-hardcode this if there are fans with
                            // different ranges
                            config.max = 255;
                            config.min = 0;
                        }
                    }
                }
                // if the sensors aren't available in the current state, don't
                // add them to the configuration.
                if (!sensorsAvailable)
                {
                    continue;
                }
                struct controller_info& info =
                    conf[variant_ns::get<std::string>(base.at("Name"))];
                info.inputs = std::move(inputs);

                info.type = variant_ns::get<std::string>(base.at("Class"));
                // todo: auto generation yaml -> c script seems to discard this
                // value for fans, verify this is okay
                if (info.type == "fan")
                {
                    info.setpoint = 0;
                }
                else
                {
                    info.setpoint = variant_ns::apply_visitor(
                        VariantToFloatVisitor(), base.at("SetPoint"));
                }
                info.pidInfo.ts = 1.0; // currently unused
                info.pidInfo.p_c = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("PCoefficient"));
                info.pidInfo.i_c = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("ICoefficient"));
                info.pidInfo.ff_off = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("FFOffCoefficient"));
                info.pidInfo.ff_gain = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("FFGainCoefficient"));
                info.pidInfo.i_lim.max = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("ILimitMax"));
                info.pidInfo.i_lim.min = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("ILimitMin"));
                info.pidInfo.out_lim.max = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("OutLimitMax"));
                info.pidInfo.out_lim.min = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("OutLimitMin"));
                info.pidInfo.slew_neg = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("SlewNeg"));
                info.pidInfo.slew_pos = variant_ns::apply_visitor(
                    VariantToFloatVisitor(), base.at("SlewPos"));
            }
        }
        auto findStepwise =
            configuration.second.find(stepwiseConfigurationInterface);
        if (findStepwise != configuration.second.end())
        {
            const auto& base = findStepwise->second;
            const std::vector<std::string>& zones =
                variant_ns::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                auto it = std::find(zoneIndex.begin(), zoneIndex.end(), zone);
                size_t index = 1;
                if (it == zoneIndex.end())
                {
                    zoneIndex.emplace_back(zone);
                    index = zoneIndex.size();
                }
                else
                {
                    index = zoneIndex.end() - it;
                }
                PIDConf& conf = ZoneConfig[index];

                std::vector<std::string> inputs;
                std::vector<std::string> sensorNames =
                    variant_ns::get<std::vector<std::string>>(
                        base.at("Inputs"));

                bool sensorFound = sensorNames.size();
                for (const std::string& sensorName : sensorNames)
                {
                    std::string name = sensorName;
                    // replace spaces with underscores to be legal on dbus
                    std::replace(name.begin(), name.end(), ' ', '_');
                    std::pair<std::string, std::string> sensorPathIfacePair;

                    if (!findSensor(sensors, name, sensorPathIfacePair))
                    {
                        sensorFound = false;
                        break;
                    }

                    inputs.push_back(name);
                    auto& config = SensorConfig[name];
                    config.readpath = sensorPathIfacePair.first;
                    config.type = "temp";
                    // todo: maybe un-hardcode this if we run into slower
                    // timeouts with sensors

                    config.timeout = 500;
                }
                if (!sensorFound)
                {
                    continue;
                }
                struct controller_info& info =
                    conf[variant_ns::get<std::string>(base.at("Name"))];
                info.inputs = std::move(inputs);

                info.type = "stepwise";
                info.stepwiseInfo.ts = 1.0; // currently unused
                info.stepwiseInfo.positiveHysteresis = 0.0;
                info.stepwiseInfo.negativeHysteresis = 0.0;
                auto findPosHyst = base.find("PositiveHysteresis");
                auto findNegHyst = base.find("NegativeHysteresis");
                if (findPosHyst != base.end())
                {
                    info.stepwiseInfo.positiveHysteresis =
                        variant_ns::apply_visitor(VariantToFloatVisitor(),
                                                  findPosHyst->second);
                }
                if (findNegHyst != base.end())
                {
                    info.stepwiseInfo.positiveHysteresis =
                        variant_ns::apply_visitor(VariantToFloatVisitor(),
                                                  findNegHyst->second);
                }
                std::vector<double> readings =
                    variant_ns::get<std::vector<double>>(base.at("Reading"));
                if (readings.size() > ec::maxStepwisePoints)
                {
                    throw std::invalid_argument("Too many stepwise points.");
                }
                if (readings.empty())
                {
                    throw std::invalid_argument(
                        "Must have one stepwise point.");
                }
                std::copy(readings.begin(), readings.end(),
                          info.stepwiseInfo.reading);
                if (readings.size() < ec::maxStepwisePoints)
                {
                    info.stepwiseInfo.reading[readings.size()] =
                        std::numeric_limits<float>::quiet_NaN();
                }
                std::vector<double> outputs =
                    variant_ns::get<std::vector<double>>(base.at("Output"));
                if (readings.size() != outputs.size())
                {
                    throw std::invalid_argument(
                        "Outputs size must match readings");
                }
                std::copy(outputs.begin(), outputs.end(),
                          info.stepwiseInfo.output);
                if (outputs.size() < ec::maxStepwisePoints)
                {
                    info.stepwiseInfo.output[outputs.size()] =
                        std::numeric_limits<float>::quiet_NaN();
                }
            }
        }
    }
    if (DEBUG)
    {
        debugPrint();
    }
    if (ZoneConfig.empty())
    {
        std::cerr << "No fan zones, application pausing until reboot\n";
        while (1)
        {
            bus.process_discard();
        }
    }
}
} // namespace dbus_configuration
