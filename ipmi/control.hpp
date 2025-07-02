#pragma once

#include <cstdint>
#include <string>

namespace pid_control_ipmi
{

// Implement this interface to control a zone's mode or read back its status.
class ZoneControlInterface
{
  public:
    virtual ~ZoneControlInterface() = default;

    // Reads the fan control property (either manual or failsafe) and returns an
    // IPMI code based on success or failure of this.
    virtual uint8_t getFanCtrlProperty(uint8_t zoneId, bool* value,
                                       const std::string& property) = 0;

    // Sets the fan control property (only manual mode is settable presently)
    // and returns an IPMI code based on success or failure of this.
    virtual uint8_t setFanCtrlProperty(uint8_t zoneId, bool value,
                                       const std::string& property) = 0;
};

} // namespace pid_control_ipmi
