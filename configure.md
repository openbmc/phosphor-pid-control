# How to Configure Phosphor-pid-control

A system needs two groups of configurations: zones and sensors.

## Json Configuration

```
{
    "sensors" : [
        {
            "name": "fan1",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
            "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm1",
            "min": 0,
            "max": 255
        },
        {
            "name": "fan2",
            "type": "fan",
            "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan2",
            "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm2",
            "min": 0,
            "max": 255
        },
<trim>
}
```

The overall configuratino is a dictionary with two keys, `sensors` and `zones`.
Each of those is an array of entries.

A sensor has a `name`, a `type`, a `readPath`, a `writePath`, a `minimum` value
and an `maximum` value.

The `name` is used to reference the sensor in the zone portion of the
configuration.

The `type` is the type of sensor it is. This influences how its value is
treated. Supports values are: `fan`, `temp`, and `margin`.

**TODO: Add further details on what the types mean.**

The `readPath` is the path that tells the daemon how to read the value from this
sensor. It is optional, allowing for write-only sensors. If the value is absent
or `None` it'll be treated as a write-only sensor.

If the `readPath` value contains: `/xyz/openbmc_project/extsensors/` it'll be
treated as a sensor hosted by the daemon itself whose value is provided
externally. The daemon will own the sensor and publish it to dbus. This is
currently only supported for `temp` and `margin` sensor types.

If the `readPath` value contains: `/xyz/openbmc_project/` (this is checked after
external), then it's treated as a passive dbus sensor. A passive dbus sensor is
one that listens for property updates to receive its value instead of actively
reading the `Value` property.

If the `readPath` value contains: `/sys/` this is treated as a directly read
sysfs path. There are two supported paths:

*   `/sys/class/hwmon/hwmon0/pwm1`
*   `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk
    asterisk>/pwm1`

The `writePath` is the path to set the value for the sensor. This is only valid
for a sensor of type `fan`. The path is optional. If can be empty or `None`. It
then only supports two options.

If the `writePath` value contains: `/sys/` this is treated as a directory
written sysfs path. There are two support paths:

*   `/sys/class/hwmon/hwmon0/pwm1`
*   `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk
    asterisk>/pwm1`

If the `writePath` value contains: `/xyz/openbmc_project/control/fanpwm/` it
sets of a sensor object that writes over dbus to the
`xyz.openbmc_project.Control.FanPwm` interface. The `writePath` should end with
the sensor's dbus name.

**BUG NOTE** It's currently using the writePath specified for fanpwm as the
sensor path, when in fact, the interface is attached to:

```
busctl introspect xyz.openbmc_project.Hwmon-1644477290.Hwmon1 /xyz/openbmc_project/sensors/fan_tach/fan1 --no-pager
NAME                                TYPE      SIGNATURE RESULT/VALUE                             FLAGS
org.freedesktop.DBus.Introspectable interface -         -                                        -
.Introspect                         method    -         s                                        -
org.freedesktop.DBus.Peer           interface -         -                                        -
.GetMachineId                       method    -         s                                        -
.Ping                               method    -         -                                        -
org.freedesktop.DBus.Properties     interface -         -                                        -
.Get                                method    ss        v                                        -
.GetAll                             method    s         a{sv}                                    -
.Set                                method    ssv       -                                        -
.PropertiesChanged                  signal    sa{sv}as  -                                        -
xyz.openbmc_project.Control.FanPwm  interface -         -                                        -
.Target                             property  t         255                                      emits-change writable
xyz.openbmc_project.Sensor.Value    interface -         -                                        -
.MaxValue                           property  x         0                                        emits-change writable
.MinValue                           property  x         0                                        emits-change writable
.Scale                              property  x         0                                        emits-change writable
.Unit                               property  s         "xyz.openbmc_project.Sensor.Value.Uni... emits-change writable
.Value                              property  x         2823                                     emits-change writable
```

The `minimum` and `maximum` values are optional. When `maximum` is non-zero it
expects to write a percentage value converted to a value between the minimum and
maximum.
