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
#include "dbusconfiguration.hpp"

#include "conf.hpp"
#include "dbushelper.hpp"
#include "dbusutil.hpp"
#include "util.hpp"

#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <unordered_map>
#include <variant>

namespace pid_control
{

constexpr const char* pidConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid";
constexpr const char* objectManagerInterface =
    "org.freedesktop.DBus.ObjectManager";
constexpr const char* pidZoneConfigurationInterface =
    "xyz.openbmc_project.Configuration.Pid.Zone";
constexpr const char* stepwiseConfigurationInterface =
    "xyz.openbmc_project.Configuration.Stepwise";
constexpr const char* thermalControlIface =
    "xyz.openbmc_project.Control.ThermalMode";
constexpr const char* sensorInterface = "xyz.openbmc_project.Sensor.Value";
constexpr const char* defaultPwmInterface =
    "xyz.openbmc_project.Control.FanPwm";

using Association = std::tuple<std::string, std::string, std::string>;
using Associations = std::vector<Association>;

namespace thresholds
{
constexpr const char* warningInterface =
    "xyz.openbmc_project.Sensor.Threshold.Warning";
constexpr const char* criticalInterface =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::array<const char*, 4> types = {"CriticalLow", "CriticalHigh",
                                          "WarningLow", "WarningHigh"};

} // namespace thresholds

namespace dbus_configuration
{
using SensorInterfaceType = std::pair<std::string, std::string>;

inline std::string getSensorNameFromPath(const std::string& dbusPath)
{
    return dbusPath.substr(dbusPath.find_last_of("/") + 1);
}

inline std::string sensorNameToDbusName(const std::string& sensorName)
{
    std::string retString = sensorName;
    std::replace(retString.begin(), retString.end(), ' ', '_');
    return retString;
}

std::vector<std::string> getSelectedProfiles(sdbusplus::bus_t& bus)
{
    std::vector<std::string> ret;
    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("/", 0, std::array<const char*, 1>{thermalControlIface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;

    try
    {
        auto resp = bus.call(mapper);
        resp.read(respData);
    }
    catch (const sdbusplus::exception_t&)
    {
        // can't do anything without mapper call data
        throw std::runtime_error("ObjectMapper Call Failure");
    }
    if (respData.empty())
    {
        // if the user has profiles but doesn't expose the interface to select
        // one, just go ahead without using profiles
        return ret;
    }

    // assumption is that we should only have a small handful of selected
    // profiles at a time (probably only 1), so calling each individually should
    // not incur a large cost
    for (const auto& objectPair : respData)
    {
        const std::string& path = objectPair.first;
        for (const auto& ownerPair : objectPair.second)
        {
            const std::string& busName = ownerPair.first;
            auto getProfile =
                bus.new_method_call(busName.c_str(), path.c_str(),
                                    "org.freedesktop.DBus.Properties", "Get");
            getProfile.append(thermalControlIface, "Current");
            std::variant<std::string> variantResp;
            try
            {
                auto resp = bus.call(getProfile);
                resp.read(variantResp);
            }
            catch (const sdbusplus::exception_t&)
            {
                throw std::runtime_error("Failure getting profile");
            }
            std::string mode = std::get<std::string>(variantResp);
            ret.emplace_back(std::move(mode));
        }
    }
    if constexpr (pid_control::conf::DEBUG)
    {
        std::cout << "Profiles selected: ";
        for (const auto& profile : ret)
        {
            std::cout << profile << " ";
        }
        std::cout << "\n";
    }
    return ret;
}

int eventHandler(sd_bus_message* m, void* context, sd_bus_error*)
{

    if (context == nullptr || m == nullptr)
    {
        throw std::runtime_error("Invalid match");
    }

    // we skip associations because the mapper populates these, not the sensors
    const std::array<const char*, 2> skipList = {
        "xyz.openbmc_project.Association",
        "xyz.openbmc_project.Association.Definitions"};

    sdbusplus::message_t message(m);
    if (std::string(message.get_member()) == "InterfacesAdded")
    {
        sdbusplus::message::object_path path;
        std::unordered_map<
            std::string,
            std::unordered_map<std::string, std::variant<Associations, bool>>>
            data;

        message.read(path, data);

        for (const char* skip : skipList)
        {
            auto find = data.find(skip);
            if (find != data.end())
            {
                data.erase(find);
                if (data.empty())
                {
                    return 1;
                }
            }
        }

        if constexpr (pid_control::conf::DEBUG)
        {
            std::cout << "New config detected: " << path.str << std::endl;
            for (auto& d : data)
            {
                std::cout << "\tdata is " << d.first << std::endl;
                for (auto& second : d.second)
                {
                    std::cout << "\t\tdata is " << second.first << std::endl;
                }
            }
        }
    }

    boost::asio::steady_timer* timer =
        static_cast<boost::asio::steady_timer*>(context);

    // do a brief sleep as we tend to get a bunch of these events at
    // once
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([](const boost::system::error_code ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            /* another timer started*/
            return;
        }

        std::cout << "New configuration detected, reloading\n.";
        tryRestartControlLoops();
    });

    return 1;
}

void createMatches(sdbusplus::bus_t& bus, boost::asio::steady_timer& timer)
{
    // this is a list because the matches can't be moved
    static std::list<sdbusplus::bus::match_t> matches;

    const std::array<std::string, 4> interfaces = {
        thermalControlIface, pidConfigurationInterface,
        pidZoneConfigurationInterface, stepwiseConfigurationInterface};

    // this list only needs to be created once
    if (!matches.empty())
    {
        return;
    }

    // we restart when the configuration changes or there are new sensors
    for (const auto& interface : interfaces)
    {
        matches.emplace_back(
            bus,
            "type='signal',member='PropertiesChanged',arg0namespace='" +
                interface + "'",
            eventHandler, &timer);
    }
    matches.emplace_back(
        bus,
        "type='signal',member='InterfacesAdded',arg0path='/xyz/openbmc_project/"
        "sensors/'",
        eventHandler, &timer);
    matches.emplace_back(bus,
                         "type='signal',member='InterfacesRemoved',arg0path='/"
                         "xyz/openbmc_project/sensors/'",
                         eventHandler, &timer);
}

/**
 * retrieve an attribute from the pid configuration map
 * @param[in] base - the PID configuration map, keys are the attributes and
 * value is the variant associated with that attribute.
 * @param attributeName - the name of the attribute
 * @return a variant holding the value associated with a key
 * @throw runtime_error : attributeName is not in base
 */
inline DbusVariantType getPIDAttribute(
    const std::unordered_map<std::string, DbusVariantType>& base,
    const std::string& attributeName)
{
    auto search = base.find(attributeName);
    if (search == base.end())
    {
        throw std::runtime_error("missing attribute " + attributeName);
    }
    return search->second;
}

inline void getCycleTimeSetting(
    const std::unordered_map<std::string, DbusVariantType>& zone,
    const int zoneIndex, const std::string& attributeName, uint64_t& value)
{
    auto findAttributeName = zone.find(attributeName);
    if (findAttributeName != zone.end())
    {
        double tmpAttributeValue =
            std::visit(VariantToDoubleVisitor(), zone.at(attributeName));
        if (tmpAttributeValue >= 1.0)
        {
            value = static_cast<uint64_t>(tmpAttributeValue);
        }
        else
        {
            std::cerr << "Zone " << zoneIndex << ": " << attributeName
                      << " is invalid. Use default " << value << " ms\n";
        }
    }
    else
    {
        std::cerr << "Zone " << zoneIndex << ": " << attributeName
                  << " cannot find setting. Use default " << value << " ms\n";
    }
}

void populatePidInfo(
    [[maybe_unused]] sdbusplus::bus_t& bus,
    const std::unordered_map<std::string, DbusVariantType>& base,
    conf::ControllerInfo& info, const std::string* thresholdProperty,
    const std::map<std::string, conf::SensorConfig>& sensorConfig)
{
    info.type = std::get<std::string>(getPIDAttribute(base, "Class"));
    if (info.type == "fan")
    {
        info.setpoint = 0;
    }
    else
    {
        info.setpoint = std::visit(VariantToDoubleVisitor(),
                                   getPIDAttribute(base, "SetPoint"));
    }

    if (thresholdProperty != nullptr)
    {
        std::string interface;
        if (*thresholdProperty == "WarningHigh" ||
            *thresholdProperty == "WarningLow")
        {
            interface = thresholds::warningInterface;
        }
        else
        {
            interface = thresholds::criticalInterface;
        }
        const std::string& path = sensorConfig.at(info.inputs.front()).readPath;

        DbusHelper helper(sdbusplus::bus::new_system());
        std::string service = helper.getService(interface, path);
        double reading = 0;
        try
        {
            helper.getProperty(service, path, interface, *thresholdProperty,
                               reading);
        }
        catch (const sdbusplus::exception_t& ex)
        {
            // unsupported threshold, leaving reading at 0
        }

        info.setpoint += reading;
    }

    info.pidInfo.ts = 1.0; // currently unused
    info.pidInfo.proportionalCoeff = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "PCoefficient"));
    info.pidInfo.integralCoeff = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ICoefficient"));
    // DCoefficient is below, it is optional, same reason as in buildjson.cpp
    info.pidInfo.feedFwdOffset = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "FFOffCoefficient"));
    info.pidInfo.feedFwdGain = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "FFGainCoefficient"));
    info.pidInfo.integralLimit.max = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ILimitMax"));
    info.pidInfo.integralLimit.min = std::visit(
        VariantToDoubleVisitor(), getPIDAttribute(base, "ILimitMin"));
    info.pidInfo.outLim.max = std::visit(VariantToDoubleVisitor(),
                                         getPIDAttribute(base, "OutLimitMax"));
    info.pidInfo.outLim.min = std::visit(VariantToDoubleVisitor(),
                                         getPIDAttribute(base, "OutLimitMin"));
    info.pidInfo.slewNeg =
        std::visit(VariantToDoubleVisitor(), getPIDAttribute(base, "SlewNeg"));
    info.pidInfo.slewPos =
        std::visit(VariantToDoubleVisitor(), getPIDAttribute(base, "SlewPos"));

    double negativeHysteresis = 0;
    double positiveHysteresis = 0;
    double derivativeCoeff = 0;

    auto findNeg = base.find("NegativeHysteresis");
    auto findPos = base.find("PositiveHysteresis");
    auto findDerivative = base.find("DCoefficient");

    if (findNeg != base.end())
    {
        negativeHysteresis =
            std::visit(VariantToDoubleVisitor(), findNeg->second);
    }
    if (findPos != base.end())
    {
        positiveHysteresis =
            std::visit(VariantToDoubleVisitor(), findPos->second);
    }
    if (findDerivative != base.end())
    {
        derivativeCoeff =
            std::visit(VariantToDoubleVisitor(), findDerivative->second);
    }

    info.pidInfo.negativeHysteresis = negativeHysteresis;
    info.pidInfo.positiveHysteresis = positiveHysteresis;
    info.pidInfo.derivativeCoeff = derivativeCoeff;
}

bool init(sdbusplus::bus_t& bus, boost::asio::steady_timer& timer,
          std::map<std::string, conf::SensorConfig>& sensorConfig,
          std::map<int64_t, conf::PIDConf>& zoneConfig,
          std::map<int64_t, conf::ZoneConfig>& zoneDetailsConfig)
{

    sensorConfig.clear();
    zoneConfig.clear();
    zoneDetailsConfig.clear();

    createMatches(bus, timer);

    auto mapper =
        bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    mapper.append("/", 0,
                  std::array<const char*, 6>{
                      objectManagerInterface, pidConfigurationInterface,
                      pidZoneConfigurationInterface,
                      stepwiseConfigurationInterface, sensorInterface,
                      defaultPwmInterface});
    std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>
        respData;
    try
    {
        auto resp = bus.call(mapper);
        resp.read(respData);
    }
    catch (const sdbusplus::exception_t&)
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
                if (interface == sensorInterface ||
                    interface == defaultPwmInterface)
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
            responce.read(configuration);
        }
        catch (const sdbusplus::exception_t&)
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

    // remove controllers from config that aren't in the current profile(s)
    std::vector<std::string> selectedProfiles = getSelectedProfiles(bus);
    if (selectedProfiles.size())
    {
        for (auto pathIt = configurations.begin();
             pathIt != configurations.end();)
        {
            for (auto confIt = pathIt->second.begin();
                 confIt != pathIt->second.end();)
            {
                auto profilesFind = confIt->second.find("Profiles");
                if (profilesFind == confIt->second.end())
                {
                    confIt++;
                    continue; // if no profiles selected, apply always
                }
                auto profiles =
                    std::get<std::vector<std::string>>(profilesFind->second);
                if (profiles.empty())
                {
                    confIt++;
                    continue;
                }

                bool found = false;
                for (const std::string& profile : profiles)
                {
                    if (std::find(selectedProfiles.begin(),
                                  selectedProfiles.end(),
                                  profile) != selectedProfiles.end())
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    confIt++;
                }
                else
                {
                    confIt = pathIt->second.erase(confIt);
                }
            }
            if (pathIt->second.empty())
            {
                pathIt = configurations.erase(pathIt);
            }
            else
            {
                pathIt++;
            }
        }
    }

    // On D-Bus, although not necessary,
    // having the "zoneID" field can still be useful,
    // as it is used for diagnostic messages,
    // logging file names, and so on.
    // Accept optional "ZoneIndex" parameter to explicitly specify.
    // If not present, or not unique, auto-assign index,
    // using 0-based numbering, ensuring uniqueness.
    std::map<std::string, int64_t> foundZones;
    for (const auto& configuration : configurations)
    {
        auto findZone =
            configuration.second.find(pidZoneConfigurationInterface);
        if (findZone != configuration.second.end())
        {
            const auto& zone = findZone->second;

            const std::string& name = std::get<std::string>(zone.at("Name"));

            auto findZoneIndex = zone.find("ZoneIndex");
            if (findZoneIndex == zone.end())
            {
                continue;
            }

            auto ptrZoneIndex = std::get_if<double>(&(findZoneIndex->second));
            if (!ptrZoneIndex)
            {
                continue;
            }

            auto desiredIndex = static_cast<int64_t>(*ptrZoneIndex);
            auto grantedIndex = setZoneIndex(name, foundZones, desiredIndex);
            std::cout << "Zone " << name << " is at ZoneIndex " << grantedIndex
                      << "\n";
        }
    }

    for (const auto& configuration : configurations)
    {
        auto findZone =
            configuration.second.find(pidZoneConfigurationInterface);
        if (findZone != configuration.second.end())
        {
            const auto& zone = findZone->second;

            const std::string& name = std::get<std::string>(zone.at("Name"));

            auto index = getZoneIndex(name, foundZones);

            auto& details = zoneDetailsConfig[index];

            details.minThermalOutput = std::visit(VariantToDoubleVisitor(),
                                                  zone.at("MinThermalOutput"));
            details.failsafePercent = std::visit(VariantToDoubleVisitor(),
                                                 zone.at("FailSafePercent"));

            getCycleTimeSetting(zone, index, "CycleIntervalTimeMS",
                                details.cycleTime.cycleIntervalTimeMS);
            getCycleTimeSetting(zone, index, "UpdateThermalsTimeMS",
                                details.cycleTime.updateThermalsTimeMS);
        }
        auto findBase = configuration.second.find(pidConfigurationInterface);
        // loop through all the PID configurations and fill out a sensor config
        if (findBase != configuration.second.end())
        {
            const auto& base =
                configuration.second.at(pidConfigurationInterface);
            const std::string pidName = std::get<std::string>(base.at("Name"));
            const std::string pidClass =
                std::get<std::string>(base.at("Class"));
            const std::vector<std::string>& zones =
                std::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                auto index = getZoneIndex(zone, foundZones);

                conf::PIDConf& conf = zoneConfig[index];
                std::vector<std::string> inputSensorNames(
                    std::get<std::vector<std::string>>(base.at("Inputs")));
                std::vector<std::string> outputSensorNames;

                // assumption: all fan pids must have at least one output
                if (pidClass == "fan")
                {
                    outputSensorNames = std::get<std::vector<std::string>>(
                        getPIDAttribute(base, "Outputs"));
                }

                bool unavailableAsFailed = true;
                auto findUnavailableAsFailed =
                    base.find("InputUnavailableAsFailed");
                if (findUnavailableAsFailed != base.end())
                {
                    unavailableAsFailed =
                        std::get<bool>(findUnavailableAsFailed->second);
                }

                std::vector<SensorInterfaceType> inputSensorInterfaces;
                std::vector<SensorInterfaceType> outputSensorInterfaces;
                /* populate an interface list for different sensor direction
                 * types (input,output)
                 */
                /* take the Inputs from the configuration and generate
                 * a list of dbus descriptors (path, interface).
                 * Mapping can be many-to-one since an element of Inputs can be
                 * a regex
                 */
                for (const std::string& sensorName : inputSensorNames)
                {
                    findSensors(sensors, sensorNameToDbusName(sensorName),
                                inputSensorInterfaces);
                }
                for (const std::string& sensorName : outputSensorNames)
                {
                    findSensors(sensors, sensorNameToDbusName(sensorName),
                                outputSensorInterfaces);
                }

                inputSensorNames.clear();
                for (const SensorInterfaceType& inputSensorInterface :
                     inputSensorInterfaces)
                {
                    const std::string& dbusInterface =
                        inputSensorInterface.second;
                    const std::string& inputSensorPath =
                        inputSensorInterface.first;

                    // Setting timeout to 0 is intentional, as D-Bus passive
                    // sensor updates are pushed in, not pulled by timer poll.
                    // Setting ignoreDbusMinMax is intentional, as this
                    // prevents normalization of values to [0.0, 1.0] range,
                    // which would mess up the PID loop math.
                    // All non-fan PID classes should be initialized this way.
                    // As for why a fan should not use this code path, see
                    // the ed1dafdf168def37c65bfb7a5efd18d9dbe04727 commit.
                    if ((pidClass == "temp") || (pidClass == "margin") ||
                        (pidClass == "power") || (pidClass == "powersum"))
                    {
                        std::string inputSensorName =
                            getSensorNameFromPath(inputSensorPath);
                        auto& config = sensorConfig[inputSensorName];
                        inputSensorNames.push_back(inputSensorName);
                        config.type = pidClass;
                        config.readPath = inputSensorInterface.first;
                        config.timeout = 0;
                        config.ignoreDbusMinMax = true;
                        config.unavailableAsFailed = unavailableAsFailed;
                    }

                    if (dbusInterface != sensorInterface)
                    {
                        /* all expected inputs in the configuration are expected
                         * to be sensor interfaces
                         */
                        throw std::runtime_error(
                            "sensor at dbus path [" + inputSensorPath +
                            "] has an interface [" + dbusInterface +
                            "] that does not match the expected interface of " +
                            sensorInterface);
                    }
                }

                /* fan pids need to pair up tach sensors with their pwm
                 * counterparts
                 */
                if (pidClass == "fan")
                {
                    /* If a PID is a fan there should be either
                     * (1) one output(pwm) per input(tach)
                     * OR
                     * (2) one putput(pwm) for all inputs(tach)
                     * everything else indicates a bad configuration.
                     */
                    bool singlePwm = false;
                    if (outputSensorInterfaces.size() == 1)
                    {
                        /* one pwm, set write paths for all fan sensors to it */
                        singlePwm = true;
                    }
                    else if (inputSensorInterfaces.size() ==
                             outputSensorInterfaces.size())
                    {
                        /* one to one mapping, each fan sensor gets its own pwm
                         * control */
                        singlePwm = false;
                    }
                    else
                    {
                        throw std::runtime_error(
                            "fan PID has invalid number of Outputs");
                    }
                    std::string fanSensorName;
                    std::string pwmPath;
                    std::string pwmInterface;
                    std::string pwmSensorName;
                    if (singlePwm)
                    {
                        /* if just a single output(pwm) is provided then use
                         * that pwm control path for all the fan sensor write
                         * path configs
                         */
                        pwmPath = outputSensorInterfaces.at(0).first;
                        pwmInterface = outputSensorInterfaces.at(0).second;
                    }
                    for (uint32_t idx = 0; idx < inputSensorInterfaces.size();
                         idx++)
                    {
                        if (!singlePwm)
                        {
                            pwmPath = outputSensorInterfaces.at(idx).first;
                            pwmInterface =
                                outputSensorInterfaces.at(idx).second;
                        }
                        if (defaultPwmInterface != pwmInterface)
                        {
                            throw std::runtime_error(
                                "fan pwm control at dbus path [" + pwmPath +
                                "] has an interface [" + pwmInterface +
                                "] that does not match the expected interface "
                                "of " +
                                defaultPwmInterface);
                        }
                        const std::string& fanPath =
                            inputSensorInterfaces.at(idx).first;
                        fanSensorName = getSensorNameFromPath(fanPath);
                        pwmSensorName = getSensorNameFromPath(pwmPath);
                        std::string fanPwmIndex = fanSensorName + pwmSensorName;
                        inputSensorNames.push_back(fanPwmIndex);
                        auto& fanConfig = sensorConfig[fanPwmIndex];
                        fanConfig.type = pidClass;
                        fanConfig.readPath = fanPath;
                        fanConfig.writePath = pwmPath;
                        // todo: un-hardcode this if there are fans with
                        // different ranges
                        fanConfig.max = 255;
                        fanConfig.min = 0;
                    }
                }
                // if the sensors aren't available in the current state, don't
                // add them to the configuration.
                if (inputSensorNames.empty())
                {
                    continue;
                }

                std::string offsetType;

                // SetPointOffset is a threshold value to pull from the sensor
                // to apply an offset. For upper thresholds this means the
                // setpoint is usually negative.
                auto findSetpointOffset = base.find("SetPointOffset");
                if (findSetpointOffset != base.end())
                {
                    offsetType =
                        std::get<std::string>(findSetpointOffset->second);
                    if (std::find(thresholds::types.begin(),
                                  thresholds::types.end(),
                                  offsetType) == thresholds::types.end())
                    {
                        throw std::runtime_error("Unsupported type: " +
                                                 offsetType);
                    }
                }

                if (offsetType.empty())
                {
                    conf::ControllerInfo& info =
                        conf[std::get<std::string>(base.at("Name"))];
                    info.inputs = std::move(inputSensorNames);
                    populatePidInfo(bus, base, info, nullptr, sensorConfig);
                }
                else
                {
                    // we have to split up the inputs, as in practice t-control
                    // values will differ, making setpoints differ
                    for (const std::string& input : inputSensorNames)
                    {
                        conf::ControllerInfo& info = conf[input];
                        info.inputs.emplace_back(input);
                        populatePidInfo(bus, base, info, &offsetType,
                                        sensorConfig);
                    }
                }
            }
        }
        auto findStepwise =
            configuration.second.find(stepwiseConfigurationInterface);
        if (findStepwise != configuration.second.end())
        {
            const auto& base = findStepwise->second;
            const std::vector<std::string>& zones =
                std::get<std::vector<std::string>>(base.at("Zones"));
            for (const std::string& zone : zones)
            {
                auto index = getZoneIndex(zone, foundZones);

                conf::PIDConf& conf = zoneConfig[index];

                std::vector<std::string> inputs;
                std::vector<std::string> sensorNames =
                    std::get<std::vector<std::string>>(base.at("Inputs"));

                bool unavailableAsFailed = true;
                auto findUnavailableAsFailed =
                    base.find("InputUnavailableAsFailed");
                if (findUnavailableAsFailed != base.end())
                {
                    unavailableAsFailed =
                        std::get<bool>(findUnavailableAsFailed->second);
                }

                bool sensorFound = false;
                for (const std::string& sensorName : sensorNames)
                {
                    std::vector<std::pair<std::string, std::string>>
                        sensorPathIfacePairs;
                    if (!findSensors(sensors, sensorNameToDbusName(sensorName),
                                     sensorPathIfacePairs))
                    {
                        break;
                    }

                    for (const auto& sensorPathIfacePair : sensorPathIfacePairs)
                    {
                        size_t idx =
                            sensorPathIfacePair.first.find_last_of("/") + 1;
                        std::string shortName =
                            sensorPathIfacePair.first.substr(idx);

                        inputs.push_back(shortName);
                        auto& config = sensorConfig[shortName];
                        config.readPath = sensorPathIfacePair.first;
                        config.type = "temp";
                        config.ignoreDbusMinMax = true;
                        config.unavailableAsFailed = unavailableAsFailed;
                        // todo: maybe un-hardcode this if we run into slower
                        // timeouts with sensors

                        config.timeout = 0;
                        sensorFound = true;
                    }
                }
                if (!sensorFound)
                {
                    continue;
                }
                conf::ControllerInfo& info =
                    conf[std::get<std::string>(base.at("Name"))];
                info.inputs = std::move(inputs);

                info.type = "stepwise";
                info.stepwiseInfo.ts = 1.0; // currently unused
                info.stepwiseInfo.positiveHysteresis = 0.0;
                info.stepwiseInfo.negativeHysteresis = 0.0;
                std::string subtype = std::get<std::string>(base.at("Class"));

                info.stepwiseInfo.isCeiling = (subtype == "Ceiling");
                auto findPosHyst = base.find("PositiveHysteresis");
                auto findNegHyst = base.find("NegativeHysteresis");
                if (findPosHyst != base.end())
                {
                    info.stepwiseInfo.positiveHysteresis = std::visit(
                        VariantToDoubleVisitor(), findPosHyst->second);
                }
                if (findNegHyst != base.end())
                {
                    info.stepwiseInfo.negativeHysteresis = std::visit(
                        VariantToDoubleVisitor(), findNegHyst->second);
                }
                std::vector<double> readings =
                    std::get<std::vector<double>>(base.at("Reading"));
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
                        std::numeric_limits<double>::quiet_NaN();
                }
                std::vector<double> outputs =
                    std::get<std::vector<double>>(base.at("Output"));
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
                        std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
    }
    if constexpr (pid_control::conf::DEBUG)
    {
        debugPrint(sensorConfig, zoneConfig, zoneDetailsConfig);
    }
    if (zoneConfig.empty() || zoneDetailsConfig.empty())
    {
        std::cerr
            << "No fan zones, application pausing until new configuration\n";
        return false;
    }
    return true;
}

} // namespace dbus_configuration
} // namespace pid_control
