# Sensor Overrides

Author: clabrado@google.com

`phoshpor-pid-control` provides a dbus API to override the value of a single
sensor.  Attempts to add an additional override will be ignored.

The override will remain in place until 10 minutes have elapsed or it is removed
via the API.

When `phosphor-pid-control` polls a sensor with an active override it will use
whichever reading is higher between the actual sensor reading and the specified
override value.  This protects the BMC from silently overheat due to an overridden
sensor.

If the override has instead expired then `phosphor-pid-control` will discard the
override and a new one can be assigned via the DBUS API.

## Usage

### Override sensor

    busctl call xyz.openbmc_project.Swampd \
    /xyz/openbmc_project/swampd  xyz.openbmc_project.swampd.override \
    AddTemperatureOverride  sd \
    SENSOR_PATH OVERRIDE_VALUE

    busctl call xyz.openbmc_project.Swampd \
    /xyz/openbmc_project/swampd \
    xyz.openbmc_project.swampd.override \
    AddTemperatureOverride  sd \
    /xyz/openbmc_project/sensors/temperature/DIMM0 88.8

### Remove override

    busctl call xyz.openbmc_project.Swampd \
    /xyz/openbmc_project/swampd \
    xyz.openbmc_project.swampd.override \
    RemoveTemperatureOverride s \
    SENSOR_PATH

    busctl call xyz.openbmc_project.Swampd \
    /xyz/openbmc_project/swampd \
    xyz.openbmc_project.swampd.override \
    RemoveTemperatureOverride s \
    /xyz/openbmc_project/sensors/temperature/DIMM0
