# OEM-IPMI Commands to talk to Phosphor-pid-control

## Sensor Data Pushing Advice

For temperature sensors not reachable by the BMC, sensor readings can be pushed
to the BMC from the host, by using the IPMI SDR as shared storage.

To do this, it is recommended that
[ExternalSensor](https://github.com/openbmc/docs/blob/master/designs/external-sensor.md)
be used. ExternalSensor can add virtual sensors to the IPMI SDR. The host can
then write values to these virtual sensors, to keep them up to date. Thus, these
sensors will become visible on the BMC, and can be used normally, similar to
other temperature sensors already on the BMC.

For temperature sensors on the BMC, including those added with ExternalSensor as
mentioned above, these can be manipulated through normal IPMI commands, and are
already supported. The same applies to fan RPM and PWM sensors. No additional
OEM commands are required to use these.

## IPMI OEM Command Specification

### OEM Get/Set Control Mode

A host tool needs to be able to set the control of the thermal system to either
automatic or manual. When manual, the daemon will effectively wait to be told to
be put back in automatic mode. It is expected in this manual mode that something
will be controlling the fans via the other commands.

Manual mode is controlled by zone through the following OEM command:

#### Request

| Byte | Purpose      | Value                                                 |
| ---- | ------------ | ----------------------------------------------------- |
| `00` | `netfn`      | `0x2e`                                                |
| `01` | `command`    | `0x04 (also using manual command)`                    |
| `02` | `oem1`       | `0xcf`                                                |
| `03` | `oem2`       | `0xc2`                                                |
| `04` | `padding`    | `0x00`                                                |
| `05` | `SubCommand` | `Get or Set. Get == 0, Set == 1`                      |
| `06` | `ZoneId`     |                                                       |
| `07` | `Mode`       | `If Set, Value 1 == Manual Mode, 0 == Automatic Mode` |

#### Response

| Byte | Purpose   | Value                                                 |
| ---- | --------- | ----------------------------------------------------- |
| `02` | `oem1`    | `0xcf`                                                |
| `03` | `oem2`    | `0xc2`                                                |
| `04` | `padding` | `0x00`                                                |
| `07` | `Mode`    | `If Set, Value 1 == Manual Mode, 0 == Automatic Mode` |

### OEM Get Failsafe Mode

A host tool needs to be able to read back whether a zone is in failsafe mode.
This setting is read-only because it's dynamically determined within
phosphor-pid-control per zone.

| Byte | Purpose      | Value                              |
| ---- | ------------ | ---------------------------------- |
| `00` | `netfn`      | `0x2e`                             |
| `01` | `command`    | `0x04 (also using manual command)` |
| `02` | `oem1`       | `0xcf`                             |
| `03` | `oem2`       | `0xc2`                             |
| `04` | `padding`    | `0x00`                             |
| `05` | `SubCommand` | `Get == 2`                         |
| `06` | `ZoneId`     |                                    |

#### Response

| Byte | Purpose    | Value                                           |
| ---- | ---------- | ----------------------------------------------- |
| `02` | `oem1`     | `0xcf`                                          |
| `03` | `oem2`     | `0xc2`                                          |
| `04` | `padding`  | `0x00`                                          |
| `07` | `failsafe` | `1 == in Failsafe Mode, 0 not in failsafe mode` |
