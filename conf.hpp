#pragma once

#include "pid/ec/pid.hpp"
#include "pid/ec/stepwise.hpp"

#include <map>
#include <string>
#include <vector>

/*
 * General sensor structure used for configuration.
 */
struct Sensor
{
    /* Used for listen if readpath is passive. */
    std::string _type;
    /* Can be a sensor path or a dbus path. */
    std::string _readpath;
    std::string _writepath;
    /* min/max values for writing a percentage or error checking. */
    int64_t _min;
    int64_t _max;
    int64_t _timeout;
};

/*
 * Structure for holding the configuration of a PID.
 */
struct ControllerInfo
{
    std::string _type;                // fan or margin or temp?
    std::vector<std::string> _inputs; // one or more sensors.
    float _setpoint;                  // initial setpoint for thermal.
    union
    {
        ec::pidinfo _pidInfo; // pid details
        ec::StepwiseInfo _stepwiseInfo;
    };
};

/*
 * General zone structure used for configuration.  A zone is a list of PIDs
 * and a set of configuration settings.  This structure gets filled out with
 * the zone configuration settings and not the PID details.
 */
struct Zone
{
    /* The minimum RPM value we would ever want. */
    float _minthermalrpm;

    /* If the sensors are in fail-safe mode, this is the percentage to use. */
    float _failsafepercent;
};

using PIDConf = std::map<std::string, struct controller_info>;
