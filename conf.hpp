#pragma once

#include <map>
#include <string>
#include <vector>

#include "pid/ec/pid.hpp"


/*
 * General sensor structure used for configuration.
 */
struct sensor
{
    /* Used for listen if readpath is passive. */
    std::string type;
    /* Can be a sensor path or a dbus path. */
    std::string readpath;
    std::string writepath;
    /* min/max values for writing a percentage or error checking. */
    int64_t min;
    int64_t max;
    int64_t timeout;
};

/*
 * Structure for holding the configuration of a PID.
 */
struct controller_info
{
    std::string type;                // fan or margin or temp?
    std::vector<std::string> inputs; // one or more sensors.
    float setpoint;                  // initial setpoint for thermal.
    ec::pidinfo info;                // pid details
};

/*
 * General zone structure used for configuration.  A zone is a list of PIDs
 * and a set of configuration settings.  This structure gets filled out with
 * the zone configuration settings and not the PID details.
 */
struct zone
{
    /* The minimum RPM value we would ever want. */
    float minthermalrpm;

    /* If the sensors are in fail-safe mode, this is the percentage to use. */
    float failsafepercent;
};

using PIDConf = std::map<std::string, struct controller_info>;
