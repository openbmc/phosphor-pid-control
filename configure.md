# How to Configure Phosphor-pid-control

A system needs two groups of configurations: zones and sensors.

They can come either from a dedicated config file or via D-Bus from e.g.
`entity-manager`.

## D-Bus Configuration

If config file does not exist the configuration is obtained from a set of D-Bus
interfaces. When using `entity-manager` to provide them refer to `Pid`,
`Pid.Zone` and `Stepwise`
[schemas](https://github.com/openbmc/entity-manager/tree/master/schemas). The
key names are not identical to JSON but similar enough to see the
correspondence.

## Compile Flag Configuration

### --strict-failsafe-pwm

This build flag is used to set the fans strictly at the failsafe percent when in
failsafe mode, even when the calculated PWM is higher than failsafe PWM. Without
this enabled, the PWM is calculated and set to the calculated PWM **or** the
failsafe PWM, whichever is higher.

### --offline-failsafe-pwm

This build flag is used to set the fans to failsafe percent when offline.
The controller is offline when it's rebuilding the configuration or when it's
about to shutdown.

## JSON Configuration

Default config file path `/usr/share/swampd/config.json` can be overridden by
using `--conf` command line option.

The JSON object should be a dictionary with two keys, `sensors` and `zones`.
`sensors` is a list of the sensor dictionaries, whereas `zones` is a list of
zones.

### Sensors

```
"sensors" : [
    {
        "name": "fan1",
        "type": "fan",
        "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan1",
        "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm1",
        "min": 0,
        "max": 255,
        "ignoreDbusMinMax": true
        "unavailableAsFailed": true
    },
    {
        "name": "fan2",
        "type": "fan",
        "readPath": "/xyz/openbmc_project/sensors/fan_tach/fan2",
        "writePath": "/sys/devices/platform/ahb/ahb:apb/1e786000.pwm-tacho-controller/hwmon/**/pwm2",
        "min": 0,
        "max": 255,
        "timeout": 4,
    },
...
```

A sensor has a `name`, a `type`, a `readPath`, a `writePath`, a `minimum` value,
a `maximum` value, a `timeout`, a `ignoreDbusMinMax` and a `unavailableAsFailed`
value.

The `name` is used to reference the sensor in the zone portion of the
configuration.

The `type` is the type of sensor it is. This influences how its value is
treated. Supported values are: `fan`, `temp`, and `margin`.

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

- `/sys/class/hwmon/hwmon0/pwm1`
- `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk asterisk>/pwm1`

The `writePath` is the path to set the value for the sensor. This is only valid
for a sensor of type `fan`. The path is optional. If can be empty or `None`. It
then only supports two options.

If the `writePath` value contains: `/sys/` this is treated as a directory
written sysfs path. There are two support paths:

- `/sys/class/hwmon/hwmon0/pwm1`
- `/sys/devices/platform/ahb/1e786000.pwm-tacho-controller/hwmon/<asterisk asterisk>/pwm1`

If the `writePath` value contains:
`/xyz/openbmc_project/sensors/fan_tach/fan{N}` it sets of a sensor object that
writes over dbus to the `xyz.openbmc_project.Control.FanPwm` interface. The
`writePath` should be the full object path.

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

The `timeout` value is optional and controls the sensor failure behavior. If a
sensor is a fan the default value is 2 seconds, otherwise it's 0. When a
sensor's timeout is 0 it isn't checked against a read timeout failure case. If a
sensor fails to be read within the timeout period, the zone goes into failsafe
to handle the case where it doesn't know what to do -- as it doesn't have all
its inputs.

The `ignoreDbusMinMax` value is optional and defaults to false. The dbus passive
sensors check for a `MinValue` and `MaxValue` and scale the incoming values via
these. Setting this property to true will ignore `MinValue` and `MaxValue` from
dbus and therefore won't call any passive value scaling.

The `unavailableAsFailed` value is optional and defaults to true. However, some
specific thermal sensors should not be treated as Failed when they are
unavailable. For example, when a system is powered-off, its CPU/DIMM Temp
sensors are unavailable, in such state these sensors should not be treated as
Failed and trigger FailSafe. This is important for systems whose Fans are always
on. For these specific sensors set this property to false.

### Zones

```
"zones" : [
        {
            "id": 1,
            "minThermalOutput": 3000.0,
            "failsafePercent": 75.0,
            "pids": [],
...
```

Each zone has its own fields, and a list of controllers.

| field              | type              | meaning                                                                                                                         |
| ------------------ | ----------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| `id`               | `int64_t`         | This is a unique identifier for the zone.                                                                                       |
| `minThermalOutput` | `double`          | This is the minimum value that should be considered from the thermal outputs. Commonly used as the minimum fan RPM.             |
| `failsafePercent`  | `double`          | If there is a fan PID, it will use this value if the zone goes into fail-safe as the output value written to the fan's sensors. |
| `pids`             | `list of strings` | Fan and thermal controllers used by the zone.                                                                                   |

The `id` field here is used in the d-bus path to talk to the
`xyz.openbmc_project.Control.Mode` interface.

**_TODO:_** Examine how the fan controller always treating its output as a
percentage works for future cases.

A zone collects all the setpoints and ceilings from the thermal controllers
attached to it, selects the maximum setpoint, clamps it by the minimum ceiling
and `minThermalOutput`; the result is used to control fans.

### Controllers

There are `fan`, `temp`, `margin` (PID), and `stepwise` (discrete steps)
controllers.

The `fan` PID is meant to drive fans or other cooling devices. It's expecting to
get the setpoint value from the owning zone and then drive the fans to that
value.

A `temp` PID is meant to drive the setpoint given an absolute temperature value
(higher value indicates warmer temperature).

A `margin` PID is meant to drive the setpoint given a margin value (lower value
indicates warmer temperature, in other words, it's the safety margin remaining
expressed in degrees Celsius).

The setpoint output from the thermal controllers is called `RPMSetpoint()`
However, it doesn't need to be an RPM value.

**_TODO:_** Rename this method and others to not say necessarily RPM.

Some PID configurations have fields in common, but may be interpreted
differently.

When using D-Bus, each configuration can have a list of strings called
`Profiles`. In this case the controller will be loaded only if at least one of
them is returned as `Current` from an object implementing
`xyz.openbmc_project.Control.ThermalMode` interface (which can be anywhere on
D-Bus). `swampd` will automatically reload full configuration whenever `Current`
is changed.

D-Bus `Name` attribute is used for indexing in certain cases so should be unique
for all defined configurations.

#### PID Field

If the PID `type` is not `stepwise` then the PID field is defined as follows:

| field                | type     | meaning                                                                  |
| -------------------- | -------- | ------------------------------------------------------------------------ |
| `samplePeriod`       | `double` | How frequently the value is sampled. 0.1 for fans, 1.0 for temperatures. |
| `proportionalCoeff`  | `double` | The proportional coefficient.                                            |
| `integralCoeff`      | `double` | The integral coefficient.                                                |
| `feedFwdOffsetCoeff` | `double` | The feed forward offset coefficient.                                     |
| `feedFwdGainCoeff`   | `double` | The feed forward gain coefficient.                                       |
| `integralLimit_min`  | `double` | The integral minimum clamp value.                                        |
| `integralLimit_max`  | `double` | The integral maximum clamp value.                                        |
| `outLim_min`         | `double` | The output minimum clamp value.                                          |
| `outLim_max`         | `double` | The output maximum clamp value.                                          |
| `slewNeg`            | `double` | Negative slew value to dampen output.                                    |
| `slewPos`            | `double` | Positive slew value to accelerate output.                                |

The units for the coefficients depend on the configuration of the PIDs.

If the PID is a `margin` controller and its `setpoint` is in centigrade and
output in RPM: proportionalCoeff is your p value in units: RPM/C and integral
coefficient: RPM/C sec

If the PID is a fan controller whose output is pwm: proportionalCoeff is %/RPM
and integralCoeff is %/RPM sec.

**_NOTE:_** The sample periods are specified in the configuration as they are
used in the PID computations, however, they are not truly configurable as they
are used for the update periods for the fan and thermal sensors.

#### type == "fan"

```
"name": "fan1-5",
"type": "fan",
"inputs": ["fan1", "fan5"],
"setpoint": 90.0,
"pid": {
...
}
```

The type `fan` builds a `FanController` PID.

| field      | type              | meaning                                                                        |
| ---------- | ----------------- | ------------------------------------------------------------------------------ |
| `name`     | `string`          | The name of the PID. This is just for humans and logging.                      |
| `type`     | `string`          | `fan`                                                                          |
| `inputs`   | `list of strings` | The names of the sensor(s) that are used as input and output for the PID loop. |
| `setpoint` | `double`          | Presently UNUSED                                                               |
| `pid`      | `dictionary`      | A PID dictionary detailed above.                                               |

#### type == "margin"

```
"name": "fleetingpid0",
"type": "margin",
"inputs": ["fleeting0"],
"setpoint": 10,
"pid": {
...
}
```

The type `margin` builds a `ThermalController` PID.

| field      | type              | meaning                                                                      |
| ---------- | ----------------- | ---------------------------------------------------------------------------- |
| `name`     | `string`          | The name of the PID. This is just for humans and logging.                    |
| `type`     | `string`          | `margin`                                                                     |
| `inputs`   | `list of strings` | The names of the sensor(s) that are used as input for the PID loop.          |
| `setpoint` | `double`          | The setpoint value for the thermal PID. The setpoint for the margin sensors. |
| `pid`      | `dictionary`      | A PID dictionary detailed above.                                             |

Each input is normally a temperature difference between some hardware threshold
and the current state. E.g. a CPU sensor can be reporting that it's 20 degrees
below the point when it starts thermal throttling. So the lower the margin
temperature, the higher the corresponding absolute value.

Out of all the `inputs` the minimal value is selected and used as an input for
the PID loop.

The output of a `margin` PID loop is that it sets the setpoint value for the
zone. It does this by adding the value to a list of values. The value chosen by
the fan PIDs (in this cascade configuration) is the maximum value.

#### type == "temp"

Exactly the same as `margin` but all the inputs are supposed to be absolute
temperatures and so the maximal value is used to feed the PID loop.

#### type == "stepwise"

```
"name": "temp1",
"type": "stepwise",
"inputs": ["temp1"],
"setpoint": 30.0,
"pid": {
  "samplePeriod": 0.1,
  "positiveHysteresis": 1.0,
  "negativeHysteresis": 1.0,
  "isCeiling": false,
  "reading": {
    "0": 45,
    "1": 46,
    "2": 47,
  },
  "output": {
    "0": 5000,
    "1": 2400,
    "2": 2600,
  }
}
```

The type `stepwise` builds a `StepwiseController`.

| field    | type              | meaning                                                                          |
| -------- | ----------------- | -------------------------------------------------------------------------------- |
| `name`   | `string`          | The name of the controller. This is just for humans and logging.                 |
| `type`   | `string`          | `stepwise`                                                                       |
| `inputs` | `list of strings` | The names of the sensor(s) that are used as input and output for the controller. |
| `pid`    | `dictionary`      | A controller settings dictionary detailed below.                                 |

The `pid` dictionary (confusingly named) is defined as follows:

| field                | type         | meaning                                                                                              |
| -------------------- | ------------ | ---------------------------------------------------------------------------------------------------- |
| `samplePeriod`       | `double`     | Presently UNUSED.                                                                                    |
| `reading`            | `dictionary` | Enumerated list of input values, indexed from 0, must be monotonically increasing, maximum 20 items. |
| `output`             | `dictionary` | Enumerated list of output values, indexed from 0, must match the amount of `reading` items.          |
| `positiveHysteresis` | `double`     | How much the input value must raise to allow the switch to the next step.                            |
| `negativeHysteresis` | `double`     | How much the input value must drop to allow the switch to the previous step.                         |
| `isCeiling`          | `bool`       | Whether this controller provides a setpoint or a ceiling for the zone                                |
| `setpoint`           | `double`     | Presently UNUSED.                                                                                    |

**_NOTE:_** `reading` and `output` are normal arrays and not embedded in the
dictionary in Entity Manager.

Each measurement cycle out of all the `inputs` the maximum value is selected.
Then it's compared to the list of `reading` values finding the largest that's
still lower or equal the input (the very first item is used even if it's larger
than the input). The corresponding `output` value is selected if hysteresis
allows the switch (the current input value is compared with the input present at
the moment of the previous switch). The result is added to the list of setpoints
or ceilings for the zone depending on `isCeiling` setting.
