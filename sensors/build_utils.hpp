#pragma once

#include <string>

namespace pid_control
{

/* This program assumes sensors use the Sensor.Value interface
 * and for sensor->write() I only implemented sysfs as a type,
 * but -- how would it know whether to use Control.FanSpeed or Control.FanPwm?
 *
 * One could get the interface list for the object and search for Control.*
 * but, it needs to know the maximum, minimum.  The only sensors it wants to
 * write in this code base are Fans...
 */
enum class IOInterfaceType
{
    NONE, // There is no interface.
    EXTERNAL,
    DBUSPASSIVE,
    DBUSACTIVE, // This means for write that it needs to look up the interface.
    SYSFS,
    UNKNOWN
};

/* WriteInterfaceType is different because Dbusactive/passive. how to know... */
IOInterfaceType getWriteInterfaceType(const std::string& path);

IOInterfaceType getReadInterfaceType(const std::string& path);

} // namespace pid_control
