#pragma once

#include "conf.hpp"
#include "pid/ec/pid.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace pid_control
{

void tryRestartControlLoops(bool first = true, std::optional<int> maxtries = 5);

/*
 * Given a configuration structure, fill out the information we use within the
 * PID loop.
 */
void initializePIDStruct(ec::pid_info_t* info, const ec::pidinfo& initial);

void dumpPIDStruct(ec::pid_info_t* info);

struct SensorThresholds
{
    double lowerThreshold = std::numeric_limits<double>::quiet_NaN();
    double upperThreshold = std::numeric_limits<double>::quiet_NaN();
};

const std::string sensorintf = "xyz.openbmc_project.Sensor.Value";
const std::string criticalThreshInf =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
const std::string propertiesintf = "org.freedesktop.DBus.Properties";

/*
 * Given a path that optionally has a glob portion, fill it out.
 */
std::string FixupPath(std::string original);

/*
 * Dump active configuration.
 */
void debugPrint(const std::map<std::string, conf::SensorConfig>& sensorConfig,
                const std::map<int64_t, conf::PIDConf>& zoneConfig,
                const std::map<int64_t, conf::ZoneConfig>& zoneDetailsConfig);

} // namespace pid_control
