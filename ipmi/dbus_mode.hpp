#pragma once

#include "control.hpp"

#include <cstdint>
#include <string>

namespace pid_control_ipmi
{

class DbusZoneControl : public ZoneControlInterface
{
  public:
    DbusZoneControl() = default;
    ~DbusZoneControl() override = default;

    /*
     * busctl call xyz.openbmc_project.State.FanCtrl \
     *     /xyz/openbmc_project/settings/fanctrl/zone1 \
     *     org.freedesktop.DBus.Properties \
     *     GetAll \
     *     s \
     *     xyz.openbmc_project.Control.Mode
     * a{sv} 2 "Manual" b false "FailSafe" b false
     *
     * This returns an IPMI code as a uint8_t (which will always be sufficient
     * to hold the result). NOTE: This does not return the typedef value to
     * avoid including a header with conflicting types.
     */
    uint8_t getFanCtrlProperty(uint8_t zoneId, bool* value,
                               const std::string& property) override;

    uint8_t setFanCtrlProperty(uint8_t zoneId, bool value,
                               const std::string& property) override;
};

} // namespace pid_control_ipmi
