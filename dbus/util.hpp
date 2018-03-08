#pragma once

#include <sdbusplus/bus.hpp>

struct SensorProperties
{
    int64_t scale;
    int64_t value;
    std::string unit;
};

/*
 * Retrieve the dbus bus (or service) for the given object and interface.
 */
std::string GetService(sdbusplus::bus::bus& bus,
                       const std::string& intf,
                       const std::string& path);

void GetProperties(sdbusplus::bus::bus& bus,
                   const std::string& service,
                   const std::string& path,
                   struct SensorProperties* prop);

std::string GetSensorPath(const std::string& type, const std::string& id);
std::string GetMatch(const std::string& type, const std::string& id);

const std::string sensorintf = "xyz.openbmc_project.Sensor.Value";
const std::string propertiesintf = "org.freedesktop.DBus.Properties";

