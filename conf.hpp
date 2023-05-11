#pragma once

#include "pid/ec/pid.hpp"
#include "pid/ec/stepwise.hpp"

#include <map>
#include <string>
#include <vector>

namespace pid_control
{
namespace conf
{

/*
 * General sensor structure used for configuration.
 */
struct SensorConfig
{
    /* Used for listen if readPath is passive. */
    std::string type;
    /* Can be a sensor path or a dbus path. */
    std::string readPath;
    std::string writePath;
    /* min/max values for writing a percentage or error checking. */
    int64_t min;
    int64_t max;
    int64_t timeout;
    bool ignoreDbusMinMax;
    bool unavailableAsFailed;
};

/*
 * Structure for holding the configuration of a PID.
 */
struct ControllerInfo
{
    std::string type;                // fan or margin or temp?
    std::vector<std::string> inputs; // one or more sensors.
    double setpoint;                 // initial setpoint for thermal.
    ec::pidinfo pidInfo;             // pid details
    ec::StepwiseInfo stepwiseInfo;
    double failSafePercent;
};

struct CycleTime
{
    /* The time interval every cycle. 0.1 seconds by default */
    uint64_t cycleIntervalTimeMS = 100; // milliseconds

    /* The interval of updating thermals. 1 second by default */
    uint64_t updateThermalsTimeMS = 1000; // milliseconds
};

/*
 * General zone structure used for configuration.  A zone is a list of PIDs
 * and a set of configuration settings.  This structure gets filled out with
 * the zone configuration settings and not the PID details.
 */
struct ZoneConfig
{
    /* The minimum set-point value we would ever want (typically in RPM) */
    double minThermalOutput;

    /* If the sensors are in fail-safe mode, this is the percentage to use. */
    double failsafePercent;

    /* Customize time settings for every cycle */
    CycleTime cycleTime;
};

using PIDConf = std::map<std::string, ControllerInfo>;

constexpr bool DEBUG = false; // enable to print found configuration

} // namespace conf
} // namespace pid_control
