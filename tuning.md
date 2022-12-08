# PID Control Tuning & Logging

The openBMC PID control daemon, swampd (phosphor-pid-control) requires the user
to specify the sensors and PID coefficients. Determining good coefficients is
beyond the scope of this document.

**NOTE** The steps below may not be applicable if you are using D-Bus based
configuration.

## Tuning Fan PID Using a Fixed RPM Setpoint

Flag `"-t"` can be specified to enabled the daemon to read the fan RPM setpoint
from a file `/etc/thermal.d/setpoint` instead from D-Bus.

The value in the setpoint file is expected to be a normal decimal integer, such
as `4000` or `5000`, and is in RPM.

## Tuning Fan PID Control Parameters

`phosphor-pid-control` reads PID control values from
`/usr/share/swampd/config.json`, one can modify the PID parameters in the config
file and restart the daemon to make the new values effective.

```
systemctl restart phosphor-pid-control.service
```

## Logging

Flag `"-l \<path\>"` can be specified to enable the daemon to log fan control
data into `path`. The log output is in CSV format with the following header:

```
epoch_ms,setpt,fan1,fan2,...fanN,fleeting,failsafe
```

`phosphor-pid-control` will create a log for each PID control zone.

## Core Logging

For even more detailed logging, flag `"-g"` can be specified to enable the
daemon to log the computations made during the core of the PID loop.

The log output is in CSV format, in the same directory as specified with the
`"-l"` option. Default is the system temporary directory, typically `/tmp`.

Two files will be created, for each PID loop configured. The `pidcoeffs.*` file
will show the PID coefficients that are in use for the PID loop. The `pidcore.*`
file will show the computations that take place, to transform the input into the
output. The configured name of the PID loop is used as the suffix, for both of
these files.

The `pidcoeffs.*` file will grow slowly, updated only when new coefficients are
set using D-Bus without restarting the program. The `pidcore.*` file, on the
other hand, will grow rapidly, as it will be updated during each PID loop pass
in which there were changes. To prevent needless log file growth, identical
logging lines are throttled, unless it has been at least 60 seconds since the
last line.

## Fan RPM Tuning Helper script

`https://github.com/openbmc/phosphor-pid-control/blob/master/tools/fan_rpm_loop_test.sh`
is an example script on how to sweep through available RPM setpoints and log the
fan responses.

## Thermal Tuning Example

1.  Create initial `/usr/share/swampd/config.json` used for PID control
2.  (Option 1) If using a fixed setpoint, write the value to
    `/etc/thermal.d/setpoint`, run swampd manually with
    `swampd -l ${LOG_PATH}&`, and kill the process after desired duration.
3.  (Option 2) If sweeping fan setpoint, using the tuning helper script
    `fan_rpm_loop_test.sh` to configure fan setpoint in steps and collect logs
4.  Parse logs from `${LOG_PATH}/zone_*.log` and analyze response data
5.  Modify `/usr/share/swampd/config.json` as needed
6.  Repeat from step 2 or step 3
