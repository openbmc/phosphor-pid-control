# Fan Configuration Versioning

When we have different source fans or fan controllers using the same BMC
firmware, we may have different configurations for fan control. Instead of
checking details of configuration, having a version of configuration should be
easy for user to know the current setting.

## Software Version Management

There is a set of system software management applications provided by
`phosphor-bmc-code-mgmt`. It claims the d-bus name
`xyz.openbmc_project.Software.BMC.Updater` which contains software version.

For fan configuration versioning, we might have object path likes:

```console

# busctl tree xyz.openbmc_project.Software.BMC.Updater
`-/xyz
  `-/xyz/openbmc_project
    `-/xyz/openbmc_project/software
      |-/xyz/openbmc_project/software/bmc_image
      |-/xyz/openbmc_project/software/bios_active
      `-/xyz/openbmc_project/software/fan_configuration

# busctl introspect xyz.openbmc_project.Software.BMC.Updater /xyz/openbmc_project/software/fan_configuration
NAME                                               TYPE      SIGNATURE RESULT/VALUE                             FLAGS
org.freedesktop.DBus.Introspectable                interface -         -                                        -
.Introspect                                        method    -         s                                        -
org.freedesktop.DBus.Peer                          interface -         -                                        -
.GetMachineId                                      method    -         s                                        -
.Ping                                              method    -         -                                        -
org.freedesktop.DBus.Properties                    interface -         -                                        -
.Get                                               method    ss        v                                        -
.GetAll                                            method    s         a{sv}                                    -
.Set                                               method    ssv       -                                        -
.PropertiesChanged                                 signal    sa{sv}as  -                                        -
xyz.openbmc_project.Association.Definitions        interface -         -                                        -
.Associations                                      property  a(sss)    1 "inventory" "activation" ""            emits-change writable
xyz.openbmc_project.Common.FilePath                interface -         -                                        -
.Path                                              property  s         "$config_path"                           emits-change writable
xyz.openbmc_project.Inventory.Decorator.Compatible interface -         -                                        -
.Names                                             property  as        0                                        emits-change writable
xyz.openbmc_project.Software.Activation            interface -         -                                        -
.Activation                                        property  s         "xyz.openbmc_project.Software.Activat... emits-change writable
.RequestedActivation                               property  s         "xyz.openbmc_project.Software.Activat... emits-change writable
xyz.openbmc_project.Software.ExtendedVersion       interface -         -                                        -
.ExtendedVersion                                   property  s         "1.0"       emits-change writable
xyz.openbmc_project.Software.RedundancyPriority    interface -         -                                        -
.Priority                                          property  y         0                                        emits-change writable
xyz.openbmc_project.Software.Version               interface -         -                                        -
.Purpose                                           property  s         "xyz.openbmc_project.Software.Version... emits-change writable
.Version                                           property  s         "1.0"       emits-change writable

```

We propose that the d-bus path `/xyz/openbmc_project/software/fan_configuration`
is published by `phosphor-bmc-code-mgmt`, but properties of interfaces are
maintained by `phosphor-pid-control`. The approach for configuring
`phosphor-pid-control` is either using JSON config or using `entity-manager`. We
can include version information in configuration and `phosphor-pid-control`
should set properties on the interfaces of fan version.

## Redfish Software Inventory API

The bmcweb has supported URL
`/redfish/v1/UpdateService/FirmwareInventory/`.
([Redfish.md](https://github.com/openbmc/bmcweb/blob/master/Redfish.md#redfishv1updateservicefirmwareinventory))
It gets firmware inforamtion from dbus
`xyz.openbmc_project.Software.BMC.Updater` which was mentioned above. If the fan
configuration version is published and maintained there, the user can get fan
configuration information via Redfish URL.
